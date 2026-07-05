#include "Vision.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>

VisionManager::VisionManager()
{
    hasTrack     = false;

    successCount = 0;
    failCount    = 0;

    succToStart  = 3;
    failToDrop   = 5;

    searchRadius = 50.0;

    predX = 0.0;
    predY = 0.0;
}

void VisionManager::resetTracking()
{
    hasTrack     = false;
    successCount = 0;
    failCount    = 0;

    predX = 0.0;
    predY = 0.0;
}

VisionResult VisionManager::findTargetPosition(const cv::Mat &frame, bool starMode)
{
    VisionResult result;

    std::vector<Centroid> centroids = getCentroidsFromImage(frame);
    if (centroids.empty()) {
        successCount = 0;
        failCount++;
        if (failCount >= failToDrop)
            hasTrack = false;
        return result;
    }

    int width  = frame.cols;
    int height = frame.rows;

    // Convert centroids to centered pixel coordinates
    std::vector<cv::Point2d> candidates;
    candidates.reserve(centroids.size());

    for (const auto &c : centroids) {
        double x = c.pos.x - width  * 0.5;
        double y = -(c.pos.y - height * 0.5);   // y increases downward
        candidates.emplace_back(x, y);
    }

    int bestIndex = -1;

    // --------------------------------------------------------
    // Acquisition mode (hasTrack == false)
    // --------------------------------------------------------
    if (!hasTrack) {

        // Use frame center as search position
        double searchX = 0.0;
        double searchY = 0.0;

        if (starMode) {
            // Star calibration: brightest centroid
            bestIndex = 0;
        } else {
            // ISS acquisition: closest to center
            double bestDist2 = std::numeric_limits<double>::infinity();
            for (int i = 0; i < (int)candidates.size(); ++i) {
                double dx = candidates[i].x - searchX;
                double dy = candidates[i].y - searchY;
                double d2 = dx*dx + dy*dy;
                if (d2 < bestDist2) {
                    bestDist2 = d2;
                    bestIndex = i;
                }
            }
        }

        if (bestIndex >= 0) {
            successCount++;
            failCount = 0;

            if (successCount >= succToStart) {
                hasTrack = true;
                predX = centroids[bestIndex].pos.x;
                predY = centroids[bestIndex].pos.y;
            }
        } else {
            failCount++;
            return result;
        }
    }

    // --------------------------------------------------------
    // Tracking mode (hasTrack == true)
    // --------------------------------------------------------
    else {
        // Use internal prediction
        double searchX = predX - width  * 0.5;
        double searchY = -(predY - height * 0.5);

        double bestDist2 = std::numeric_limits<double>::infinity();
        for (int i = 0; i < (int)candidates.size(); ++i) {
            double dx = candidates[i].x - searchX;
            double dy = candidates[i].y - searchY;
            double d2 = dx*dx + dy*dy;
            if (d2 < bestDist2) {
                bestDist2 = d2;
                bestIndex = i;
            }
        }

        if (bestIndex < 0) {
            failCount++;
            if (failCount >= failToDrop)
                hasTrack = false;
            return result;
        }

        double dx = candidates[bestIndex].x - searchX;
        double dy = candidates[bestIndex].y - searchY;

        if (std::abs(dx) > searchRadius || std::abs(dy) > searchRadius) {
            failCount++;
            if (failCount >= failToDrop)
                hasTrack = false;
            return result;
        }

        successCount++;
        failCount = 0;

        // Update prediction
        predX = centroids[bestIndex].pos.x;
        predY = centroids[bestIndex].pos.y;
    }

    // --------------------------------------------------------
    // Build result
    // --------------------------------------------------------
    const Centroid &c = centroids[bestIndex];

    result.found = true;
    result.x     = (int)c.pos.x;
    result.y     = (int)c.pos.y;

    // FWHM and eccentricity
    double trace = c.xx + c.yy;
    double det   = c.xx * c.yy - c.xy * c.xy;
    double disc  = trace * trace * 0.25 - det;
    if (disc < 0) disc = 0;

    double sqrtDisc = std::sqrt(disc);
    double lambda1  = 0.5 * trace + sqrtDisc;
    double lambda2  = 0.5 * trace - sqrtDisc;
    if (lambda2 <= 0) lambda2 = std::numeric_limits<double>::min();

    double sigmaMajor = std::sqrt(lambda1);
    double sigmaMinor = std::sqrt(lambda2);

    result.fwhm = 2.355 * sigmaMajor;
    result.ecc  = std::sqrt(1.0 - (sigmaMinor*sigmaMinor) /
                                   (sigmaMajor*sigmaMajor));

    return result;
}


std::vector<Centroid> VisionManager::getCentroidsFromImage(const cv::Mat &frame)
{
    std::vector<Centroid> out;
    findComponents(frame, out);

    // Sort by brightness (sum) descending
    std::sort(out.begin(), out.end(),
              [](const Centroid &a, const Centroid &b){ return a.sum > b.sum; });

    return out;
}

void VisionManager::findComponents(const cv::Mat &imgFloat,
                           std::vector<Centroid> &out)
{
    // Local mean background subtraction
    int filtsize = 15;

    cv::Mat localMean;
    cv::blur(imgFloat, localMean, cv::Size(filtsize, filtsize), cv::Point(-1,-1), cv::BORDER_REPLICATE);
    cv::Mat img = imgFloat - localMean;

    // Global root-square sigma
    cv::Mat sq;
    cv::multiply(img, img, sq);
    double meanSq = cv::mean(sq)[0];
    double imgStd = std::sqrt(std::max(meanSq, 0.0));
    double sigma  = 3.0;
    double th     = imgStd * sigma;

    // Threshold
    cv::Mat mask;
    cv::threshold(img, mask, th, 255.0, cv::THRESH_BINARY);
    mask.convertTo(mask, CV_8U);

    // Binary opening with 3x3 cross
    cv::Mat kernel = (cv::Mat_<uchar>(3,3) <<
                       0,1,0,
                       1,1,1,
                       0,1,0);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);

    // Connected components
    cv::Mat labels, stats, centroidsGeom;
    int nLabels = cv::connectedComponentsWithStats(mask, labels, stats, centroidsGeom, 8, CV_32S);
    if (nLabels <= 1) return;

    int rows = img.rows;
    int cols = img.cols;

    for (int lab = 1; lab < nLabels; ++lab) {
        int area = stats.at<int>(lab, cv::CC_STAT_AREA);
        if (area < MIN_AREA || area > MAX_AREA) continue;

        int x0 = stats.at<int>(lab, cv::CC_STAT_LEFT);
        int y0 = stats.at<int>(lab, cv::CC_STAT_TOP);
        int w  = stats.at<int>(lab, cv::CC_STAT_WIDTH);
        int h  = stats.at<int>(lab, cv::CC_STAT_HEIGHT);

        cv::Mat labelROI = labels(cv::Rect(x0, y0, w, h));
        cv::Mat imgROI   = img(cv::Rect(x0, y0, w, h));

        double sumI = 0.0;
        double m10  = 0.0;
        double m01  = 0.0;
        double m20  = 0.0;
        double m02  = 0.0;
        double m11  = 0.0;

        for (int ry = 0; ry < h; ++ry) {
            const int   *plab  = labelROI.ptr<int>(ry);
            const float *pimg  = imgROI.ptr<float>(ry);
            int gy = y0 + ry;
            for (int rx = 0; rx < w; ++rx) {
                if (plab[rx] != lab) continue;
                double I = static_cast<double>(pimg[rx]);
                if (I <= 0.0) continue;
                int gx = x0 + rx;
                sumI += I;
                m10  += I * gx;
                m01  += I * gy;
                m20  += I * gx * gx;
                m02  += I * gy * gy;
                m11  += I * gx * gy;
            }
        }

        if (sumI <= 0.0) continue;

        double xc = m10 / sumI;
        double yc = m01 / sumI;

        double xx = m20 / sumI - xc * xc;
        double yy = m02 / sumI - yc * yc;
        double xy = m11 / sumI - xc * yc;
        if (xx < 0) xx = 0;
        if (yy < 0) yy = 0;

        double trace = xx + yy;
        double det   = xx * yy - xy * xy;
        double disc  = trace * trace * 0.25 - det;
        if (disc < 0) disc = 0;
        double sqrtDisc = std::sqrt(disc);
        double lambda1  = 0.5 * trace + sqrtDisc;
        double lambda2  = 0.5 * trace - sqrtDisc;
        if (lambda2 <= 0) lambda2 = std::numeric_limits<double>::min();
        double axisRatio = std::sqrt(std::max(lambda1 / lambda2, 1.0));
        if (axisRatio > MAX_AXIS_RATIO) continue;

        Centroid c;
        c.pos        = cv::Point2d(xc, yc);
        c.sum        = sumI;
        c.area       = area;
        c.xx         = xx;
        c.yy         = yy;
        c.xy         = xy;
        c.axisRatio  = axisRatio;
        out.push_back(c);
    }
}

bool VisionManager::defineROIUpperLeft(int targetX, int targetY) {
    if (primaryROI->CornerBx != -1) {
        if (targetX > primaryROI->CornerBx || targetY > primaryROI->CornerBy) {
            int tempX = primaryROI->CornerBx;
            int tempY = primaryROI->CornerBy;
            primaryROI->CornerBx = targetX;
            primaryROI->CornerBy = targetY;
            primaryROI->CornerAx = tempX;
            primaryROI->CornerAy = tempY;
        } else {
            primaryROI->CornerAx = targetX;
            primaryROI->CornerAy = targetY;
        }

        primaryROI->primaryCenterX = (primaryROI->CornerAx + primaryROI->CornerBx) / 2;
        primaryROI->primaryCenterY = (primaryROI->CornerAy + primaryROI->CornerBy) / 2;
        primaryROI->ROISet = true;

        return true;
    } else {
        primaryROI->CornerAx = targetX;
        primaryROI->CornerAy = targetY;
    }

    return false;
}

bool VisionManager::defineROILowerRight(int targetX, int targetY) {
    if (primaryROI->CornerAx != -1) {
        if (targetX < primaryROI->CornerAx || targetY < primaryROI->CornerAy) {
            int tempX = primaryROI->CornerAx;
            int tempY = primaryROI->CornerAy;
            primaryROI->CornerAx = targetX;
            primaryROI->CornerAy = targetY;
            primaryROI->CornerBx = tempX;
            primaryROI->CornerBy = tempY;
        } else {
            primaryROI->CornerBx = targetX;
            primaryROI->CornerBy = targetY;
        }

        primaryROI->primaryCenterX = (primaryROI->CornerAx + primaryROI->CornerBx) / 2;
        primaryROI->primaryCenterY = (primaryROI->CornerAy + primaryROI->CornerBy) / 2;
        primaryROI->ROISet = true;
        return true;
    } else {
        primaryROI->CornerBx = targetX;
        primaryROI->CornerBy = targetY;
    } 

    return false;
}