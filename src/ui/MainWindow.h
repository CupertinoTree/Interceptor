#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "model/Enums.h"
#include "model/Vision.h"

class QLabel;
class QSlider;
class QComboBox;
class QPushButton;
class QTabWidget;
class CameraTab;
class MountTab;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setFrame(const QPixmap& frame);
    void setAutodetect(bool autodetect);
    void updateVisionResult(VisionResult vr);
    void setFPS(int fps);
    void setUpperLeft(QString coords);
    void setLowerRight(QString coords);
    void setROI();
    //void setTabsEnabled(bool cameraEnabled, bool mountEnabled, bool trackingEnabled);
    void showErrorDialog(const QString& message);

    void onGainApplied(int value);
    void onExposureTimeApplied(int value, QString unit);
    void setCameraConnected();
    void setCameraDisconnected();
    void setCameraListEmpty();
    void setCameraList(QStringList list);

signals:
    // CameraTab → AppController
    void cameraConnectRequested(int cameraIndex);
    void exposureChangeRequested(int value);
    void gainChangeRequested(int value);
    void exposureUnitChangeRequested(ExposureUnit unit);
    void defineULRequested();
    void defineLRRequested();
    void roiResetRequested();
    void autodetectTargetRequested();

private:
    void setupUI();
    void setupConnections();

    QLabel* videoLabel;
    QTabWidget* tabWidget;

    QDialog *errorDialog = nullptr;
    QLabel *errorLabel = nullptr;

    CameraTab* cameraTab;
    MountTab* mountTab;
};

#endif // MAINWINDOW_H
