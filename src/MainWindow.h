#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QTabWidget>
#include <QPushButton>
#include <QTimer>
#include <QPainter>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "CameraThread.h"
#include "Vision.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void updateFrame();
    void updateFPS();
    void setExposure(int value);
    void setGain(int value);
    void switchExposureUnit(int index);
    void selectCamera(int index);
    void updateCameraList();
    void chaseTarget();
    void defineUL();
    void defineLR();
    void resetPrimaryROI();
    void resetEverything();

private:
    void setupUI();
    void startTimers();
    bool initCamera();
    void setupCameraThread();
    void updateGuidingTabState();

    int cameraID;
    CameraThread* cameraThread;
    bool exposureInMs;
    bool isChasingFrame, isGuiding;

    // UI elements
    QLabel* videoLabel;
    QLabel* fpsLabel;
    QLabel* exposureLabel;
    QLabel * gainLabel;
    QSlider* exposureSlider;
    QSlider* gainSlider;
    QComboBox* cameraSelectCombo;
    QComboBox* exposureUnitCombo;
    QTabWidget* tabWidget;

    // Guiding tab elements
    QPushButton* chaseTargetButton;
    QLabel* trackingStatusLabel;
    QPushButton* upperLeftButton;
    QPushButton* lowerRightButton;
    QPushButton* resetButton;
};

#endif // MAINWINDOW_H