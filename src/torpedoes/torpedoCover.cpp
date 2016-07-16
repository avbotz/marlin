/*
 * Starting funtion is cv::Point torpedoCover(cv::Mat)
 */

#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

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
    //std::cout << orangeMax << " " << orangeMaxLoc << std::endl;
    //cv::circle(finalImage, orangeMaxLoc, 20, cv::Scalar(255, 255, 255), 2, 8, 0);
    //cv::imshow("Final Image!", finalImage);

    return orangeMaxLoc;
}

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
    cv::merge(imageChannels, balancedImage);

    //Set image height and width for pixel loop
    const int imageWidth = balancedImage.cols;
    const int imageHeight = balancedImage.rows;

    //Filter red with formula
    maxOrangePoint = balanceOrange(balancedImage, imageWidth, imageHeight);

    cv::waitKey(0);
    return maxOrangePoint;
}


