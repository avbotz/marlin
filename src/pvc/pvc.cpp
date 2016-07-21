#include <vector>
#include <algorithm>
#include <functional>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cv.h>

#include "common/defs.hpp"
#include "common/config.hpp"
#include "vision/config.hpp"
#include "vision/vision.hpp"
#include "image/image.hpp"

const float pvcWidth = 2;
const float minDist = 8;
const float scalex = 0.4;
const float scaley = 0.4;
const float cutoff = 0.8;

std::vector<cv::Point2f> flatten(cv::Mat& img)
{
	std::vector<cv::Point2f> points;

	float* ptr = img.ptr<float>();
	int i = 1;

	for(int c = 0; c < img.cols; c++)
	{
		double sum = 0;
		for(int r = 0; r < img.rows; r++)
		{
			sum += ptr[r*img.cols + c];
		}
		points.push_back(cv::Point2f(i, sum/img.rows));
		i++;
	}

	return points;
}

/*
auto yfilter = nnFilter(
{
	{
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},
	{
		{0},
		{0},
		{0},
		{0},
	},
	{{0, 0, 0, 0}},
	{{0}},
});
*/

float yfilter(float r, float g, float b)
{
	return (g - b) * 5 + r;
}

bool comparePoints(cv::Point2f& a, cv::Point2f& b)
{
	return a.y > b.y;
}

std::vector<cv::Point2f> bestPoints(std::vector<cv::Point2f> points, int num)
{
	std::vector<cv::Point2f> top;
	std::sort(points.begin(), points.end(), comparePoints);

	top.push_back(points[0]);
	int j = 1;
	while(top.size() < num)
	{
		bool fail = false;
		for(int k = 0; k < top.size(); k++)
		{
			if(points[j].x - top[k].x < minDist && points[j].x - top[k].x > -minDist)
			{
				j++; fail = true;
				if(j >= points.size()) return top;
				break;
			}
		}
		if(!fail) {top.push_back(points[j]); j++;}
	}
	
	return top;
}

cv::Mat dispPoints(cv::Mat img, std::vector<cv::Point2f> pts, int cwidth)
{
	cv::Mat disp(img);
	for(cv::Point2f p : pts)
	{
		cv::ellipse(disp, cv::Point(p.x, cwidth), cv::Size(cwidth, cwidth), 0, 0, 360, cv::Scalar(255, 255, 255));
	}
	return disp;
}

void pResults(FILE* in, FILE* out, std::vector<cv::Point2f>& pts, int cols)
{
	if(pts.size() != 2 || pts[1].y < cutoff*pts[0].y) {fprintf(out, "0\n"); return;}

	fprintf(out, "1\n");

	float theta = fhFOV * (pts[0].x + pts[1].x - cols) / (2*cols);
	float dist = pvcWidth/2 / std::tan((std::max(pts[0].x - pts[1].x, pts[1].x - pts[0].x))/cols * fhFOV / 2 * 2*M_PI);

	fprintf(out, "%i %i -2\n%f 0 %f\n",
		M_PVC_X, M_PVC_Y,
		theta, dist);
	fflush(out);
}

int main(int argc, char** argv)
{
	FILE* in = stdin;
	FILE* out = stdout;
	FILE* log = stderr;

	while(true)
	{
		cv::Mat img = imageRead(in); //Read image

		cv::resize(img, img, cv::Size(img.cols*scalex, img.rows*scaley));

		cv::Mat yelo = filter(img, yfilter); //Enhance to make pvc show up

		cv::Mat diff = generateDiffMap(yelo, minDist/2, true);

		cv::Mat scaled = scaleIntensity(diff);
		
		std::vector<cv::Point2f> pts = bestPoints(flatten(scaled), 2);
		
		pResults(in, out, pts, img.cols);

		imageWrite(log, dispPoints(img, pts, 5));
	}
}
