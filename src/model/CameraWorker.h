#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <opencv2/opencv.hpp>
#include "ASICamera2.h"
#include <iostream>

class CameraWorker : public ::QObject {
    Q_OBJECT

public:
    CameraWorker(int cameraID);
    int getASIDefaultGain();
    bool setGain(int value);
    bool setExposure(int value);
    int getNumOfCameras();
    QStringList getCamerasList(int numCameras);

public slots:
    bool openCamera();
    bool closeCamera();
    bool startVideoCapture();
    bool stopVideoCapture();
    void startCaptureLoop();
    void stopCaptureLoop();

signals:
    void frameReady(const cv::Mat& frame);
    void ASIErrorMessage(const QString& message);
    void captureLoopTimeout();
    void captureLoopErrorOccured(const QString& errorMsg);

private:
    void captureLoop();
    cv::Mat toMonochrome(const cv::Mat &frame);
    QString asiErrorToString(ASI_ERROR_CODE code);
    void simulationCaptureLoop();

private:
    int cameraID;
    bool running;
    bool isColorCamera;

    //Creates fake frames with a twinkling stars and always returns yes for camera connection.
    bool SIMULATION_MODE = false;
};

#endif