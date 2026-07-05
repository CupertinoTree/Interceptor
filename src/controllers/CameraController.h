#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QThread>
#include "CameraWorker.h"
#include "Vision.h"
#include "model/Enums.h"

class CameraController : public ::QObject {
    Q_OBJECT
public:
    explicit CameraController(QObject* parent = nullptr);
    ~CameraController();

    void selectCamera(int cameraID);
    void setAutodetect();

    void setExposure(int value);
    void setGain(int value);
    void setExposureUnit(ExposureUnit unit);

    void defineROIUpperLeft(int targetX, int targetY);
    void defineROILowerRight(int targetX, int targetY);
    void resetROI();

signals:
    void frameReadyForUI(QPixmap frame);
    void fpsUpdated(int fps);
    void newVisionResult(VisionResult vr);
    void cameraConnected();
    void cameraDisconnected();
    void errorMessage(const QString& message);
    void gainApplied(int value);
    void exposureTimeApplied(int value, QString unit);
    void autodetectSet(bool autodetect);
    void ROISet();
    void upperLeftSet(QString coords);
    void lowerRightSet(QString coords);
    void setCameraListEmpty();
    void setCameraList(QStringList list);

private:
    QThread* cameraThread;
    CameraWorker* cameraWorker;
    ExposureUnit exposureUnit;
    VisionManager visionManager;
    int cameraID;
    int lastNumCameras = -1;
    int gain, exposure;
    int DEFAUlT_EXPOSURE = 1; // in microseconds

    QTimer* cameraListTimer;
    QTimer* fpsTimer;
    int framesThisSecond = 0;
    int captureLoopTimeoutCount = 0;
    void setupConnections();
    void setupThreadConnections();

    bool openCamera();
    void closeCamera();

    bool cameraWorkerRunning() const;
    void updateCameraList();
    int getAndResetFrameCount();

    bool starMode = true; //implement a slot to change this if the tracking thread sends a signal
    bool autodetectTarget;

private slots:
    void onFrameReady(const cv::Mat& frame);
    void onFPSTimer();
    void onCaptureLoopTimeout();
    void onCaptureLoopError(const QString& errorMsg);
};

#endif // CAMERACONTROLLER_H
