#include "AppController.h"
#include "CameraController.h"
#include "ui/MainWindow.h"
#include <iostream>

AppController::AppController(MainWindow* mw, QObject* parent)
    : QObject(parent),
      mainWindow(mw),
      cameraController(new CameraController(this))
{
    setupConnections();
}

AppController::~AppController() { 
    std::cout << "destroying AppController" << std::endl;
 }

void AppController::setupConnections()
{
    //
    // UI → Controller
    //
    connect(mainWindow, &MainWindow::cameraConnectRequested,
            this, &AppController::onCameraConnectRequested);

    connect(mainWindow, &MainWindow::exposureChangeRequested,
            this, &AppController::onExposureChangeRequest);

    connect(mainWindow, &MainWindow::gainChangeRequested,
            this, &AppController::onGainChangeRequest);

    connect(mainWindow, &MainWindow::exposureUnitChangeRequested,
            this, &AppController::onExposureUnitChangeRequested);

    connect(mainWindow, &MainWindow::defineULRequested,
            this, &AppController::onDefineULRequested);

    connect(mainWindow, &MainWindow::defineLRRequested,
            this, &AppController::onDefineLRRequested);

    connect(mainWindow, &MainWindow::roiResetRequested,
            this, &AppController::onROIResetRequested);

    connect(mainWindow, &MainWindow::autodetectTargetRequested,
            this, &AppController::onAutodetectTargetRequested);


    //
    // Controller → UI
    //
    connect(cameraController, &CameraController::cameraConnected,
            this, &AppController::onCameraConnected);

    connect(cameraController, &CameraController::cameraDisconnected,
            this, &AppController::onCameraDisconnected);

    connect(cameraController, &CameraController::errorMessage,
            mainWindow, &MainWindow::showErrorDialog);
    
    connect(cameraController, &CameraController::frameReadyForUI,
            mainWindow, &MainWindow::setFrame);

    connect(cameraController, &CameraController::setCameraListEmpty,
            mainWindow, &MainWindow::setCameraListEmpty);

    connect(cameraController, &CameraController::setCameraList,
            mainWindow, &MainWindow::setCameraList);

    connect(cameraController, &CameraController::newVisionResult, 
            this, &AppController::onNewVisionResult);

    connect(cameraController, &CameraController::autodetectSet,
            mainWindow, &MainWindow::setAutodetect);

    connect(cameraController, &CameraController::upperLeftSet,
            mainWindow, &MainWindow::setUpperLeft);

    connect(cameraController, &CameraController::lowerRightSet,
            mainWindow, &MainWindow::setLowerRight);

    connect(cameraController, &CameraController::ROISet,
            mainWindow, &MainWindow::setROI);

    connect(cameraController, &CameraController::fpsUpdated,
            mainWindow, &MainWindow::setFPS);

    connect(cameraController, &CameraController::gainApplied,
        mainWindow, &MainWindow::onGainApplied);

    connect(cameraController, &CameraController::exposureTimeApplied,
        mainWindow, &MainWindow::onExposureTimeApplied);

}


//
// UI → Controller
//
void AppController::onCameraConnectRequested(int cameraIndex)
{
    cameraController->selectCamera(cameraIndex);
}

void AppController::onAutodetectTargetRequested() {
    cameraController->setAutodetect();
}

void AppController::onGainChangeRequest(int value)
{
    cameraController->setGain(value);
}

void AppController::onExposureChangeRequest(int value)
{
    cameraController->setExposure(value);
}

void AppController::onExposureUnitChangeRequested(ExposureUnit unit)
{
    cameraController->setExposureUnit(unit);
}

void AppController::onDefineULRequested() {
    cameraController->defineROIUpperLeft(latestVisionResult.x, latestVisionResult.y);
}

void AppController::onDefineLRRequested() {
    cameraController->defineROILowerRight(latestVisionResult.x, latestVisionResult.y);
}

void AppController::onROIResetRequested() {
    cameraController->resetROI();
}

//
// Controller → UI
//
void AppController::onNewVisionResult(VisionResult vr) {
    if (vr.found) {
        latestVisionResult = vr;
    }

    mainWindow->updateVisionResult(vr);
}

void AppController::onCameraConnected()
{
    mainWindow->setCameraConnected();
    //mainWindow->setTabsEnabled(true, true, true);
}

void AppController::onCameraDisconnected()
{
    // latestVisionResult.found can never be false except here after reset
    latestVisionResult = VisionResult();
    mainWindow->setCameraDisconnected();
    /////////////////////////////////////// Stop the tracking loop
    //mainWindow->setTabsEnabled(false, false, false);
}