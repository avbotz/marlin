#include <cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <flycapture/FlyCapture2.h>
#include <cstdio>
#include <iomanip>
#include <thread>
#include <chrono>

#include "image/image.hpp"

int startCamera(FILE* in, FILE* out, FILE* log, FlyCapture2::PGRGuid guid, bool flip)
{
	FlyCapture2::Error error;
	FlyCapture2::Camera cam;

	// connect
	error = cam.Connect(&guid);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	// start and stop capturing and disconnect and reconnect to clear buffer
	error = cam.StartCapture();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}
	error = cam.StopCapture();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}
	error = cam.Disconnect();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}
	error = cam.Connect(&guid);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	// set mode (pixel format, fps)
	FlyCapture2::Format7ImageSettings format;
	unsigned int packetSize;
	float percentage;
	error = cam.GetFormat7Configuration(&format, &packetSize, &percentage);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	format.mode = FlyCapture2::MODE_1;
	format.offsetX = 0;
	format.offsetY = 0;
	format.width = 644;
	format.height = 482;
	format.pixelFormat = FlyCapture2::PIXEL_FORMAT_RGB8;
	bool valid;
	FlyCapture2::Format7PacketInfo packetInfo;
	error = cam.ValidateFormat7Settings(&format, &valid, &packetInfo);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}
	if (!valid)
	{
		fprintf(stderr, "error: invalid format\n");
		return 1;
	}

	error = cam.SetFormat7Configuration(&format, packetInfo.recommendedBytesPerPacket);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	error = cam.GetFormat7Configuration(&format, &packetSize, &percentage);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	// start capturing
	error = cam.StartCapture();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	bool quit = false;
	while (!quit)
	{
		// wait for image request
		int c = fgetc(in);
		switch (c)
		{
			case 'i':
			{
				// get image
				FlyCapture2::Image fcImage;
				error = cam.RetrieveBuffer(&fcImage);
				if (error != FlyCapture2::PGRERROR_OK)
				{
					fprintf(log, "error: getting image\n");
					error.PrintErrorTrace();
					continue;
				}

				// convert
				FlyCapture2::Image bgr;
				error = fcImage.Convert(FlyCapture2::PIXEL_FORMAT_BGR, &bgr);
				if (error != FlyCapture2::PGRERROR_OK)
				{
					fprintf(log, "error: converting image\n");
					error.PrintErrorTrace();
					continue;
				}

				// convert to opencv mat
				cv::Mat image(bgr.GetRows(), bgr.GetCols(), CV_8UC3, bgr.GetData(), bgr.GetStride());

				// flip image if flip == true
				if(flip)
					flipImage(image, image, -1);

				// send image
				imageWrite(out, image);

				break;
			}

			case 'q':
			case EOF:
				quit = true;
				break;
		}

		// wait a bit
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// stop capturing
	error = cam.StopCapture();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	// disconnect
	error = cam.Disconnect();
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	return 0;
}

int main(int argc, char** argv)
{

	FlyCapture2::Error error;

	FlyCapture2::BusManager busMgr;
	unsigned int numCameras;
	error = busMgr.GetNumOfCameras(&numCameras);
	if (error != FlyCapture2::PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return 1;
	}

	if (numCameras < 1)
	{
		fprintf(stderr, "error: no camera detected\n");
		return 1;
	}

	FlyCapture2::PGRGuid guid;

	if (argc < 3)
	{
		fprintf(stderr, "Error: Not enough arguments.\n");
		return 1;
	}

	switch (argv[1][0])
	{
		case 'i': // index
		{
			unsigned int sn;
			sscanf(argv[2], "%i", &sn);
			error = busMgr.GetCameraFromIndex(sn, &guid);
			break;
		}
		case 'u': // usb serial number
		{
			unsigned int sn;
			sscanf(argv[2], "%i", &sn);
			error = busMgr.GetCameraFromSerialNumber(sn, &guid);
			break;
		}
		case 'n': // network ip address
		{
			unsigned int ip[4];
			sscanf(argv[2], "%i.%i.%i.%i", &ip[0], &ip[1], &ip[2], &ip[3]);
			unsigned int ip32 = ip[0] * (1 << 24) + ip[1] * (1 << 16) + ip[2] * (1 << 8) + ip[3];
			error = busMgr.GetCameraFromIPAddress(FlyCapture2::IPAddress(ip32), &guid);
			break;
		}
	}
	if (error != FlyCapture2::PGRERROR_OK)
	{
		fprintf(stderr, "error: failed to connect to the specified camera\n");
		error.PrintErrorTrace();
		return 1;
	}
	
	bool flip = false;
	if(argc >= 4)
	{
		flip = (argv[3][0] == '1') ? true : false;
	}

	return startCamera(stdin, stdout, stderr, guid, flip);
}
