#include "color_crop.hpp"

#include <set>

#include <opencv2/imgproc/imgproc.hpp>


cv::Mat color_crop::multichannel_thresh(const cv::Mat& input,
		const std::vector< std::array<double, 2> > thresh_params)
{
	/*
	 * split up channels, threshold by range and merge everything together
	 * with a bitwise and. only pixels that pass all thresholds will be white,
	 * all else will be 0;
	 */
	std::vector<cv::Mat> channel(thresh_params.size());
	split(input, channel);
	cv::Mat merged(channel[0].size(), channel[0].type(), cv::Scalar(255));
	for (int i = 0; i < thresh_params.size(); i++)
	{
		/* thresholding pixels in range is achieved by seperately thresholding
		 * pixels that are too low followed by pixels that are too high and
		 * merging everything together
		 */
		cv::Mat tmp0;
		cv::threshold(channel[i], tmp0, thresh_params[i][0], 255,
				cv::THRESH_BINARY);

		cv::Mat tmp1;
		cv::threshold(channel[i], tmp1, thresh_params[i][1], 255,
				cv::THRESH_BINARY_INV);

		if (thresh_params[i][0] > thresh_params[i][1])
		{
			// ORing here seems like the logical thing to do
			cv::bitwise_or(tmp0, tmp1, tmp0);
		}
		else
		{
			cv::bitwise_and(tmp0, tmp1, tmp0);
		}
		cv::bitwise_and(tmp0, merged, merged);
	}

	return merged;
}

/**
 * @returns first in pair is list of contours, second is the contour hierarchy
 * @see cv::findContours()
 *
 * @arg dialation dialates edges after canny edge detection and before finding
 * contours. This is useful for increasing chances of getting longer connected
 * contours. a value of zero results in no dialation. the dialation is
 * elliptical.
 */
std::pair<std::vector< std::vector<cv::Point> >, std::vector<cv::Vec4i> >
		color_crop::image_to_contours(const cv::Mat& img, const int thresh,
				int dilation)
{
	std::pair<std::vector< std::vector<cv::Point> >, std::vector<cv::Vec4i> >
		contours;
	cv::Mat edges;

	cv::Canny(img, edges, thresh, thresh*3, 3);

	/* see: http://docs.opencv.org/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html */
	cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
			cv::Size(2*dilation+1, 2*dilation+1),
			cv::Point(dilation, dilation)); cv::dilate(edges, edges, element);

	cv::findContours(edges, contours.first, contours.second, CV_RETR_TREE,
			CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	return contours;
}

/**
 * @returns a list of which the corresponding index to each contour
 * is true only if the contour has no parent
 *
 * @arg invert inverts the results if true
 */
std::vector<bool> color_crop::filter_out_child_contours(
	const std::pair<std::vector< std::vector<cv::Point> >,
	std::vector<cv::Vec4i> >& contours, bool invert)
{
	std::vector<bool> scores(contours.second.size());
	for (int i = 0; i < contours.second.size(); i++)
	{
		scores[i] = (contours.second[i][3] < 0) != invert;
	}

	return scores;
}

/**
 * @returns a list of which the corresponding index to each contour
 * is true only if the contour has no child
 *
 * @arg invert inverts the results if true
 */
std::vector<bool> color_crop::filter_out_parent_contours(
		const std::pair<std::vector< std::vector<cv::Point> >,
		std::vector<cv::Vec4i> >&contours, bool invert)
{
	std::vector<bool> scores(contours.first.size());
	for (int i = 0; i < contours.first.size(); i++)
	{
		scores[i] = (contours.second[i][2] < 0) != invert;
	}

	return scores;
}

/**
 * should sort your list of contours by area least to greatest
 * @returns a the indices of the contours sorted from least to greatest
 */
std::vector<int> color_crop::contours_sort_by_area(
		const std::vector< std::vector<cv::Point> >& contours)
{
	std::vector<int> indices(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		indices[i] = i;
	}
	std::sort(indices.begin(), indices.end(), [contours](int a,
				int b) {
			return cv::contourArea(contours[a], false) -
			cv::contourArea(contours[b], false);
			});
	return indices;
}

int color_crop::contour_max(const std::vector< std::vector<cv::Point > >&
		contours)
{
	return std::distance(contours.begin(), std::max_element(contours.begin(),
				contours.end(), [](std::vector<cv::Point> a,
					std::vector<cv::Point> b){
				return cv::contourArea(a, false) -
				cv::contourArea(b, false);
				}));
}

std::vector<bool> color_crop::filter_contours_area(
		std::vector< std::vector<cv::Point> >& contours,
		const double min, const double max, bool invert)
{
	std::vector<bool> scores(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		scores[i] = ((cv::contourArea(contours[i], false) > min) &&
			(cv::contourArea(contours[i], false) < max)) != invert;
	}
	return scores;
}

/*
 * consider if you want to convert outer to a convex hull first
 */
double color_crop::contour_ratio_within(const std::vector<cv::Point> inner,
		const std::vector<cv::Point> outer)
{
	unsigned int numwithin = 0;
	for (int i = 0; i < inner.size(); i++)
	{
		if (cv::pointPolygonTest(outer, inner[i], false) > 0)
		{
			numwithin++;
		}
	}
	return ((double)numwithin)/((double)inner.size());
}

/*
 * you can't have a contour that contains 'hollow' sections (eg. you can't
 * have a contour that represents the pool deck without also including the
 * pool).
 * you may want to convert your outer contour to a convex hull to
 * try to remove some artifacts.
 */
void color_crop::filter_contours_within(
	const std::vector< std::vector<cv::Point> >& outer_src,
	std::vector<int>& outer_dst,
	const std::vector< std::vector<cv::Point> >& inner_src,
	std::vector<int>& inner_dst,
	const double thresh)
{
	std::set<int> matched_outer;
	std::set<int> matched_inner;
	for (int i = 0; i < outer_src.size(); i++)
	{
		for (int j = 0; j < inner_src.size(); j++)
		{
			if (contour_ratio_within(inner_src[j], outer_src[i]) > thresh)
			{
				matched_outer.insert(i);
				matched_inner.insert(j);
			}
		}
	}

	outer_dst.resize(matched_outer.size());
	inner_dst.resize(matched_inner.size());
	std::copy(matched_outer.begin(), matched_outer.end(), outer_dst.begin());
	std::copy(matched_inner.begin(), matched_inner.end(), inner_dst.begin());
}

/**
 * every pixel within the contour is set to 255 in the mask, and all others
 * are set to 0
 *
 * @arg mask must be prefilled and of a size and type that it can contain
 * the contours
 */
void color_crop::contour_to_mask(std::vector<cv::Point>& contour, cv::Mat& mask)
{
	std::vector< std::vector<cv::Point> > contour_v = {contour};
	cv::drawContours(mask, contour_v, -1, cv::Scalar(255), CV_FILLED);
}

/*
 * blacks out all points outside contour, points inside contour unchanged
 * this acts on the mat you pass in, so make a copy of it if you need.
 * @arg invert causes all points in contour to be blacked out, others unchanged
 */
void color_crop::black_outside_contour(cv::Mat& img, std::vector<cv::Point> contour, bool invert)
{
	const cv::Vec3b black = {0, 0, 0};
	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			cv::Point point(i, j);
			if ((cv::pointPolygonTest(contour, point, false) <= 0) != invert)
			{
				img.at<cv::Vec3b>(point) = black;
			}
		}
	}
}

/**
 * this is usually not a good tool for color correction.
 * it might be useful for enhancing greyscale images.
 */
void color_crop::enhance_hist(cv::Mat& src, cv::Mat& dst)
{
	std::vector<cv::Mat> channels(src.channels());
	cv::split(src, channels);
	for (int i = 0; i < src.channels(); ++i)
	{
		cv::equalizeHist(channels[i], channels[i]);
	}

	cv::merge(channels, dst);
}

/**
 * just uses opencv normalize functions
 * you should probably use one of the whitebalance functions instead
 */
void color_crop::color_correct_dumb(cv::Mat& src, cv::Mat& dst)
{
	std::vector<cv::Mat> channels(src.channels());
	cv::split(src, channels);

	for (int i = 0; i < channels.size(); i++)
	{
		cv::normalize(channels[i], channels[i], 0, 255, cv::NORM_MINMAX);
		cv::normalize(channels[i], channels[i],
				41200 /* totally arbitrary */, cv::NORM_L2);
	}
	cv::merge(channels, dst);
}

template <typename T>
static inline T trunc(T val, T min, T max)
{
	if (val > min)
	{
		if (val < max)
		{
			return val;
		}
		return max;
	}
	return min;
}

/**
 * the classic auto white balance algorithm
 * the lowest s1'th pixels are set to zero and the highest s2'th are set to 255
 * the remaining values are scaled to stretch from 0 to 255.
 * the goal is for s1 and s2 to be set such that 'outliers' do not stifle the
 * color correction.
 * ~.01 for both s1 and s2 is usually good for normal images, but underwater
 * more agressive values like .05 may be neccessary.
 *
 * this is apparently the method gimp uses for colors->auto->White Balance.
 *
 * on gimp go to colors->info->histogram and look at the red green and blue
 * channels of an image before and after, and you can see the histogram
 * scaling this does.
 * see ipol.im/pub/art/2011/llmps-scb/
 * see http://pi-virtualworld.blogspot.com/2013/02/simple-color-balance-algorithm.html
 * @arg s1 and @arg s2 should be 0 - 1. otherwise dst is just set to src
 * @arg src must single channel 8 bit image
 *
 * note that you can potentially make a white balance that gets better
 * results by allowing some assumptions about the environment. most
 * digital cameras still allow you to selectio WB modes like
 * flourescent, incandescent, outside, et cetera because it helps the
 * camera make assumptions that enhance the white balancing. it
 * especially helps when dealing with images that lack a range of colors
 * and highlights, which completely auto white balancing like this can
 * really mess up.
 */
void color_crop::whitebalance_simple(const cv::Mat& src, cv::Mat& dst,
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
	int	minv = 0; // lowest val in range
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

/**
 * just splits up image channels, runs whitebalance_simple on each
 * and merges channels back together.
 */
void color_crop::whitebalance_simple_wrapper(const cv::Mat& src,
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

/**
 * uses cv::Moments
 * see http://docs.opencv.org/doc/tutorials/imgproc/shapedescriptors/moments/moments.html
 */
cv::Point2f color_crop::contour_centroid(std::vector<cv::Point> contour)
{
	cv::Moments mu = cv::moments(contour, false);
	cv::Point2f centroid(mu.m10/mu.m00, mu.m01/mu.m00);
	return centroid;
}

/**
 * this is probably the least performant way to do this because one bit
 * is being accessed at a time, but the quirks of std::vector<bool> make
 * it hard to write fast and portable ways of doing this.
 *
 * @returns empty vector if @arg a and @arg b are not same size
 */
std::vector<bool> color_crop::list_and(const std::vector<bool>& a, const std::vector<bool>& b)
{
	if (a.size() != b.size())
	{
		return std::vector<bool>(0);
	}

	std::vector<bool> out(a.size());
	for (int i = 0; i < out.size(); i++)
	{
		out[i] = a[i] && b[i];
	}
	return out;
}

std::vector<int> color_crop::contour_children(
		std::vector<cv::Vec4i> hierarchy, int index)
{
	std::vector<int> children;
	int child = hierarchy[index][2];
	if (child < 0)
	{
		return children;
	}

	for (int rightchild = hierarchy[child][0]; rightchild > 0;
			rightchild = hierarchy[rightchild][0])
	{
		children.push_back(rightchild);
	}

	for (int leftchild = hierarchy[child][1]; leftchild > 0;
			leftchild = hierarchy[leftchild][1])
	{
		children.push_back(leftchild);
	}

	return children;
}

void color_crop::contours_to_convexhulls(
		const std::vector< std::vector<cv::Point> >& src,
		std::vector< std::vector<cv::Point> >& dst)
{
	dst.resize(src.size());
	for (int i = 0; i < src.size(); i++)
	{
		cv::convexHull(src[i], dst[i]);
	}
}

std::vector<bool> color_crop::filter_out_enclosing_contours(
		const std::vector< std::vector<cv::Point> >& contours, double thresh)
{
	std::vector<bool> scores(contours.size());
	// probably a better way to do this
	for (int i = 0; i < scores.size(); i++)
	{
		scores[i] = true;
	}

	for (int i = 0; i < contours.size(); i++)
	{
		for (int j = i + 1; j != i;)
		{
			if (j >= contours.size())
			{
				j = 0;
				continue;
			}
			scores[i] = scores[i] & (contour_ratio_within(contours[j], contours[i])
				< thresh);
			j++;
		}
	}
	return scores;
}

std::vector<bool> color_crop::indices_to_scores(
		std::vector<int> indices, size_t size, bool invert)
{
	std::vector<bool> scores(size);
	for (int i = 0; i < scores.size(); i++)
	{
		scores[i] = invert;
	}

	for (int i = 0; i < indices.size(); i++)
	{
		scores[indices[i]] = !invert;
	}

	return scores;
}

std::vector< std::pair<int, int> > color_crop::contours_find_intersections(
		std::vector< std::vector<cv::Point> > contours, double thresh)
{
	std::vector< std::pair<int, int> > intersects;
	for (int i = 0; i < contours.size(); i++)
		for (int j = i + 1; j < contours.size(); j++)
		{
			if ((contour_ratio_within(contours[i], contours[j]) > thresh) &&
					(contour_ratio_within(contours[j], contours[i]) > thresh))
			{
				intersects.push_back(std::pair<int, int>(i, j));
			}
		}
	return intersects;
}

/**
 * @returns false for the corresponding indice of every contour that
 * was smaller in an intersection at least once
 */
std::vector<bool> color_crop::contour_pair_area_smaller(
		std::vector< std::pair<int,int> > pairs,
		std::vector< std::vector<cv::Point> > contours,
		bool invert)
{
	std::vector<bool> smalls(contours.size());
	for (int i = 0; i < smalls.size(); i++)
	{
		smalls[i] = true;
	}
	for (int i = 0; i < pairs.size(); i++)
	{
		if (cv::contourArea(contours[pairs[i].first], false) <
				cv::contourArea(contours[pairs[i].second], false) != invert)
		{
			smalls[pairs[i].first] = false;
		}
		else
		{
			smalls[pairs[i].second] = false;
		}
	}
	return smalls;
}

/*
 * i think this may be messed up...i dont really know how
 * hu invariants work
 */
std::vector<bool> color_crop::filter_shapes(
		std::vector<cv::Point> sample,
		std::vector< std::vector<cv::Point> > inputs,
		double thresh, bool invert)
{
	std::vector<bool> scores(inputs.size());
	for (int i = 0; i < inputs.size(); i++)
	{
		scores[i] = cv::matchShapes(sample, inputs[i], CV_CONTOURS_MATCH_I1,
				0.0) < thresh;
	}
	return scores;
}

cv::Mat color_crop::draw_contours(
		std::vector< std::vector<cv::Point> > contours,
		const cv::Mat& img_bgr)
{
	cv::Mat img = img_bgr.clone();
	cv::RNG rng(12345);

	// see
	// http://docs.opencv.org/doc/tutorials/imgproc/shapedescriptors/find_contours/find_contours.html
	for (int i = 0; i < contours.size(); i++)
	{
		cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		cv::drawContours(img, contours, i, color, 2, 8);
	}

	return img;
}

/**
 * the area of the contour over the area of a circle with the perimeter
 * of the contour
 */
double color_crop::circularity(
		const std::vector<cv::Point>& contour)
{
	double radius = cv::arcLength(contour, true)/(2*3.14159);
	return cv::contourArea(contour)/(3.14159*radius*radius);
}

std::vector<bool> color_crop::filter_circularity(
		const std::vector< std::vector<cv::Point> >& contours,
		const double thresh)
{
	std::vector<bool> scores(contours.size());
	for (int i = 0; i < contours.size(); ++i)
	{
		scores[i] = circularity(contours[i]) > thresh;
	}
	return scores;
}

int color_crop::integrate(cv::Mat src, int min, int max)
{
	int integrand = 0;
	unsigned char *data = src.ptr();
	const int numpix = src.cols*src.rows;
	for (int i = 0; i < numpix; ++i)
	{
		if ((data[i] >= min) && (data[i] <= max))
		{
			integrand += data[i];
		}
	}
	return integrand;
}
