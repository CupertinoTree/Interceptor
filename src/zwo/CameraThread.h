#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <opencv2/opencv.hpp>
#include "ASICamera2.h"
#include <iostream>

class CameraThread : public QThread {
    Q_OBJECT
public:
    CameraThread(int cameraID, QObject *parent = nullptr);
    void startCapture();
    void stopCapture();
    cv::Mat getLatestFrame();
    int getAndResetFrameCount();

signals:
    void frameReady();

protected:
    void run() override;

private:
    int cameraID;
    bool running;
    cv::Mat latestFrame;
    QMutex mutex;
    int frameCount;
    QMutex countMutex;
    bool SIMULATION_MODE = false;
};

#endif // CAMERATHREAD_H