#include <iostream>
#include <vector>

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

Mat balancePurple(const Mat& src)
{
    assert(src.type() == CV_8UC3);
    Mat purple = src.clone();
    Mat filteredPurple;
    
    
    for(int y=0; y<purple.rows; y++)
    {
        for(int x=0; x<purple.cols; x++)
        {
            Vec3b color = purple.at<Vec3b>(Point(x,y));
            if(color[2] > 100 || color[1] > 100 || (color[1] > color[2]) || color[0] < 50 || (abs(color[2] - color[1]))<10)
            {
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
                purple.at<Vec3b>(Point(x,y)) = color;
            }
        }
    }
 
    
    cv::GaussianBlur(purple, purple, cv::Size(35, 35), 0);

    return purple;
}
 

int main(int argc, char** argv)
{
    Mat input = imread(argv[1], 1);
    imshow("Original", input);
    
    vector<Mat> imageChannels;
    split(input, imageChannels);
    
    for (int i = 0; i < 3; i++) {
        equalizeHist(imageChannels[i], imageChannels[i]);
    }
    
    merge(imageChannels, input);
    imshow("Post color balance!", input);
    Mat filteredPurple = balancePurple(input);
    imshow("Balanced Purple", filteredPurple);
    
    vector<Mat> finalChannels;
    split(filteredPurple, finalChannels);
    
    Mat finalGrayscale = finalChannels[0];
    imshow("Final Grayscale!", finalGrayscale);
    
    double purpleMin;
    double purpleMax;
    Point purpleMinLoc;
    Point purpleMaxLoc;
    minMaxLoc(finalGrayscale, &purpleMin, &purpleMax, &purpleMinLoc, &purpleMaxLoc);
    circle(finalGrayscale, purpleMaxLoc, 10, Scalar(255, 255, 255), 2, 8, 0);
    imshow("Grayscale", finalGrayscale);
    
    waitKey();
    return 0;
}
