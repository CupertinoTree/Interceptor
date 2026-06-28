#include "Vision.h"
#include <opencv2/opencv.hpp>

// Static variables
int Vision::satelliteX = -1;
int Vision::satelliteY = -1;
int Vision::ROICornerAx = -1;
int Vision::ROICornerAy = -1;
int Vision::ROICornerBx = -1;
int Vision::ROICornerBy = -1;
int Vision::primaryCenterX = -1;
int Vision::primaryCenterY = -1;
double Vision::FWHM = 0.0;
double Vision::eccentricity = 0.0;
bool Vision::primaryROISet = false;

bool Vision::findTargetPosition(cv::Mat matrix)
{
    if (matrix.empty())
        return false;

    int width = matrix.cols;
    int height = matrix.rows;

    // ---------------------------------------------------------
    // 1. Extract green channel (fast grayscale)
    // ---------------------------------------------------------
    cv::Mat green;
    cv::extractChannel(matrix, green, 1);   // channel 1 = G

    // ---------------------------------------------------------
    // 2. Downscale for speed (0.5x)
    // ---------------------------------------------------------
    cv::Mat small;
    cv::resize(green, small, cv::Size(), 0.5, 0.5, cv::INTER_AREA);

    int sw = small.cols;
    int sh = small.rows;

    // ---------------------------------------------------------
    // 3. Threshold to isolate saturated star cores
    // ---------------------------------------------------------
    cv::Mat thresh;
    cv::threshold(small, thresh, 180, 255, cv::THRESH_BINARY);

    // ---------------------------------------------------------
    // 4. Find contours (candidate stars)
    // ---------------------------------------------------------
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty())
        return false;

    // ---------------------------------------------------------
    // 5. Pick the largest contour (brightest star)
    // ---------------------------------------------------------
    double bestArea = -1.0;
    int bestIndex = -1;

    for (int i = 0; i < contours.size(); ++i)
    {
        double area = cv::contourArea(contours[i]);
        if (area > bestArea)
        {
            bestArea = area;
            bestIndex = i;
        }
    }

    if (bestIndex < 0)
        return false;

    // ---------------------------------------------------------
    // 6. Compute centroid of the chosen contour
    // ---------------------------------------------------------
    cv::Moments m = cv::moments(contours[bestIndex]);
    if (m.m00 == 0)
        return false;

    double cx = m.m10 / m.m00;
    double cy = m.m01 / m.m00;

    // Scale back to original resolution
    Vision::satelliteX = static_cast<int>(cx * 2.0 + 1);
    Vision::satelliteY = static_cast<int>(cy * 2.0 + 1);

    // ---------------------------------------------------------
    // 7. Compute FWHM + eccentricity (small ROI around centroid)
    // ---------------------------------------------------------
    int roiSize = 51;
    int half = roiSize / 2;

    int rx = std::clamp(Vision::satelliteX, half, width - half - 1);
    int ry = std::clamp(Vision::satelliteY, half, height - half - 1);

    cv::Rect roi(rx - half, ry - half, roiSize, roiSize);
    cv::Mat patch = green(roi);

    // Threshold the patch
    cv::Mat patchThresh;
    cv::threshold(patch, patchThresh, 180, 255, cv::THRESH_BINARY);

    // Find contours inside the patch
    std::vector<std::vector<cv::Point>> patchContours;
    cv::findContours(patchThresh, patchContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (!patchContours.empty())
    {
        // We know this patch is centered on the chosen star,
        // so just use the first (dominant) contour.
        const auto& starContour = patchContours[0];

        if (starContour.size() >= 5)
        {
            cv::RotatedRect ellipse = cv::fitEllipse(starContour);

            double major = std::max(ellipse.size.width, ellipse.size.height);
            double minor = std::min(ellipse.size.width, ellipse.size.height);

            // FWHM from Gaussian sigma (major axis)
            double sigma = major / 2.355;
            Vision::FWHM = 2.355 * sigma;

            // Eccentricity
            Vision::eccentricity = std::sqrt(1.0 - (minor * minor) / (major * major));
    }
}

    return true;
}
