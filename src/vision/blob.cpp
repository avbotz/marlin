#include "vision/blob.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/* 
 * Constructor for blobs using a vector of points
 */
Blob::Blob(int color, int channels, std::vector<cv::Point> blobPoints, std::vector<cv::Point> perimeterPoints, cv::Mat src) :
	num_channels(channels), moment{}, moment_x{}, moment_y{}
{
	memset(moment, 0, channels*sizeof(long));
	memset(moment_x, 0, channels*sizeof(long));
	memset(moment_y, 0, channels*sizeof(long));

	min_x = blobPoints[0].x;
	max_x = blobPoints[0].x+1;
	min_y = blobPoints[0].y;
	max_y = blobPoints[0].y+1;
	
	for (unsigned int i = 1; i < blobPoints.size(); i++)
	{
		if (blobPoints[i].x < min_x)
			min_x = blobPoints[i].x;
		if (blobPoints[i].x > max_x)
			max_x = blobPoints[i].x;
		if (blobPoints[i].y < min_y)
			min_y = blobPoints[i].y;
		if (blobPoints[i].y > max_y)
			max_y = blobPoints[i].y;
	}
	
	area = blobPoints.size();
	perimeter = perimeterPoints.size();
	
	bounding_box_fill = (float)area / ((max_x - min_x)*(max_y - min_y));

	for (unsigned int i = 0; i < area; i++)
	{
		int pixel = src.at<unsigned char>(blobPoints[i]);
		moment[color] += pixel;
		moment_x[color] += (pixel * blobPoints[i].y);
		moment_y[color] += (pixel * blobPoints[i].x);
	}

	calculateConvexHull(perimeterPoints);
}


/*
 * Constructor of blob using two existing blobs
 */
Blob::Blob(Blob* a, Blob* b, int channels) :
	num_channels(channels), moment{}, moment_x{}, moment_y{}
{
	// calculate the new bounding box
	min_x = MIN(a->min_x, b->min_x);
	max_x = MAX(a->max_x, b->max_x);
	min_y = MIN(a->min_y, b->min_y);
	max_y = MAX(a->max_y, b->max_y);
	
	
	// calculate the new moments
	for (int i = 0; i < channels; i++)
	{
		moment_x[i] = a->moment_x[i] + b->moment_x[i];
		moment_y[i] = a->moment_y[i] + b->moment_y[i];
		moment[i] = a->moment[i] + b->moment[i];
	}
	
	area = a->area + b->area;
	
	// compute the area of the intersection
	int int_min_x = MAX(a->min_x, b->min_x);
	int int_max_x = MIN(a->max_x, b->max_x);
	int int_min_y = MAX(a->min_y, b->min_y);
	int int_max_y = MIN(a->max_y, b->max_y);
	int int_area = (int_max_x - int_min_x)*(int_max_y - int_min_y);
	
	area = a->area + b->area - (a->bounding_box_fill + b->bounding_box_fill)*int_area/2;
	bounding_box_fill = ((float)area) / ((max_x - min_x)*(max_y - min_y));

	std::vector<cv::Point> perimeterPoints;
	perimeterPoints.insert(perimeterPoints.end(), a->convexHull.begin(), a->convexHull.end());
	perimeterPoints.insert(perimeterPoints.end(), b->convexHull.begin(), b->convexHull.end());
	calculateConvexHull(perimeterPoints);
	
	perimeter = (a->perimeter + b->perimeter) * getConvexHullPerimeter() / (a->getConvexHullPerimeter() + b->getConvexHullPerimeter());
}

/*
 * Copy constructor of blob
 */
Blob::Blob(const Blob &b) :
	min_x(b.min_x), max_x(b.max_x), min_y(b.min_y), max_y(b.max_y),
	area(b.area), bounding_box_fill(b.bounding_box_fill), num_channels(b.num_channels), 
	moment{}, moment_x{}, moment_y{},
	convexHull(b.convexHull), perimeter(b.perimeter)
{
	for (int i = 0; i < num_channels; i++)
	{
		moment[i] = b.moment[i];
		moment_x[i] = b.moment_x[i];
		moment_y[i] = b.moment_y[i];
	}
}

Blob::~Blob()
{
}

/*
 * Finds the channel with the highest average value, and returns as the color
 */
int Blob::getColor()
{
	int color = 0;
	for (int i = 0; i < num_channels; i++)
	{
		if (moment[i] > moment[color])
		{
			color = i;
		}
	}
	return color;
}

cv::Point Blob::getCentroid(int color)
{
	return cv::Point(moment_y[color]/moment[color], moment_x[color]/moment[color]);
}

cv::Point Blob::getCentroid()
{
	int color = getColor();
	return cv::Point(moment_y[color]/moment[color], moment_x[color]/moment[color]);
}

float Blob::getDensity(int channel)
{
	if (area == 0) 
		return 0;
	return moment[channel]/(float)area;
}

float Blob::getDensity()
{
	return moment[getColor()]/(float)area;
}

bool Blob::overlapsWith(Blob* b)
{
	//check bounding boxes
	if (max_x < b->min_x) return false;
	if (max_y < b->min_y) return false;
	if (min_x > b->max_x) return false;
	if (min_y > b->max_y) return false;
	
	return true;
}

void Blob::addToThis(Blob* b)
{
	
	min_x = MIN(min_x, b->min_x);
	max_x = MAX(max_x, b->max_x);
	min_y = MIN(min_y, b->min_y);
	max_y = MAX(max_y, b->max_y);
	
	
	// calculate the new moments
	for (int i = 0; i < num_channels; i++)
	{
		moment_x[i] += b->moment_x[i];
		moment_y[i] += b->moment_y[i];
		moment[i]   += b->moment[i];
	}
	
	return;
}

void Blob::scale(int n)
{
	for (int i = 0; i < num_channels; i++)
	{
		moment_x[i] *= n;
		moment_y[i] *= n;
	}
	min_x *= n;
	min_y *= n;
	max_x *= n;
	max_y *= n;
	
	return;
}

void Blob::drawBlob(cv::Mat img, cv::Scalar color)
{
	cv::circle(img, getCentroid(getColor()), 2, color, CV_FILLED);
	char intensity_str[32];
	sprintf(intensity_str, "%ld %u %f", moment[getColor()], area, getHullCircularity());
//	cv::putText(img, intensity_str, cv::Point(min_x, min_y), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, color, 1, CV_AA);
	
	if (convexHull.size() > 1)
	{
		for(int i = 0; i < convexHull.size() - 1; i++)
		{
			cv::line(img, convexHull[i], convexHull[i+1], color);
		}
		cv::line(img, convexHull[convexHull.size()-1], convexHull[0], color);
	}
}

/*
 * See http://geomalgorithms.com/a10-_hull-1.html for details.
 */
void Blob::calculateConvexHull(std::vector<cv::Point> points)
{
	auto dot = [](cv::Point a, cv::Point b)->int{return a.x*b.x + a.y*b.y;};
	if (points.size() < 4)
	{
		convexHull.insert(convexHull.begin(), points.begin(), points.end());
		return;
	}
	
	std::sort(points.begin(), points.end(), [](cv::Point a, cv::Point b)->bool
		{
			if (a.x < b.x)
				return true;
			else if (a.x == b.x)
				return (a.y < b.y);
			else //a.x > b.x
				return false;
		});
	
	cv::Point P_minmin, P_minmax, P_maxmin, P_maxmax;
	int i_minmax, i_maxmin;
	
	int i;
	P_minmin = points[0];
	for (i = 0; i < points.size() && points[i].x == points[0].x; i++);
	i_minmax = i-1;
	P_minmax = points[i_minmax];
	if (i+1 == points.size())
	{
		// ran out of points
		convexHull.push_back(P_minmin);
		convexHull.push_back(P_minmax);
		return;
	}
	
	P_maxmax = points[points.size() - 1];
	for (i = points.size() - 1; i >= 0 && points[i].x == points[points.size() - 1].x; i--);
	i_maxmin = i+1;
	P_maxmin = points[i_maxmin];
	
	// create the chain above L_min
	// first convert the line to hyperplane form, then dot it with the point to find the distance from the hyperplane
	// http://www.gamedev.net/topic/542870-determine-which-side-of-a-line-a-point-is/
	cv::Point L_min = P_maxmin - P_minmin;
	L_min = cv::Point(-L_min.y, L_min.x);

	convexHull.push_back(P_minmin);
	int num_in_upper_stack = 1;
	
	for (i = i_minmax + 1; i <= i_maxmin; i++)
	{
		cv::Point current = points[i];
		if (dot(current - P_minmin, L_min) >= 0 && i != i_maxmin)
		{
			// below or on the L_min line
			// so ignore you
			continue;
		}
		bool hasPointBeenPushed = false;
		
		while (!hasPointBeenPushed)
		{
			if (num_in_upper_stack == 1)
			{
				convexHull.push_back(current);
				hasPointBeenPushed = true;
				num_in_upper_stack++;
				continue;
			}
			else
			{
				// get the vector perpendicular to the vector created by the last 2 points in the stack
				cv::Point perp = convexHull[convexHull.size()-1] - convexHull[convexHull.size()-2];
				perp = cv::Point(-perp.y, perp.x);
				
				if (dot(current - convexHull[convexHull.size()-2], perp) > 0)
				{
					// current is strictly left of the vector created by the last 2 points
					convexHull.push_back(current);
					hasPointBeenPushed = true;
					num_in_upper_stack++;
				}
				else
				{
					// the last point in the stack is not part of the convex hull, so remove it
					convexHull.pop_back();
					num_in_upper_stack--;
				}
			}
		}
	}
	
	// now we've finished determining the upper hull
	
	cv::Point L_max = P_minmax - P_maxmax;
	L_max = cv::Point(-L_max.y, L_max.x);
	
	if (P_maxmin != P_maxmax)
	{
		convexHull.push_back(P_maxmax);
	}
	
	int num_in_lower_stack = 1;
	
	for (int i = i_maxmin-1; i >= i_minmax; i--)
	{
		cv::Point current = points[i];
		if (dot(current - P_maxmax, L_max) >= 0 && i != i_minmax)
		{
			// below or on the L_max line
			// so ignore you
			continue;
		}
		bool hasPointBeenPushed = false;
		
		while (!hasPointBeenPushed)
		{
			if (num_in_lower_stack == 1)
			{
				convexHull.push_back(current);
				hasPointBeenPushed = true;
				num_in_lower_stack++;
				continue;
			}
			else
			{
				// get the vector perpendicular to the vector created by the last 2 points in the stack
				cv::Point perp = convexHull[convexHull.size()-1] - convexHull[convexHull.size()-2];
				perp = cv::Point(-perp.y, perp.x);
				
				if (dot(current - convexHull[convexHull.size()-2], perp) > 0)
				{
					// current is strictly left of the vector created by the last 2 points
					convexHull.push_back(current);
					hasPointBeenPushed = true;
					num_in_lower_stack++;
				}
				else
				{
					// the last point in the stack is not part of the convex hull, so remove it
					convexHull.pop_back();
					num_in_lower_stack--;
				}
			}
		}
	}
	
	if (P_minmin == P_minmax)
	{
		// don't double count the start/end value
		convexHull.erase(convexHull.end()-1);
	}
}

cv::Vec<float, 2> Blob::unitVector(cv::Vec<float, 2> v)
{
	cv::Vec<float, 2> unitVector;
	float mag = sqrt(v[0]*v[0] + v[1]*v[1]);
	
	return cv::Vec<float, 2>(v[0]/mag, v[1]/mag);
}

std::vector<cv::Vec<float, 2> > Blob::rotatePoints(std::vector<cv::Vec<float, 2> > points, cv::Vec<float, 2> unitVec, cv::Vec<float, 2> orthoVec)
{	
	for(int i = 0; i < points.size(); i++)
	{
		points[i] = cv::Vec<float, 2>(dot(unitVec, points[i]), dot(orthoVec, points[i]));
	}
	return points;
}

int Blob::findNextSide(std::vector<cv::Vec<float, 2> >points, int sidesIndeces[4])
{
	float nextSlopes[4];
	
	// calculate the effective slopes for each of the next possible sides to rotate to
	for (int i = 0; i < 4; i++)
	{
		cv::Vec<float, 2> init = points[sidesIndeces[i]];
		cv::Vec<float, 2> term = points[(sidesIndeces[i]+1)%points.size()];
		
		if (i%2 == 0)   // we are working with a side on min/max x, use inverse slope
		{
			if (term[1] != init[1])
				nextSlopes[i] = fabs((term[0] - init[0])/(term[1] - init[1]));
			else
				nextSlopes[i] = INFINITY;
		}

		else
		{
			if (term[0] != init[0])
				nextSlopes[i] = fabs((term[1] - init[1])/(term[0] - init[0]));
			else
				nextSlopes[i] = INFINITY;
		}
	}
	
	// find the slope that is smallest, advance all points with this slope
	int smallest_index;
	std::vector<int> matches;
    for (int i = 0; i < 4; i++)
    {
        if (nextSlopes[i] != 0.0f)
        {
            smallest_index = i;
            break;
        }
    }
    
	for (int i = smallest_index+1; i < 4; i++)
	{
		if (nextSlopes[i] / nextSlopes[smallest_index] > .999f && nextSlopes[i] / nextSlopes[smallest_index] < 1.001f)
		{
			matches.push_back(i);
		}
		else if (nextSlopes[i] < nextSlopes[smallest_index] && nextSlopes[i] != 0.0f)
		{
			smallest_index = i;
			matches.erase(matches.begin(), matches.end());
		}
	}
	
	// actually advance the sides
	sidesIndeces[smallest_index] = (sidesIndeces[smallest_index] + 1) % points.size();
	for (int i = 0; i < matches.size(); i++)
	{
		sidesIndeces[matches[i]] = (sidesIndeces[matches[i]] + 1) % points.size();
	}
	
	// those correspond to a side of your new rectangle!!!
	return smallest_index;
}

cv::RotatedRect Blob::minSurroundingRect()
{
	// implemented naively
	// should be converted to the rotating calipers method
	
	std::vector<cv::Vec<float, 2> > pts;
	// copy the point array to a vector array of floats
	for (int i = 0; i < convexHull.size(); i++)
	{
		pts.push_back(cv::Vec<float, 2>(convexHull[i].x, convexHull[i].y));
	}
	
	float minRectArea;
	float minRectminX, minRectminY, minRectmaxX, minRectmaxY;
    
	float rectangleSides[4] = {pts[0][0], pts[0][1], pts[0][0], pts[0][1]};
	int sideIndeces[4] = {0,0,0,0};
	
	// make the initial rectangle
	for (int i = 1; i < pts.size(); i++)
	{
		cv::Vec<float, 2> v = pts[i];
		if (v[0] <= rectangleSides[0])
		{
			rectangleSides[0] = v[0];
			sideIndeces[0] = i;
		}
		if (v[1] <= rectangleSides[1])
		{
			rectangleSides[1] = v[1];
			sideIndeces[1] = i;
		}
		if (v[0] >= rectangleSides[2])
		{
			rectangleSides[2] = v[0];
			sideIndeces[2] = i;
		}
		
		if (v[1] >= rectangleSides[3])
		{
			rectangleSides[3] = v[1];
			sideIndeces[3] = i;
		}
	}
	
	// we need the most clockwise points at the extremes, so check if the first index is extremer than the max
	for (int i = 0; i < 4; i++)
	{
		if (pts[0][i%2] == rectangleSides[i] && sideIndeces[i] == pts.size()-1)
		{
			sideIndeces[i] = 0;
		}
	}
	
	// check if this could be a minimum area rectangle (if it shares a side with the hull)
	bool isValid = false;
	int minEdgeI, minEdgeT;
	for (int i = 0; i < pts.size(); i++)
	{
		int i_next = (i+1)%pts.size();
		for (int j = 0; j < 4; j++)
		{
			if (pts[i][j%2] == rectangleSides[j] && pts[i_next][j%2] == rectangleSides[j])
            {
				isValid = true;
                minEdgeI = i;
                minEdgeT = i_next;
            }
		}
	}
	
	minRectArea = (rectangleSides[2] - rectangleSides[0])*(rectangleSides[3] - rectangleSides[1]);
	
	minRectminX = rectangleSides[0];
	minRectminY = rectangleSides[1];
	minRectmaxX = rectangleSides[2];
	minRectmaxY = rectangleSides[3];
    
	float caliperRotation = 0;
	for(int i = 0; i < convexHull.size(); i++)
	{
		int changedEdge = findNextSide(pts, sideIndeces);
        
        int nextEdge;
        if (sideIndeces[changedEdge] > 0)
            nextEdge = sideIndeces[changedEdge]-1;
        else
            nextEdge = pts.size()-1;    //the last side
        cv::Vec<float, 2> unitVec(0.0f, 0.0f), orthoVec(0.0f, 0.0f);
        cv::Vec<float, 2> nextEdgeI = pts[nextEdge];
        cv::Vec<float, 2> nextEdgeT = pts[(nextEdge+1)%pts.size()];
        switch (changedEdge)
        {
            case 0:
                orthoVec = unitVector(cv::Vec<float, 2>(nextEdgeI[0] - nextEdgeT[0], nextEdgeI[1] - nextEdgeT[1]));
                unitVec = cv::Vec<float, 2>(orthoVec[1], -orthoVec[0]);
                break;
            case 1:
                unitVec = unitVector(cv::Vec<float, 2>(nextEdgeT[0] - nextEdgeI[0], nextEdgeT[1] - nextEdgeI[1]));
                orthoVec = cv::Vec<float, 2>(-unitVec[1], unitVec[0]);
            case 2:
                orthoVec = unitVector(cv::Vec<float, 2>(nextEdgeT[0] - nextEdgeI[0], nextEdgeT[1] - nextEdgeI[1]));
                unitVec = cv::Vec<float, 2>(orthoVec[1], -orthoVec[0]);
            case 3:
                unitVec = unitVector(cv::Vec<float, 2>(nextEdgeI[0] - nextEdgeT[0], nextEdgeI[1] - nextEdgeT[1]));
                orthoVec = cv::Vec<float, 2>(-unitVec[1], unitVec[0]);
        }
        
		float rotation = fabs(atan2f(unitVec[1], unitVec[0]));
		pts = rotatePoints(pts, unitVec, orthoVec);
		rectangleSides[0] = pts[sideIndeces[0]][0];
		rectangleSides[1] = pts[sideIndeces[1]][1];
		rectangleSides[2] = pts[sideIndeces[2]][0];
		rectangleSides[3] = pts[sideIndeces[3]][1];
		
		float area = (rectangleSides[2] - rectangleSides[0])*(rectangleSides[3] - rectangleSides[1]);
        
		if(area < minRectArea)
		{
			minRectArea = area;
			minRectminX = rectangleSides[0];
			minRectminY = rectangleSides[1];
			minRectmaxX = rectangleSides[2];
			minRectmaxY = rectangleSides[3];
			minEdgeI = nextEdge;
			minEdgeT = (nextEdge+1)%pts.size();
		}
		caliperRotation += rotation;
		if (caliperRotation > CV_PI/2 - 0.0001)
			break;
	}
	
	// find the unit vector of side
	cv::Vec<float, 2> finalUnitVector = unitVector(cv::Vec<float,2>(convexHull[minEdgeT].x-convexHull[minEdgeI].x,
									convexHull[minEdgeT].y-convexHull[minEdgeI].y));
	
	// get orthoganal vector
	cv::Vec<float, 2> finalOrthoVector(finalUnitVector[1], -finalUnitVector[0]);
	
	// create matrix
	cv::Mat M(2, 2, CV_32F);
	M.at<float>(0, 0) = finalUnitVector[0];
	M.at<float>(0, 1) = finalUnitVector[1];
	M.at<float>(1, 0) = finalOrthoVector[0];
	M.at<float>(1, 1) = finalOrthoVector[1];
	
	// find inverse of M
	M.inv();
	
	// find corner points of bounding box
	std::vector<cv::Vec<float, 2> > boundingBoxCorners;
	boundingBoxCorners.push_back(cv::Vec<float,2>(minRectminX, minRectminY));
	boundingBoxCorners.push_back(cv::Vec<float,2>(minRectmaxX, minRectminY));
	boundingBoxCorners.push_back(cv::Vec<float,2>(minRectmaxX, minRectmaxY));
	boundingBoxCorners.push_back(cv::Vec<float,2>(minRectminX, minRectmaxY));
	
	// find points
	std::vector<cv::Vec<float, 2> > finalCorners;
	cv::Vec<float, 2> vector_x(M.at<float>(0, 0), M.at<float>(0, 1));
	cv::Vec<float, 2> vector_y(M.at<float>(1, 0), M.at<float>(1, 1));
	for(int i = 0; i < boundingBoxCorners.size(); i++)
	{
		finalCorners.push_back(cv::Vec<float, 2>(dot(boundingBoxCorners[i], vector_x), dot(boundingBoxCorners[i], vector_y)));
	}
    
    cv::Point2f center;
    center.x = (finalCorners[0][0] + finalCorners[2][0])/2;
    center.y = (finalCorners[0][1] + finalCorners[2][1])/2;
	
    cv::Size2f size(minRectmaxX - minRectminX, minRectmaxY - minRectminY);
    
    float angle = atan2f(finalCorners[1][0] - finalCorners[0][0], finalCorners[1][1] - finalCorners[0][1]) * 180/CV_PI;
    
    if (angle > 90)
        angle -= 180;
    
    else if (angle < -90)
        angle += 180;
    
	if (angle == 45.0f || angle == -45.0f)
		angle = 0;
	
	return cv::RotatedRect(center, size, angle);
}


float Blob::dot(cv::Vec<float, 2> a, cv::Vec<float, 2> b)
{
	return a[0]*b[0] + a[1]*b[1];
}

float Blob::magnitude(float x, float y)
{
	return (float)sqrt(x*x + y*y);
}

float Blob::getConvexHullPerimeter()
{
	float p = 0;
	if (convexHull.size() > 1)
	{
		for(int i = 0; i < convexHull.size() - 1; i++)
		{
			p += magnitude(convexHull[i].x - convexHull[i+1].x, convexHull[i].y - convexHull[i+1].y);
		}
		p += magnitude(convexHull[convexHull.size()-1].x - convexHull[0].x, convexHull[convexHull.size()-1].y - convexHull[0].y);
	}
	return p;
}

float Blob::getConvexHullArea()
{
	if (convexHull.size() == 1 || convexHull.size() == 2)
	{
		return 0;
	}
	int area_pos = 0, area_neg = 0;
	for (int i = 0; i < convexHull.size(); i++)
	{
		area_pos += convexHull[i].x * convexHull[(i+1)%convexHull.size()].y;
		area_neg += convexHull[i].y * convexHull[(i+1)%convexHull.size()].x;
	}
	
	return std::abs((area_pos - area_neg)/2.0f);
}


float Blob::getHullCircularity()
{
	// compares the ratio of area to perimeter to that of a circle (r/2)
	// radius is considered to be the average of the width and height
	float area = getConvexHullArea();
	float perimeter = getConvexHullPerimeter();
	float radius = (((max_x - min_x) + (max_y - min_y)) / 4.0f);
	if (radius == 0 || perimeter == 0)
	{
		return 0;
	}
		
	return (area/perimeter) / (radius / 2.0f);
}
