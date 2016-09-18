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
			res.at<float>(row, col) = f(img.at<cv::Vec3b>(row, col)[2] / 256.f, img.at<cv::Vec3b>(row, col)[1] / 256.f, img.at<cv::Vec3b>(row, col)[0] / 256.f);

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

// assume the NeuralNetwork object is set up correctly (3 inputs, 1 output)
std::function<float(float, float, float)> nnFilter(const NeuralNetwork& nn)
{
	return [=](float r, float g, float b)
	{
		const Matrix inputs = {{r}, {g}, {b}};

		auto outputs = nn.apply(inputs);

		return outputs.get(0);
	};
}

