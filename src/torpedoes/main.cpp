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

auto yfilter = [](float r, float g, float b){ return r + g - b; };

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

cv::Mat torpedo_board_yellow(cv::Mat &src){
	//define variables and read image 
	cv::Mat light_corrected, color_balanced, dst, equalized, denoised, light_corrected_ii, denoised_ii, yellow_filter;
 
	////////////////////////////////////////////////////////////	
	//start with illumination correction
	light_corrected = rgb_illumination(src, 120);
 
	//whitebalance corrected image // makes it look funny and really overcoloured, but helps
	color_crop::whitebalance_simple_wrapper(light_corrected, color_balanced, .06, .06);
 
	//blur and correct light to remove noise caused by whitebalancing
	cv::medianBlur(color_balanced, denoised, 15); 
	light_corrected_ii = rgb_illumination(denoised, 90);
	/////////////////////////////////////////////////////////////
 
	/////////////////////////////////////////////////////////
	//equalize hist // the illumination correction really helps with this, otherwise, it's distorted even worse
	//split into channels
	std::vector<cv::Mat> equal_channels;
	split(light_corrected_ii, equal_channels);
 
	//run equalization on each channel separately 
	for(int i = 0; i < 3; i++) equalizeHist(equal_channels[i], equal_channels[i]);
 
	//merge back together;
	cv::merge(equal_channels, equalized); 
 
	//medianblur to remove the noise caused by equalization
	cv::medianBlur(equalized, denoised_ii, 25); 
	///////////////////////////////////////////////////////////////
 
 
	
	//filter out values not likely to be yellow
	yellow_filter = blacklist(denoised_ii, 80, 80);

/////////////////////////////////////////////////////////////////////////////////////////////////////////

	//the green channel is most likely to show the torpedoes; discard other channels and just keep green as b&w 
	cv::Mat g;
	//split apart channel to take green 
	std::vector<cv::Mat> filter_channels;
	split(yellow_filter, filter_channels);

	g = filter_channels[1];
	
	//number of likely points to find
	int squares = 10;
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	//cycle through and find the few brightest points likely to be the torpedo board

	int erosion_size = 5;
	
	cv::Mat element = cv::getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 2*erosion_size + 1, 2*erosion_size+1 ), cv::Point( erosion_size, erosion_size ) );
	cv::erode( g, g, element );

	std::vector <bright_point> found_points;

	cv::Mat display = g.clone();

	for(int i = 0; i < squares; i++){

		found_points.push_back(bright_point());
		
		//find brightest point to identify as torpedoes 
		double gmin, gmax;
		cv::Point gminloc, gmaxloc;
		cv::minMaxLoc(g, &gmin, &gmax, &gminloc, &gmaxloc);
	 
		//if it is the point to highlight, draw a cyan 20x20 square around the point
		if(i == 0){ 
			cv::rectangle( 
				src, 
				cv::Point(gmaxloc.x - 10, gmaxloc.y - 10), 
				cv::Point(gmaxloc.x + 10, gmaxloc.y + 10), 
				cv::Scalar(255, 255, 0), 1, 8
			);
		}
 
		//if it is not the point to highlight, but a possible point, draw a reddish square around with intensity based on likelihood
		else{
			cv::rectangle( 
				src, 
				cv::Point(gmaxloc.x - 10, gmaxloc.y - 10), 
				cv::Point(gmaxloc.x + 10, gmaxloc.y + 10), 
				cv::Scalar(0, 0,(255/squares) * i), 1, 8
			);
		}
 
		//draw a black rectangle around the point so we don't get the same cluster of points over and over 
		cv::rectangle( 
			g, 
			cv::Point(gmaxloc.x - 10, gmaxloc.y - 10), 
			cv::Point(gmaxloc.x + 10, gmaxloc.y + 10), 
			cv::Scalar(0), -1, 8
		);

		//save to vector
		found_points[i].location = gmaxloc;

		found_points[i].brightness = gmax;
	
	}
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////

	//find if points are close and count them as clusters

	cv::imshow("g", display);
	cv::imshow("blacklist", yellow_filter); 
	cv::imshow("src", src);
	cv::imshow("median_blur", denoised);
	cv::waitKey(0);
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

		cv::Mat imgF = filter(image, yfilter);

		cv::Mat imgS = scaleIntensity(imgF);

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

		cv::Mat imgB;
		cv::blur(imgS, imgB, cv::Size(2, 2));

		cv::Mat imgT;
		cv::threshold(imgS, imgT, .5 * max, 1, cv::THRESH_BINARY);
		imgT.convertTo(imgT, CV_8UC1, 255);

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
		cv::circle(imgP, cv::Point(mc, mr), 3, cv::Scalar(0, 255, 255));

		std::vector<Observation> observations;

		auto yblobs = blob_detection(imgT);
		// remove blobs not containing highest pixel
		yblobs.erase(std::remove_if(yblobs.begin(), yblobs.end(), [=](const Blob b)
		{
			return
				b.max_x < mc ||
				b.min_x > mc ||
				b.max_y < mr ||
				b.min_y > mr ||

				false;
		}), yblobs.end());
		if (yblobs.size() > 0)
		{
			auto board = yblobs.at(0);

			board.drawBlob(imgP, cv::Scalar(0, 255, 255));

			cv::Mat imgI;
			cv::bitwise_not(imgT, imgI);
			auto nyblobs = blob_detection(imgI);

			nyblobs.erase(std::remove_if(nyblobs.begin(), nyblobs.end(), [=](const Blob b)
			{
				// remove blobs not completely inside board
				return
					b.min_x < board.min_x ||
					b.max_x > board.max_x ||
					b.min_y < board.min_y ||
					b.max_y > board.max_y ||

					b.area < 5 ||
					static_cast<float>(b.max_x - b.min_x + 1) / (b.max_y - b.min_y + 1) > 1.5f ||
					static_cast<float>(b.max_x - b.min_x + 1) / (b.max_y - b.min_y + 1) < 1/1.5f ||

					false;
			}), nyblobs.end());

			auto mkObservation = [&](int x_idx, int y_idx, int d_idx, Blob hole, float objHeight)
			{
				const cv::Mat& img = imgT;
				float imgHeight = hole.max_y - hole.min_y + 3;
				return Observation
				{
					x_idx, y_idx, d_idx,
					hAngle(img, hole.getCentroid().x - img.cols/2),
					vAngle(img, hole.getCentroid().y - img.cols/2),
					static_cast<float>(std::max(.2, objHeight/2 / std::max(.001, std::tan(vAngle(img, imgHeight)/2 * 2*M_PI)))),
				};
			};

			if (nyblobs.size() == 4)
			{
				std::sort(nyblobs.begin(), nyblobs.end(), [](Blob a, Blob b){ return a.getCentroid().x < b.getCentroid().x; });

				bool top = nyblobs.at(0).getCentroid().y < nyblobs.at(1).getCentroid().y;
				auto tl = nyblobs.at(top ? 0 : 1);
				auto bl = nyblobs.at(top ? 1 : 0);
				top = nyblobs.at(2).getCentroid().y < nyblobs.at(3).getCentroid().y;
				auto tr = nyblobs.at(top ? 2 : 3);
				auto br = nyblobs.at(top ? 3 : 2);

				tl.drawBlob(imgP, cv::Scalar(255, 0, 0));
				bl.drawBlob(imgP, cv::Scalar(0, 255, 0));
				tr.drawBlob(imgP, cv::Scalar(0, 0, 255));
				br.drawBlob(imgP, cv::Scalar(255, 255, 0));

				observations.push_back(mkObservation(M_TORP_L_X, M_TORP_L_Y, M_TORP_T_D, tl, tlHeight));
				observations.push_back(mkObservation(M_TORP_L_X, M_TORP_L_Y, M_TORP_B_D, bl, blHeight));
				observations.push_back(mkObservation(M_TORP_R_X, M_TORP_R_Y, M_TORP_T_D, tr, trHeight));
				observations.push_back(mkObservation(M_TORP_R_X, M_TORP_R_Y, M_TORP_B_D, br, brHeight));
			}
			else
			{
				for (auto b : nyblobs)
					b.drawBlob(imgP, cv::Scalar(255, 0, 0));
				// the holes are probably somewhere near the board
				// TODO: add high variance low certainty observations
				// don't implement this until variance can be passed through an observation object
			}
		}
		imageWrite(log, imgP);

		fprintf(out, "%i\n", observations.size());
		for (auto o : observations)
			o.write(out);
		fflush(out);
	}
		 
	return 0;
}
