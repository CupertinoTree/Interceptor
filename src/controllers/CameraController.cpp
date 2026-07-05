#include "CameraController.h"
#include <QTimer>
#include "model/Enums.h"
#include <QImage>
#include <QPixmap>
#include "FrameRenderer.h"

CameraController::CameraController(QObject* parent): cameraThread(nullptr), cameraWorker(nullptr), visionManager() {
    fpsTimer = new QTimer(this);
    fpsTimer->setInterval(1000);

    cameraListTimer = new QTimer(this);
    cameraListTimer->setInterval(2000);
    cameraListTimer->start();

    setupConnections();

    autodetectTarget = false;
}

CameraController::~CameraController() {
    closeCamera();
}

//
//  Qt6 Connection management
//
void CameraController::setupConnections()
{
    connect(cameraListTimer, &QTimer::timeout,
        this, &CameraController::updateCameraList);

    connect(fpsTimer, &QTimer::timeout, this, &CameraController::onFPSTimer);
}

void CameraController::setupThreadConnections() {

    connect(cameraWorker, &CameraWorker::frameReady,
        this, &CameraController::onFrameReady);

    connect(cameraWorker, &CameraWorker::captureLoopTimeout,
            this, &CameraController::onCaptureLoopTimeout);

    connect(cameraWorker, &CameraWorker::captureLoopErrorOccured,
            this, &CameraController::onCaptureLoopError);

    connect(cameraWorker, &CameraWorker::ASIErrorMessage,
            this, &CameraController::errorMessage);

    connect(cameraThread, &QThread::started,
        cameraWorker, &CameraWorker::startCaptureLoop);
}

//
// Pubblic methods
//
void CameraController::selectCamera(int index)
{

    if (index <= 0) {
        //cameraID = -1; will be done by closeCamera
        closeCamera();
        return;
    }

    if (cameraID == index - 1) { return; }

    if(cameraWorkerRunning()) {
        closeCamera();
        //But continue to open the new camera
    }

    cameraID = index - 1;

    if (openCamera()) {
        emit cameraConnected();
    } else {
        //cameraID = -1; will be done by closeCamera
        closeCamera();
    }

}

void CameraController::setAutodetect() {
    if (!autodetectTarget) {
        if (cameraWorkerRunning()) {
            autodetectTarget = true;
        }
    } else {
        autodetectTarget = false;
    }

    emit autodetectSet(autodetectTarget);
}

void CameraController::setGain(int value)
{
    if (!cameraWorkerRunning()) {
        emit errorMessage("setGain:: Camera worker is not running.");
        return;
    }

    if (cameraWorker->setGain(value)) {
        gain = value;
    }

    // So that at least if the user changed the gain but the camera doesn't accept it, it will go back to the original value
    emit gainApplied(gain);
}

void CameraController::setExposure(int value)
{
    if (!cameraWorkerRunning()) {
        emit errorMessage("setExposure:: Camera worker is not running.");
        return;
    }

    int actualValue = (exposureUnit == ExposureUnit::ms) ? (value * 1000) : value;

    if (cameraWorker->setExposure(actualValue)) {
        // The value is stored in microseconds
        exposure = actualValue;
    }

    // For the UI part, we stay in [1; 1000] depending on the unit
    // However we can't just use value even though it is in the right range, because the camera might not accept it and we need to go back to the original value
    emit exposureTimeApplied(exposureUnit == ExposureUnit::ms ? exposure / 1000 : exposure, exposureUnit == ExposureUnit::ms ? "ms" : "μs");
}

void CameraController::setExposureUnit(ExposureUnit unit)
{   
    if (exposureUnit == unit) return;

    if (!cameraWorkerRunning()) {
        emit errorMessage("setExposureUnit:: Camera worker is not running.");
        return;
    }

    int newExposureValue = 0;

    // Heavy but at least it is ready for future units if ever needed
    if (unit == ExposureUnit::ms && exposureUnit == ExposureUnit::μs) {
        newExposureValue = qBound(1, exposure * 1000, 1000000);
    } else if (unit == ExposureUnit::μs && exposureUnit == ExposureUnit::ms) {
        newExposureValue = qBound(1, exposure / 1000, 1000);
    }

    if (cameraWorker->setExposure(newExposureValue)) {
        exposureUnit = unit;
        exposure = newExposureValue;
    }

    emit exposureTimeApplied(exposureUnit == ExposureUnit::ms ? exposure / 1000 : exposure, exposureUnit == ExposureUnit::ms ? "ms" : "μs");
}

//
// Private helpers
//
bool CameraController::openCamera()
{
    cameraThread = new QThread;
    cameraWorker = new CameraWorker(cameraID);

    cameraWorker->moveToThread(cameraThread);

    setupThreadConnections();

    if (!cameraWorker->openCamera()) { return false; }
    if (!cameraWorker->startVideoCapture()) { return false; }

    cameraThread->start();
    
    fpsTimer->start();

    exposure = DEFAUlT_EXPOSURE;
    gain = cameraWorker->getASIDefaultGain();
    exposureUnit = ExposureUnit::μs;

    setGain(gain);
    setExposure(exposure);

    return true;
}

void CameraController::closeCamera()
{
    if (cameraWorker) {
	    cameraWorker->stopCaptureLoop();
	    cameraWorker->stopVideoCapture();
        cameraWorker->closeCamera();
    }


    if (cameraThread) {
        cameraThread->quit();
        cameraThread->wait();
    }

    delete cameraWorker;
    delete cameraThread;

    cameraWorker = nullptr;
    cameraThread = nullptr;
    autodetectTarget = false;
    visionManager.resetTracking();
    //visionManager.primaryROI.reset();

    fpsTimer->stop();
    framesThisSecond = 0;
    captureLoopTimeoutCount = 0;
    cameraID = -1;

    exposure = 1;
    exposureUnit = ExposureUnit::μs;
    gain = 0;
    emit gainApplied(gain);
    emit exposureTimeApplied(exposure, "μs");

    emit cameraDisconnected();
}

void CameraController::defineROIUpperLeft(int targetX, int targetY) {
    if (!cameraWorkerRunning() || visionManager.primaryROI->CornerAx >= 0 /* || isGuiding*/) { return; }

    if (visionManager.defineROIUpperLeft(targetX, targetY)) { emit ROISet(); }

    emit upperLeftSet("X: " + QString::number(visionManager.primaryROI->CornerAx) + " Y: " + QString::number(visionManager.primaryROI->CornerAy));
}

void CameraController::defineROILowerRight(int targetX, int targetY) {
    if (!cameraWorkerRunning() || visionManager.primaryROI->CornerBx >= 0 /* || isGuiding*/) { return; }

    if (visionManager.defineROILowerRight(targetX, targetY)) { emit ROISet(); }

    emit lowerRightSet("X: " + QString::number(visionManager.primaryROI->CornerBx) + " Y: " + QString::number(visionManager.primaryROI->CornerBy));
}

void CameraController::resetROI() {
    visionManager.primaryROI.reset();
}

void CameraController::updateCameraList() {
    //si la caméra n'est plus dedans, envoyer cameraDisconnected

    int numCameras = cameraWorker->getNumOfCameras();

    // Only update if the number of cameras has changed
    if (numCameras == lastNumCameras) {
        return;
    }

    if (cameraID > numCameras) {
        closeCamera();
        return;
    }

    lastNumCameras = numCameras;

    if (numCameras == 0) {
        emit setCameraListEmpty();
        return;
    }

    QStringList list = cameraWorker->getCamerasList(numCameras);

    emit setCameraList(list);
}

bool CameraController::cameraWorkerRunning() const { return (cameraWorker && cameraThread && cameraThread->isRunning()); }

int CameraController::getAndResetFrameCount() {
    int count = framesThisSecond;
    framesThisSecond = 0;
    return count;
}

//
// Controller slots
//
void CameraController::onFrameReady(const cv::Mat& frame) { 
    if (captureLoopTimeoutCount > 0) { captureLoopTimeoutCount = 0; }

    QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8);
    QPixmap pix = QPixmap::fromImage(img);

    if (autodetectTarget) {
        VisionResult vr = visionManager.findTargetPosition(frame, starMode);

        if (vr.found) {
            FrameRenderer::renderTarget(pix, vr);
        }

        emit newVisionResult(vr);
    }

    if (visionManager.primaryROI) {
        FrameRenderer::renderPrimaryROI(pix, *visionManager.primaryROI);
    }

    framesThisSecond++;

    emit frameReadyForUI(pix);
}

void CameraController::onCaptureLoopTimeout() {
    captureLoopTimeoutCount++;

    if (captureLoopTimeoutCount >= 1000) {
        emit errorMessage("Capture loop timeout occurred a thousand times in a row. Stopping the camera.");
        closeCamera();
    }
}

void CameraController::onCaptureLoopError(const QString& errorMsg) {
    captureLoopTimeoutCount = 0;
    closeCamera();
    emit errorMessage(errorMsg);
}

void CameraController::onFPSTimer() {
    if (!cameraWorkerRunning()) {
        emit errorMessage("onFPSTimer:: Camera worker is not running.");
        fpsTimer->stop();
        return;
    }

    int fps = getAndResetFrameCount();
    emit fpsUpdated(fps);
}