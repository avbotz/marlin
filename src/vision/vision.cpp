#include <vector>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cv.h>

#include "vision/vision.hpp"

#include "common/defs.hpp"
#include "vision/config.hpp"
#include "image/image.hpp"

//Turns the mat into a diffmap (make each pixel the difference between it and its neighbors)
//Useful because the water near the top usually looks like the green buoys near the bottom
cv::Mat generateDiffMap(cv::Mat& img, int diff, bool bigger)
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
			float strong = (std::abs(vert) > std::abs(hori)) ? vert : hori;
			if(bigger)
			{
				op[r*img.cols+c] = hori;
			}
			else
			{
				op[r*img.cols+c] = weak;
			}
		}
	}
 
	return diffMap;
}

float hAngle(const cv::Mat& img, float cols)
{
	return fhFOV * cols / img.cols;
}

float vAngle(const cv::Mat& img, float rows)
{
	return fvFOV * rows / img.rows;
}

cv::Mat filter(const cv::Mat& img, std::function<float(float, float, float)> f)
{
	cv::Mat res(img.size(), CV_32F);

	for (size_t row = 0; row < img.rows; row++)
		for (size_t col = 0; col < img.cols; col++)
			res.at<float>(row, col) = f(img.at<cv::Vec3b>(row, col)[2], img.at<cv::Vec3b>(row, col)[1], img.at<cv::Vec3b>(row, col)[0]);

	return res;
}

cv::Mat scaleIntensity(const cv::Mat& img)
{
	const float* iptr = img.ptr<float>();

	cv::Mat res(img.size(), CV_32F);
	float* rptr = res.ptr<float>();

	float min = 9999999, max = -9999999;
	for (size_t i = 0; i < img.rows * img.cols; i++)
	{
		if (iptr[i] < min) min = iptr[i];
		if (iptr[i] > max) max = iptr[i];
	}

	float range = max - min;

	for (size_t i = 0; i < img.rows * img.cols; i++)
		rptr[i] = (iptr[i] - min) / range;

	return res;
}

int floodFill(const cv::Mat& img, std::vector<std::vector<bool> >& visited, int row, int col, float threshold)
{
	if (row < 0 || col < 0 || row >= img.rows || col >= img.cols || visited.at(row).at(col) || img.at<float>(row, col) < threshold)
		return 0;
	else
	{
		visited.at(row).at(col) = true;
		return 1 +
			floodFill(img, visited, row + 1, col, threshold) +
			floodFill(img, visited, row - 1, col, threshold) +
			floodFill(img, visited, row, col + 1, threshold) +
			floodFill(img, visited, row, col - 1, threshold);
	}
}
 
///////////////////////////////////////// 
//single channel illumination correction
//for more details, https://clouard.users.greyc.fr/Pantheon/experiments/illumination-correction/index-en.html 
cv::Mat illumination_correction(cv::Mat const &correction_src, int blur_kernel_size){
        cv::Mat blurred_img, diff_img, corrected_img;
        cv::Scalar mean_val;

        //Gaussian Blur only accepts odd kernel sizes
        int corrected_kernel = (blur_kernel_size * 2) + 1;
 
        //blur the image to find large areas of difference on each channel, likely to be illumination differences
        cv::GaussianBlur(correction_src, blurred_img, cv::Size(corrected_kernel, corrected_kernel), 0, 0); 
 
        //subtract lighting differences from the image 
        diff_img = correction_src - blurred_img;
        mean_val = mean(blurred_img);
 
        //add the average value to bring it up to normal brightness
        corrected_img = diff_img + cv::sum(mean_val)[0];


        return corrected_img;
} 
////////////////////////////////////////////

///////////////////////////////////////////
//wrapper for illumination correction that splits an rgb image and applies it to each channel
//might work better in hls
cv::Mat rgb_illumination(cv::Mat const &correction_src, int blur_kernel_size){
    
        cv::Mat dst;
 
        //define a vector to split the image channels into      
        std::vector<cv::Mat> channels;
        split(correction_src,channels);
 
        //cycle through and feed each channel into illumination correction
        for(int i = 0; i < 3; i++) channels[i] = illumination_correction(channels[i], blur_kernel_size);
    
        //merge the channels back together 
        cv::merge(channels, dst);

        return dst;
}
////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
//////////remove white removes pixels with similar r, g, and b values meaning that the image is likely greyscale
cv::Mat remove_white(cv::Mat &src, int white_tolerance){
 
        //cycle through every pixel 
        for(int x = 0; x < src.cols; x++){
                for(int y = 0; y < src.rows; y++){

                        //remove pixels with r, g, and b values that are too close together; these are likely white
                        if(
                                (abs((int)(src.at<cv::Vec3b>(cv::Point(x, y))[2]) - (int)(src.at<cv::Vec3b>(cv::Point(x,y))[1])) < white_tolerance) &&
k                               (abs((int)(src.at<cv::Vec3b>(cv::Point(x, y))[2]) - (int)(src.at<cv::Vec3b>(cv::Point(x,y))[0])) < white_tolerance) &&
                        ){

                                src.at<cv::Vec3b>(cv::Point(x,y))[0] = 0;
                                src.at<cv::Vec3b>(cv::Point(x,y))[1] = 0;
                                src.at<cv::Vec3b>(cv::Point(x,y))[2] = 0;

                        }
                }
        }

        return src;
}

//////////////////////////////////////////////////////////////////////
