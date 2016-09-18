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
#include "common/observation.hpp"
#include "vision/config.hpp"
#include "vision/vision.hpp"
#include "vision/blob_detection.hpp"
#include "image/image.hpp"

auto yfilter = nnFilter(
{
	{
		{-6.38772640e-03,  -3.78002548e+00,   3.99126935e+00},
		{ -1.77165282e+00,  -2.19997425e+01,   3.45455627e+01},
		{ 7.82506514e+00,  -1.33561554e+01,  -5.29454184e+00},
		{ 1.91624129e+00,  -2.55934963e+01,  -7.05454230e-01},
	},
	{
		{-3.34416962},
		{-1.65786171},
		{7.93236494},
		{8.20481205},
	},
	{{ -2.30254245, -17.72804451, -15.3492012 ,  14.54854965}},
	{{6.69232321}},
});

auto ofilter = nnFilter(
{
	{
		{-122.57562256,  -34.30429459,  -58.89888382},
		{8.42154789,  119.62509155,  -31.02238846},
		{-127.1678009 ,  217.79257202, -116.3327179},
		{ 7.98526287,  -99.36182404,   97.6957016 },
	},
	{
		{50.17668533},
		{-58.20920563},
		{-43.98231506},
		{9.47482681},
	},
	{{ -84.03649139,  -51.35709381,  -73.33837128, -110.05463409}},
	{{78.57619476}},
});

const float cropx = .8;
const float cropy = .4;
const float offset = 0;
const float scalex = 256;
const float scaley = 192;
//const float scalex = 0.2;
//const float scaley = 0.2;
const int diffDist = 8;

const float boardHeight = 1;

const float smallHoleHeight = .1778f;
const float largeHoleHeight = .3048f;
const float tlHeight = largeHoleHeight;
const float blHeight = smallHoleHeight;
const float trHeight = smallHoleHeight;
const float brHeight = largeHoleHeight;

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
			cv::Size(cropx*scalex, cropy*scaley),
			0, 0,
			cv::INTER_NEAREST
		);
		size_t rows = image.rows;
		size_t cols = image.cols;
		unsigned char* ptr = image.ptr();

		cv::Mat imgB;
		cv::blur(image, imgB, cv::Size(2, 2));

		cv::Mat imgF = filter(imgB, yfilter);

		// fill in gaps
		cv::Mat imgD;
		int dradius = 3;
		cv::dilate(imgF, imgD, cv::getStructuringElement(
			cv::MORPH_RECT,
			cv::Size(2 * dradius + 1, 2 * dradius + 1),
			cv::Point(dradius, dradius)
		));

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

		cv::Mat imgT;
		cv::threshold(imgS, imgT, .5 * max, 1, cv::THRESH_BINARY);
		imgT.convertTo(imgT, CV_8UC1, 255);

		cv::Mat imgO = filter(imgB, ofilter);
		cv::Mat imgE;
		int eradius = 2;
		cv::erode(imgO, imgE, cv::getStructuringElement(
			cv::MORPH_RECT,
			cv::Size(2 * eradius + 1, 2 * eradius + 1),
			cv::Point(eradius, eradius)
		));
		img = imgE;

		float cmax = -1000000;
		int cr = 0;
		int cc = 0;
		for (int r = 0; r < rows; r++)
		{
			for (int c = 0; c < cols; c++)
			{
				if (img.at<float>(r, c) > cmax)
				{
					cmax = img.at<float>(r, c);
					cr = r;
					cc = c;
				}
			}
		}


/*
		auto imgPB = imgT;
		cv::Mat imgP(imgPB.size(), CV_8UC3);
		for (size_t i = 0; i < imgPB.rows*imgPB.cols; i++)
		{
			int value = imgPB.ptr()[i];
			imgP.ptr()[3*i] = value;
			imgP.ptr()[3*i+1] = value;
			imgP.ptr()[3*i+2] = value;
		}
*/
		cv::Mat imgP;
		image.copyTo(imgP);
//		cv::cvtColor(imgT, imgP, CV_GRAY2BGR);
		cv::circle(imgP, cv::Point(cc, cr), 3, cv::Scalar(0, 127, 255));

		std::vector<Observation> observations;

		// cover
/*
		observations.push_back(
		{
			M_TORP_C_H,
			0,
			-3,
			static_cast<float>(cc - img.cols/2) / img.cols,
			0,
			.1f
		});
		observations.push_back(
		{
			M_TORP_C_V,
			0,
			-3,
			static_cast<float>(cr - img.rows/2) / img.rows,
			0,
			.1f
		});
*/

		auto yblobs = blob_detection(imgT);
		// remove blobs not containing highest orange pixel
		yblobs.erase(std::remove_if(yblobs.begin(), yblobs.end(), [=](const Blob b)
		{
			return
				b.max_x < mc ||
				b.min_x > mc ||
				b.max_y < mr ||
				b.min_y > mr ||

				false;
		}), yblobs.end());
/*
		yblobs.erase(std::remove_if(yblobs.begin(), yblobs.end(), [=](const Blob b)
		{
			return
				b.max_x < cc ||
				b.min_x > cc ||
				b.max_y < cr ||
				b.min_y > cr ||

				false;
		}), yblobs.end());
*/
		if (yblobs.size() > 0)
		{
			auto board = yblobs.at(0);

			board.drawBlob(imgP, cv::Scalar(0, 255, 255));

			auto points = board.convexHull;
			auto c = board.getCentroid();
			int maxL = board.min_y;
			int maxR = board.min_y;
			int minL = board.max_y;
			int minR = board.max_y;
			for (auto p : points)
			{
				if (p.x > c.x) // right
				{
					if (p.y > maxR) maxR = p.y;
					if (p.y < minR) minR = p.y;
				}
				else // left
				{
					if (p.y > maxL) maxL = p.y;
					if (p.y < minL) minL = p.y;
				}
			}

			float ratio = static_cast<float>(maxR - minR) / (maxL - minL);
			auto getDist = [=](int height_image, int rows, float height_actual)
			{
				return height_actual/2 / std::tan(static_cast<float>(height_image) / rows * fvFOV / 2 * 2*M_PI);
			};
			auto getAngle = [=](int col, int cols)
			{
				return static_cast<float>(col - cols/2) / cols * fhFOV;
			};
			float lt = getAngle(board.min_x, img.cols);
			float rt = getAngle(board.max_x, img.cols);
			float lr = getDist(maxL - minL, img.rows, boardHeight);
			float rr = getDist(maxR - minR, img.rows, boardHeight);

			float lx = lr * std::cos(lt * 2*M_PI);
			float rx = rr * std::cos(rt * 2*M_PI);
			float ly = lr * std::sin(lt * 2*M_PI);
			float ry = rr * std::sin(rt * 2*M_PI);

			lx = lr;
			rx = ry;
			ly = 0;
			ry = 1;

			float skew = std::atan2(ry - ly, rx - lx) / (2 * M_PI);

			observations.push_back(
			{
				M_TORP_SKEW, 0, -4, skew, 0, .3,
			});

			float theta = fhFOV * static_cast<float>(c.x - img.cols/2) / img.cols;
			float phi = fvFOV * static_cast<float>(c.y - img.rows/2) / img.rows;
			float dist = boardHeight/2 / std::tan(static_cast<float>(board.max_y - board.min_y)/img.rows * fvFOV / 2 * 2*M_PI);

			observations.push_back(
			{
				M_TORP_C_H,
				0,
				-3,
				static_cast<float>(c.x - img.cols/2) / img.cols,
				0,
				.1f
			});
			observations.push_back(
			{
				M_TORP_C_V,
				0,
				-3,
				static_cast<float>(c.y - img.rows/2) / img.rows,
				0,
				.1f
			});
			observations.push_back(
			{
				M_TORP_DIST, 
				0,
				-3,
				dist,
				0,
				.5,
			});
		}
		else
		{
			observations.push_back(
			{
				M_TORP_DIST, 
				0,
				-3,
				3,
				0,
				2,
			});
		}
		imageWrite(log, imgP);

		fprintf(out, "%zu\n", observations.size());
		for (auto o : observations)
			o.write(out);
		fflush(out);
	}
		 
	return 0;
}
