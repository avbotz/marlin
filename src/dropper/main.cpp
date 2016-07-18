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
#include "common/config.hpp"
#include "common/observation.hpp"
#include "vision/config.hpp"
#include "vision/vision.hpp"
#include "vision/blob_detection.hpp"
#include "image/image.hpp"

const float cropx = 1.0;
const float cropy = 1.0;
const float offset = 0.9 * (1 - cropy);
const float scalex = 256;
const float scaley = 192;
//const float scalex = 0.2;
//const float scaley = 0.2;
const int diffDist = 8;

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

//Turns the mat into a diffmap (make each pixel the difference between it and its neighbors)
//Useful because the water near the top usually looks like the green buoys near the bottom
cv::Mat generateDiffMap(cv::Mat& img, int diff)
{
	cv::Mat diffMap = cv::Mat(img.size(), CV_32F, cv::Scalar(0));

	float* ip = img.ptr<float>();
	float* op = diffMap.ptr<float>();

	for (int c = diff; c < img.cols - diff; c++)
	{
		for (int r = diff; r < img.rows - diff; r++)
		{
			// make the value equal to how much it stands out from its\
				horizontal or vertical neighbors, whichever is less
			float vert = (2*ip[(r*img.cols+c)]-ip[((r-diff)*img.cols+c)]-ip[((r+diff)*img.cols+c)]);
			float hori = (2*ip[(r*img.cols+c)]-ip[(r*img.cols+c+diff)]-ip[(r*img.cols+c-diff)]);
			float weak = (std::abs(vert) < std::abs(hori)) ? vert : hori;
			op[r*img.cols+c] = weak;
		}
	}
 
	return diffMap;
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

		cv::Mat image_orig;
		image.copyTo(image_orig);

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
		cv::blur(image, imgB, cv::Size(3, 3));

		auto nn = nnFilter(
		{
			{
				{ -31.24248886,   15.26303673,   -3.1319592},
				{  -7.5420475 ,   15.32956123,   22.78780556},
				{  18.96827316, -104.11196899,  -21.70586586},
				{   5.77539253,  -12.49763107,  -18.34315109},
			},
			{
				{-0.29773164},
				{20.6458149},
				{107.48106384},
				{-16.6441021},
			},
			{{-10.3265419 , -35.6080513 ,  57.83471298,  10.05905247},},
			{{-18.51780128}},
		});
		auto imgF = filter(imgB, nn);

		cv::Mat imgE;
		int radius = 5;
		cv::erode(imgF, imgE, cv::getStructuringElement(
			cv::MORPH_ELLIPSE,
			cv::Size(2 * radius + 1, 2 * radius + 1),
			cv::Point(radius, radius)
		));

		cv::Mat imgS = scaleIntensity(imgE);

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

		float x = static_cast<float>(img.rows/2 - mr + .001f) / img.rows; // up
		float y = static_cast<float>(mc - img.cols/2 + .001f) / img.cols; // right

		fprintf(out, "1\n");
		Observation(
			M_CBIN_X, M_CBIN_Y, -1,
			std::atan2(y, x) / (2*M_PI),
			.25 - x * dvFOV,
			constants.get(C_BIN_D)
		).write(out);

		cv::Mat imgP;
//		image.copyTo(imgP);
		cv::cvtColor(imgS, imgP, CV_GRAY2BGR);
		static_cast<cv::Mat>(imgP*255).convertTo(imgP, CV_8UC3);
		cv::circle(imgP, cv::Point(mc, mr), 3, cv::Scalar(0, 255, 255));
		imageWrite(log, imgP);
	}

	return 0;
}

