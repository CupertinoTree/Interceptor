#ifndef CAMERATAB_H
#define CAMERATAB_H

#include <QWidget>
#include "model/Enums.h"
#include "Vision.h"

class QLabel;
class QSlider;
class QComboBox;
class QPushButton;

class CameraTab : public QWidget {
    Q_OBJECT
public:
    explicit CameraTab(QWidget* parent = nullptr);

    void setFPS(int fps);
    void setAutodetect(bool autodetect);
    void updateVisionResult(VisionResult vr);
    void setUpperLeft(QString coords);
    void setLowerRight(QString coords);
    void resetROI();
    void setROI();
    void setGainValue(int value);
    void setExposureValue(int value, QString unit);
    void setCameraDisconnected();
    void setCameraConnected();
    void setCameraListEmpty();
    void setCameraList(QStringList list);

signals:
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

    QLabel* fpsLabel;
    QLabel * cameraLabel;
    QLabel * exposureLabel;
    QLabel * gainLabel;
    QLabel * autodetectStatusLabel;
    QSlider* exposureSlider;
    QSlider* gainSlider;
    QComboBox* cameraSelectCombo;
    QComboBox* exposureUnitCombo;

    QPushButton* upperLeftButton;
    QPushButton* lowerRightButton;
    QPushButton* resetROIButton;
    QPushButton* autodetectTargetButton;
};

#endif // CAMERATAB_H
