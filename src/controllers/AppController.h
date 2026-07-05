#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include "model/Enums.h"
#include "Vision.h"

class CameraController;
class MainWindow;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow* mainWindow, QObject* parent = nullptr);
    ~AppController();

private:
    CameraController* cameraController;
    MainWindow* mainWindow;

    void setupConnections();
    void updateTabAvailability();

    VisionResult latestVisionResult;

private slots:
    // UI → Controller
    void onCameraConnectRequested(int cameraIndex);
    void onExposureChangeRequest(int value);
    void onGainChangeRequest(int value);
    void onExposureUnitChangeRequested(ExposureUnit unit);
    void onDefineULRequested();
    void onDefineLRRequested();
    void onROIResetRequested();
    void onAutodetectTargetRequested();

    // Controller → UI
    void onNewVisionResult(VisionResult vr);
    void onCameraConnected();
    void onCameraDisconnected();
};

#endif // APPCONTROLLER_H
