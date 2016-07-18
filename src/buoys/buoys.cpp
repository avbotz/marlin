#include <vector>
#include <functional>
#include <algorithm>
#include <thread>
#include <chrono>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cv.h>

#include "common/defs.hpp"
#include "vision/config.hpp"
#include "vision/vision.hpp"
#include "image/image.hpp"

struct Buoy
{
	size_t x_idx, y_idx, d_idx;
	float color[3];
	std::function<float(float, float, float)> filter;
};

std::vector<Buoy> buoys =
{
	{
		M_RBUOY_X, M_RBUOY_Y, M_RBUOY_D, {0,0,1},
		[](float r, float g, float b){ return .7*r/(g+.001) - 1.20*g/(b+.001)-(g+b)*0.01; }
	},
	{
		M_YBUOY_X, M_YBUOY_Y, M_YBUOY_D, {1,0,0},
		nnFilter(
		{
			{
				{  46.85415268, -107.30097198,  100.08049011},
				{-61.59093475,   95.11291504,   64.77831268},
				{  24.69617271,  116.03449249, -220.81503296},
				{-153.43617249,  163.44508362, -111.20446777},
			},
			{
				{-21.81101036},
				{-124.47380829},
				{86.83537292},
				{4.44423008},
			},
			{{-9.57674789,  -8.67415428, -70.69998932, -52.24337387},},
			{{4.38262653}},
		})
	},
	{
		M_GBUOY_X, M_GBUOY_Y, M_GBUOY_D, {0,1,0},
		[](float r, float g, float b){ return 2*(g-.4*r)/(b+3*r+3.01); }
	},
};

const float cropx = 1.0;
const float cropy = 0.6;
const float offset = 0.0 * (1 - cropy);
const float scalex = 128;
const float scaley = 96;
//const float scalex = 0.2;
//const float scaley = 0.2;
const int diffDist = 8;
const float buoyWidth = .16;

//Creates an equalized distribution of certain colors
//Read more at http://docs.opencv.org/2.4/doc/tutorials/imgproc/histograms/histogram_equalization/histogram_equalization.html
cv::Mat equalColorHist(cv::Mat& img, bool red, bool green, bool blue)
{
	std::vector<cv::Mat> channels;
	cv::split(img, channels);

	if(blue)
	{
		cv::equalizeHist(channels[0], channels[0]);
	}
	if(green)
	{
		cv::equalizeHist(channels[1], channels[1]);
	}
	if(red)
	{
		cv::equalizeHist(channels[2], channels[2]);
	}

	cv::Mat result;
	cv::merge(channels, result);
	return result;
}

int main(int argc, char** argv)
{
	FILE* in = stdin;
	FILE* out = stdout;
	FILE* log = stderr;

	while(true)
	{
		//Obtain image from the front camera.
		cv::Mat image = imageRead(in);

		if(!image.data)
		{
			fprintf(out, "0\n");
			continue;
		}

		//Crop and resize image
		cv::resize(image(cv::Rect(image.cols*(1-cropx)/2, image.rows*(1-cropy-offset)/2, 
			image.cols*cropx, image.rows*cropy)), image, 
//			cv::Size(image.cols*cropx*scalex, image.rows*cropy*scaley));
			cv::Size(cropx*scalex, cropy*scaley));
		size_t rows = image.rows;
		size_t cols = image.cols;
		unsigned char* ptr = image.ptr();

		cv::Mat imgL;
		image.copyTo(imgL);

		struct Found
		{
			size_t x_idx, y_idx, d_idx;
			float theta;
			float phi;
			float rho;
		};
		std::vector<Found> found;

		for (auto b : buoys)
		{
			cv::Mat imgF;
			imgF.create(rows, cols, CV_32F);
			float* fptr = imgF.ptr<float>();

			for (size_t i = 0; i < rows * cols; i++)
				fptr[i] = b.filter(ptr[3*i+2]/256.f, ptr[3*i+1]/256.f, ptr[3*i+0]/256.f);

			auto imgD = generateDiffMap(imgF, diffDist);

			cv::Mat imgB = cv::Mat(imgD.size(), CV_32F, cv::Scalar(0));
			cv::blur(imgD, imgB, cv::Size(3, 3));

			cv::Mat imgS = scaleIntensity(imgD);

			auto img = imgS;

			float max = -1000000;
			int mr = 0;
			int mc = 0;
			for (int r = 0; r < rows; r++)
			{
				for (int c = 0; c < cols; c++)
				{
					if (img.at<float>(r, c) > max)
					{
						max = img.at<float>(r, c);
						mr = r;
						mc = c;
					}
				}
			}

			auto visited = std::vector<std::vector<bool> >(img.rows, std::vector<bool>(img.cols, false));
			float size = std::sqrt(floodFill(img, visited, mr, mc, .8 * max) * 4/M_PI);

			float theta = cropx *  fhFOV * static_cast<float>(mc - img.cols/2) / img.cols;
			float phi = cropy * fvFOV * static_cast<float>(mr - img.rows/2) / img.rows;
			float dist = std::max(.2, buoyWidth/2 / std::tan(size/img.cols * cropy * fvFOV / 2 * 2*M_PI) * .4 - 1.f);

			bool cont = false;
			for (auto f : found)
				if (std::abs(theta - f.theta) < .00)
					cont = true;
			if (cont) continue;

			found.push_back(
			{
				b.x_idx, b.y_idx, b.d_idx,
				theta, phi, dist
			});

			auto imgPB = img;
			cv::Mat imgP(imgPB.size(), CV_8UC3);
			for (size_t i = 0; i < imgPB.rows*imgPB.cols; i++)
			{
				int value = imgPB.ptr<float>()[i] * 255;
				imgP.ptr()[3*i] = b.color[0]*value;
				imgP.ptr()[3*i+1] = b.color[1]*value;
				imgP.ptr()[3*i+2] = b.color[2]*value;
			}
			cv::circle(imgP, cv::Point(mc, mr), 3, cv::Scalar(255*b.color[0], 255*b.color[1], 255*b.color[2]));
//			imageWrite(log, imgP);

			cv::circle(imgL, cv::Point(mc, mr), 3, cv::Scalar(255*b.color[0], 255*b.color[1], 255*b.color[2]));
		}

		fprintf(out, "%zu\n", found.size());
		for (auto f : found)
			fprintf(out, "%zu %zu %zu\n%f %f %f\n",
				f.x_idx, f.y_idx, f.d_idx,
				f.theta, f.phi,   f.rho
			);
		fflush(out);

		imageWrite(log, imgL);
	}
		 
	return 0;
}
