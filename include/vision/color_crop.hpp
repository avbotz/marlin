#include <cv.h>
#include <array>

class color_crop
{
	public:
		/**
		 * for each channel of an image, sets all pixels within the range
		 * to 255, and all outside of range to 0. then does a bitwise and
		 * of all channels so you have a black and white image with the
		 * white pixels being the ones that where white in each channel
		 * after the thresholding.
		 *
		 * you almost definitely want perform color correction before using
		 * this function
		 *
		 * @arg thresh_params a list of the min and max values for a pixel
		 * if min is greater than max, it will OR the pixels that passed
		 * both thresholds
		 * the first value of each array of double is
		 * exclisive eg. if you want to include pixels with value 0, it
		 * should be a negative number. the second value is inclusive.
		 */
		static cv::Mat multichannel_thresh(const cv::Mat& input,
				const std::vector< std::array<double, 2> > thresh_params);

		/**
		 * @arg thresh can usually be around 100 or so
		 */
		static std::pair<std::vector< std::vector<cv::Point> >,
				std::vector<cv::Vec4i> > image_to_contours(
				const cv::Mat& img, const int thresh, int dialation=1);

		static std::vector<bool> filter_out_child_contours(
				const std::pair<std::vector< std::vector<cv::Point> >,
				std::vector<cv::Vec4i> >& contours, bool invert=false);

		static std::vector<bool> filter_out_parent_contours(
				const std::pair<std::vector< std::vector<cv::Point> >,
				std::vector<cv::Vec4i> >& contours, bool invert=false);

		static std::vector<int> contours_sort_by_area(
				const std::vector< std::vector<cv::Point> >& contours);

		static std::vector<bool> filter_contours_area(
				std::vector< std::vector<cv::Point> >& contours,
				const double min,
				const double max=std::numeric_limits<double>::max(),
				bool invert=false);

		static double contour_ratio_within(const std::vector<cv::Point> inner,
				const std::vector<cv::Point> outer);

		static void filter_contours_within(
				const std::vector< std::vector<cv::Point> >& outer_src,
				std::vector<int>& outer_dst,
				const std::vector< std::vector<cv::Point> >& inner,
				std::vector<int>& inner_dst,
				const double thresh=0.51);

		/**
		 * use contour_to_mask() instead of this
		 */
		static void black_outside_contour(cv::Mat& img,
				std::vector<cv::Point> contour, bool invert=false);

		static void enhance_hist(cv::Mat& src, cv::Mat& dst);

		/**
		 * a stupid enhancement function
		 * use cv::balanceWhite instead
		 */
		static void color_correct_dumb(cv::Mat& src, cv::Mat& dst);

		static void contour_to_mask(std::vector<cv::Point>& contour,
				cv::Mat& mask);

		static cv::Point2f contour_centroid(std::vector<cv::Point> contour);

		static std::vector<bool> list_and(const std::vector<bool>& a,
				const std::vector<bool>& b);

		template <typename T>
		static void filter(const std::vector<bool>& scores,
				std::vector<T>& list, bool invert=false)
		{
			for (int i = list.size() - 1; i >=0; i--)
			{
				if (!scores[i] != invert)
				{
					list.erase(list.begin() + i);
				}
			}
		}

		static std::vector<int> contour_children(
				std::vector<cv::Vec4i> hierarchy, int index);

		/**
		 * @arg indices contains a list of indices of the src vector. each indice
		 * must be unique, but indices can be a subset of src.
		 */
		template <typename T>
		static std::vector<T> indices_to_vector(std::vector<T> src,
				std::vector<int> indices)
		{
			std::vector<T> dst(indices.size());
			for (int i = 0; i < indices.size(); i++)
			{
				dst[i] = src[indices[i]];
			}
			return dst;
		}

		static int contour_max(const std::vector< std::vector<cv::Point> >&
				contours);

		static void contours_to_convexhulls(
				const std::vector< std::vector<cv::Point> >& src,
				std::vector< std::vector<cv::Point> >& dst);

		static std::vector<bool> filter_out_enclosing_contours(
				const std::vector< std::vector<cv::Point> >& contours,
				double thresh=.9);

		static std::vector<bool> indices_to_scores(std::vector<int> indices,
				size_t size, bool invert=false);

		static std::vector< std::pair<int, int> > contours_find_intersections(
				std::vector< std::vector<cv::Point> > contours,
				double thresh=.1);

		static std::vector<bool> contour_pair_area_smaller(
				std::vector< std::pair<int, int> > pairs,
				std::vector< std::vector<cv::Point> > contours,
				bool invert=false);

		static std::vector<bool> filter_shapes(
				std::vector<cv::Point> sample,
				std::vector< std::vector<cv::Point> > inputs,
				double thresh=100.0, bool invert=false);

		static cv::Mat draw_contours(
				std::vector< std::vector<cv::Point> > contours,
				const cv::Mat& img_bgr);

		static void whitebalance_simple(const cv::Mat& src,
				cv::Mat& dst, float s1, float s2, int outmin, int outmax);

		static void whitebalance_simple_wrapper(
				const cv::Mat& src, cv::Mat& dst, float s1=.04f,
				float s2=.04f, int outmin=0, int outmax=255);

		static double circularity(
				const std::vector<cv::Point>& contour);

		static std::vector<bool> filter_circularity(
				const std::vector< std::vector<cv::Point> >& contours,
				double thresh);

		static int integrate(cv::Mat src, int min=0, int max=255);
};
