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

//Balances orange out of image
cv::Point balanceOrange(cv::Mat originalImage, int width, int height)
{
    //Store red-green ratio values and total pixels
    double rgbRatioTotal = 0;
    int totalPixels = 0;
    int totalBlue = 0;

    //Loop through pixels and apply formula
    for (int x = 0; x < width; x++) 
    {
        for (int y = 0; y < height; y++) 
        {
            //Reassign pixel values
            cv::Vec3b colorChannels = originalImage.at<cv::Vec3b>(y, x);
            int inputBlue = colorChannels[0];
            int inputGreen = colorChannels[1];
            int inputRed = colorChannels[2];
            int outputBlue, outputGreen, outputRed;

            //Make sure blue is not greater than red or green
            if (inputBlue < inputRed * 0.9 && inputBlue < inputGreen * 0.9) {
                outputRed = 4 * inputRed - 2 * inputGreen - inputBlue;
                outputGreen = 3 * inputGreen - 3 * inputBlue;
                outputBlue = inputBlue; 
            } else {
                outputRed = 0;
                outputGreen = 0;
                outputBlue = 0;
            }

            //Make sure values do not go under 0 or over 255 
            if (outputRed < 0) {
                outputRed = 0;
            } 
            
            if (outputRed > 255) {
                outputRed = 255;
            }

            if (outputGreen < 0) {
                outputGreen = 0;
            }
            
            if (outputGreen > 255) { 
                outputGreen = 255;
            }

            //Make sure color is mostly red
            if (outputBlue * 0.75 + outputGreen * 0.5 > outputRed){
                outputBlue = 0;
                outputGreen = 0;
                outputRed = 0;
            }

            //Make sure pixel is not black
            if (outputBlue + outputGreen + outputRed > 0) {
                if (outputGreen > 0) {

                    //Add to ratio to avg
                    float rgbRatio = (double)outputRed / (double)(outputGreen + outputBlue);
                    rgbRatioTotal += rgbRatio;
                    totalPixels += 1;
                    totalBlue += outputBlue;

                } else {
                    
                    //Make sure blue + green != 0
                    if (outputBlue > 0) {
                        float rgbRatio = (double)outputRed / (double)(outputGreen + outputBlue);
                        rgbRatioTotal += rgbRatio;
                        totalPixels += 1;
                        totalBlue += outputBlue;
                
                    } else {
                        outputBlue = 0;
                        outputGreen = 0;
                        outputRed = 0;
                    }
                }
            }

            //Reassign values to channels
            colorChannels[0] = outputBlue;
            colorChannels[1] = outputGreen;
            colorChannels[2] = outputRed;
            originalImage.at<cv::Vec3b>(y, x) = colorChannels;
        }
    }

    //Set avg red and thresholding values
    if (totalPixels != 0) {
        rgbRatioTotal /= totalPixels;
        totalBlue /= totalPixels;
    } 
    const double minRatioThreshold = rgbRatioTotal;
    const int maxBlueThreshold = totalBlue;

    //Create a mat to store the returned grayscale image
    cv::Mat returnImage = cv::Mat::zeros(height, width, CV_8UC1);

    //Assign values to blank grayscale Mat
    for (int x = 0; x < width; x++) 
    {
        for (int y = 0; y < height; y++) 
        { 
            //Reassign pixel values
            cv::Vec3b colorChannels = originalImage.at<cv::Vec3b>(y, x);
            int blue = colorChannels[0];
            int green = colorChannels[1];
            int red = colorChannels[2];

            //Find r to gb ratio, avoid floating point errors
            double rgbRatio;
            if (green + blue != 0) {
                rgbRatio = (double)red / (double)(green + blue);
            } else {
                rgbRatio = red;
            }

            //Change to 0 or 255 under threshold
            if (blue < maxBlueThreshold && rgbRatio > minRatioThreshold) {
                
                //Set grayscale value
                if (rgbRatio * 50 < 255) {
                    returnImage.at<uchar>(y, x) = rgbRatio * 50;
                } else {
                    returnImage.at<uchar>(y, x) = 255;
                } 
                

                blue = 255;
                green = 255;
                red = 255;

            } else { 

                //Change to black 
                blue = 0;
                green = 0;
                red = 0;
            }

            //Reassign values to channels
            colorChannels[0] = blue;
            colorChannels[1] = green;
            colorChannels[2] = red;
            originalImage.at<cv::Vec3b>(y, x) = colorChannels;
        }
    }

    //Erode image to remove red target
    cv::erode(returnImage, returnImage, cv::Mat(), cv::Point(-1, -1), 3, 1, 1);
    
    /*
    Optional for drawing a circle where the image is:
    */

    cv::Mat finalImage;
    cv::GaussianBlur(returnImage, finalImage, cv::Size(35, 35), 0, 0);

    double orangeMin;
    double orangeMax;
    cv::Point orangeMinLoc;
    cv::Point orangeMaxLoc;
    cv::minMaxLoc(finalImage, &orangeMin, &orangeMax, &orangeMinLoc, &orangeMaxLoc);

    return orangeMaxLoc;
}

//Retruns points of the torpedo cover
cv::Point torpedoCover(cv::Mat inputImage)
{
    //Read image and blur
    cv::Mat blurredImage;
    //cv::imshow("Original!", inputImage);
    cv::GaussianBlur(inputImage, blurredImage, cv::Size(35, 35), 0, 0);
  
    //Split image into color channels
    std::vector<cv::Mat> imageChannels;
    cv ::split(blurredImage, imageChannels);

    //Color balance to pull colors 
    for (int i = 0; i < 3; i++) 
    {
        //Reshape and copy to temporary image
        cv::Mat tmpMat;
        imageChannels[i].reshape(1, 1).copyTo(tmpMat);
        cv::sort(tmpMat, tmpMat, CV_SORT_EVERY_ROW + CV_SORT_ASCENDING);

        //Pull values to stronger colors
        int lowerBound = tmpMat.at<uchar>(cvFloor(((float)tmpMat.cols) * 0.001));
        int upperBound = tmpMat.at<uchar>(cvCeil(((float)tmpMat.cols) * 0.999));
        imageChannels[i].setTo(lowerBound, imageChannels[i] < lowerBound);
        imageChannels[i].setTo(upperBound, imageChannels[i] > upperBound);
        cv::normalize(imageChannels[i], imageChannels[i], 0, 255, cv::NORM_MINMAX);
    }

    //Post color balance image
    cv::Point maxOrangePoint;
    cv::Mat balancedImage;
    cv::merge(imageChannels, balancedImage);

    //Set image height and width for pixel loop
    const int imageWidth = balancedImage.cols;
    const int imageHeight = balancedImage.rows;

    //Filter red with formula
    maxOrangePoint = balanceOrange(balancedImage, imageWidth, imageHeight);

    cv::waitKey(0);
    return maxOrangePoint;
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
