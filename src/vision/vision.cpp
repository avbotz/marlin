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

/////////////////////////////////////////////////////////
/////seth's whitebalance
/////funky but cool
void whitebalance_simple(const cv::Mat& src, cv::Mat& dst,
                float s1, float s2, int outmin, int outmax)
{
        if ((s1 > 1) || (s2 > 1))
        {
                dst = src;
                return;
        }

        int hist[256] = {0};
        const int numpix = src.rows*src.cols;
        // make histogram of pixel vals
        const unsigned char *srcdata = src.ptr();
        for (int i = 0; i < numpix; ++i)
        {
                ++hist[(int)(srcdata[i])];
        }
        
        // find lowest val in range
        int     minv = 0; // lowest val in range
        {
                const int n1 = s1*numpix; // num pixels out of range on low end
                for (int num = 0; num < n1;)
                {
                        num += hist[minv];
                        ++minv;
                }
        }
        // find higest val in range
        int maxv = 255; // higest val in range
        {
                const int n2 = s2*numpix; // num pixels out of range on high end
                for (int num = 0; num < n2;)
                {
                        num += hist[maxv];
                        --maxv;
                }
        }

        // scale vals
        const float scale = ((float)(outmax - outmin))/((float)(maxv - minv));
        cv::Mat tmp(src.rows, src.cols, src.type());
        unsigned char *dstdata = tmp.ptr();
        for (int i = 0; i < numpix; ++i)
        {
                dstdata[i] = trunc((int)(scale*(srcdata[i] - minv) + outmin), outmin, outmax);
        }
        dst = tmp;
}

/////////////////////////////////////////////////////////
void whitebalance_simple_wrapper(const cv::Mat& src,
                cv::Mat& dst,
float s1, float s2, int outmin, int outmax)
{
        const int numchannels = 3;
        cv::Mat channels[numchannels];
        cv::split(src, channels);
        for (int i = 0; i < numchannels; ++i)
        {
                whitebalance_simple(channels[i], channels[i], s1, s2, outmin, outmax);
        }
        cv::merge(channels, numchannels, dst);
}

