#ifndef VISION_H
#define VISION_H

#include <opencv2/opencv.hpp>

// Simple struct to hold centroid stats
struct Centroid {
    cv::Point2d pos;      // (x, y) in full-resolution image coordinates
    double sum;           // total intensity
    int area;             // pixel count
    double xx, yy, xy;    // second central moments
    double axisRatio;     // major/minor
};

struct VisionResult {
    bool found = false;
    int x = 0;
    int y = 0;
    double fwhm = 0.0;
    double ecc = 0.0;
};

struct PrimaryROI {
    int CornerAx, CornerAy, CornerBx, CornerBy;
    int primaryCenterX, primaryCenterY;
    bool ROISet;
};

class VisionManager {
public:
    VisionManager();

    VisionResult findTargetPosition(const cv::Mat &frame, bool starMode);
    bool defineROIUpperLeft(int targetX, int targetY);
    bool defineROILowerRight(int targetX, int targetY);

    void resetTracking();

    std::optional<PrimaryROI> primaryROI;

private:
    bool   hasTrack        = false;
    int    successCount    = 0;
    int    failCount       = 0;
    int    succToStart     = 3;
    int    failToDrop      = 5;
    double searchRadius    = 50.0;    // pixels
    double predX           = 0.0;
    double predY           = 0.0;

    void findComponents(const cv::Mat &imgFloat,
                           std::vector<Centroid> &out);

    std::vector<Centroid> getCentroidsFromImage(const cv::Mat &frame);

    int MIN_AREA = 3, MAX_AREA = 200; // reject big blobs/trails
    double MAX_AXIS_RATIO = 2.5; // reject elongated star trails
};

#endif // VISION_H