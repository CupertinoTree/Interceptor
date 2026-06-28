#ifndef VISION_H
#define VISION_H

#include <opencv2/opencv.hpp>
#include "CameraThread.h"

class Vision {
public:
   // Vision();
    //~Vision();

    // Target tracking variables
    static int satelliteX, satelliteY, ROICornerAx, ROICornerAy, ROICornerBx, ROICornerBy, primaryCenterX, primaryCenterY;

    static double FWHM, eccentricity;

    static bool primaryROISet;

    static bool findTargetPosition(cv::Mat matrix);
};

#endif // VISION_H