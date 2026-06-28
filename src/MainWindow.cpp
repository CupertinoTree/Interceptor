#include "MainWindow.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QTabWidget>
#include <QPushButton>
#include <QTimer>
#include <QPainter>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "ASICamera2.h"
#include <iostream>
#include <chrono>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), cameraID(-1), cameraThread(nullptr), exposureInMs(false) {
    setupUI();
    startTimers();
    int initialCameras = ASIGetNumOfConnectedCameras();
    isChasingFrame = false;
    isGuiding = false;

    // Delay camera list update to ensure UI is ready
    QTimer::singleShot(100, this, &MainWindow::updateCameraList);
}

MainWindow::~MainWindow() { resetEverything(); }

void MainWindow::setupUI() {

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Video display on the left
    videoLabel = new QLabel("No camera selected");
    videoLabel->setMinimumSize(600, 450);
    videoLabel->setStyleSheet("background-color: black; color: white; font-size: 16px; qproperty-alignment: AlignCenter;");
    mainLayout->addWidget(videoLabel, 1);

    // Tab widget on the right
    tabWidget = new QTabWidget();
    tabWidget->setMaximumWidth(300);

    // Camera tab
    QWidget* cameraTab = new QWidget();
    QVBoxLayout* cameraLayout = new QVBoxLayout(cameraTab);

    QLabel* cameraLabel = new QLabel("Camera Settings");
    cameraLabel->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(cameraLabel);

    cameraSelectCombo = new QComboBox();
    cameraSelectCombo->addItem("No Camera");
    connect(cameraSelectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::selectCamera);
    cameraLayout->addWidget(cameraSelectCombo);

    QLabel* exposureTitle = new QLabel("Exposure");
    exposureTitle->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(exposureTitle);

    exposureUnitCombo = new QComboBox();
    exposureUnitCombo->addItem("Exposure (μs)");
    exposureUnitCombo->addItem("Exposure (ms)");
    connect(exposureUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::switchExposureUnit);
    cameraLayout->addWidget(exposureUnitCombo);

    exposureSlider = new QSlider(Qt::Horizontal);
    exposureSlider->setRange(1, 1000);
    exposureSlider->setValue(1000);
    exposureSlider->setEnabled(false);
    connect(exposureSlider, &QSlider::valueChanged, this, &MainWindow::setExposure);
    cameraLayout->addWidget(exposureSlider);

    exposureLabel = new QLabel("Exposure: 1000 μs");
    cameraLayout->addWidget(exposureLabel);

    QLabel* gainTitle = new QLabel("Gain");
    gainTitle->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(gainTitle);

    gainLabel = new QLabel("Gain: 135");
    cameraLayout->addWidget(gainLabel);

    gainSlider = new QSlider(Qt::Horizontal);
    gainSlider->setRange(0, 600);
    gainSlider->setValue(135);
    gainSlider->setEnabled(false);
    connect(gainSlider, &QSlider::valueChanged, this, &MainWindow::setGain);
    cameraLayout->addWidget(gainSlider);

    fpsLabel = new QLabel("FPS: 0");
    cameraLayout->addWidget(fpsLabel);

    cameraLayout->addStretch();
    tabWidget->addTab(cameraTab, "Camera");

    // Guiding tab
    QWidget* guidingTab = new QWidget();
    QVBoxLayout* guidingLayout = new QVBoxLayout(guidingTab);

    chaseTargetButton = new QPushButton("Chase target");
    chaseTargetButton -> setCheckable(true);
    connect(chaseTargetButton, &QPushButton::clicked, this, &MainWindow::chaseTarget);
    guidingLayout->addWidget(chaseTargetButton);

    trackingStatusLabel = new QLabel("Not chasing.");
    guidingLayout->addWidget(trackingStatusLabel);

    QLabel* imagingFrameLabel = new QLabel("Imaging frame:");
    imagingFrameLabel->setStyleSheet("font-weight: bold;");
    guidingLayout->addWidget(imagingFrameLabel);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    upperLeftButton = new QPushButton("Upper Left");
    upperLeftButton->setEnabled(false);
    connect(upperLeftButton, &QPushButton::clicked, this, &MainWindow::defineUL);
    buttonsLayout->addWidget(upperLeftButton);

    lowerRightButton = new QPushButton("Lower Right");
    lowerRightButton->setEnabled(false);
    connect(lowerRightButton, &QPushButton::clicked, this, &MainWindow::defineLR);
    buttonsLayout->addWidget(lowerRightButton);
    guidingLayout->addLayout(buttonsLayout);

    resetButton = new QPushButton("Reset");
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetPrimaryROI);
    guidingLayout->addWidget(resetButton);

    guidingLayout->addStretch();
    tabWidget->addTab(guidingTab, "Guiding");

    mainLayout->addWidget(tabWidget);

    setWindowTitle("Interceptor: Satellite Optical Tracker");
    resize(900, 600);

    updateGuidingTabState();
}

void MainWindow::startTimers() {
    QTimer* fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, &MainWindow::updateFPS);
    fpsTimer->start(1000); // 1 second

    QTimer* cameraListTimer = new QTimer(this);
    connect(cameraListTimer, &QTimer::timeout, this, &MainWindow::updateCameraList);
    cameraListTimer->start(2000); // Check every 2 seconds
}

bool MainWindow::initCamera() {
        ASI_ERROR_CODE err = ASIOpenCamera(cameraID);
        if (err != ASI_SUCCESS) {
            std::cout << "Failed to open camera: " << err << std::endl;
            return false;
        }
        err = ASIInitCamera(cameraID);
        if (err != ASI_SUCCESS) {
            std::cout << "Failed to init camera: " << err << std::endl;
            ASICloseCamera(cameraID);
            return false;
        }
        ASI_CAMERA_INFO info;
        ASIGetCameraProperty(&info, cameraID);
        int width = info.MaxWidth, height = info.MaxHeight, bin = 1;
        err = ASISetROIFormat(cameraID, width, height, bin, ASI_IMG_RGB24);
        if (err != ASI_SUCCESS) {
            std::cout << "Failed to set ROI: " << err << std::endl;
            ASICloseCamera(cameraID);
            return false;
        }
        err = ASIStartVideoCapture(cameraID);
        if (err != ASI_SUCCESS) {
            std::cout << "Failed to start video capture: " << err << std::endl;
            ASICloseCamera(cameraID);
            return false;
        }

        exposureInMs = false;
        setExposure(1000); // Default 1000 μs
        setGain(135); // Default gain

        return true;
    }

void MainWindow::setupCameraThread() {
    cameraThread = new CameraThread(cameraID, this);
    connect(cameraThread, &CameraThread::frameReady, this, &MainWindow::updateFrame);
    cameraThread->startCapture();
    updateGuidingTabState();
}

void MainWindow::updateFrame() {
    if (!cameraThread) return;
    cv::Mat frame = cameraThread->getLatestFrame();
    if (!frame.empty()) {
        QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        QPixmap pix = QPixmap::fromImage(img);

        // If we're in chasing mode, try to find the target and draw a rectangle around it
        if (isChasingFrame) {
            if (Vision::findTargetPosition(frame)) {
                trackingStatusLabel->setText(QString("FWHM %1px Ecc %2").arg(int(Vision::FWHM)).arg(Vision::eccentricity, 0, 'f', 2));
                
                // Draw a green rectangle around the target
                QPainter painter(&pix);
                painter.setPen(QPen((isGuiding ? Qt::red : Qt::green), 2));
                painter.drawRect(Vision::satelliteX - 15, Vision::satelliteY - 15, 30, 30);
                painter.end();

            } else {
                trackingStatusLabel->setText("Couldn't lock on target.");
            }
        }

        // If the primary camera ROI is set, draw it on the frame
        if (Vision::primaryROISet) {
                QPainter painter(&pix);

                painter.setPen(QPen(Qt::yellow, 2));
                painter.drawRect(Vision::ROICornerAx, Vision::ROICornerAy,Vision::ROICornerBx - Vision::ROICornerAx, Vision::ROICornerBy - Vision::ROICornerAy);
        
                painter.end();
        }

        QSize targetSize = MainWindow::videoLabel -> size();
        QPixmap scaled = pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        MainWindow::videoLabel->setPixmap(scaled);

        MainWindow::videoLabel->setMinimumSize(600, 450);
    }
}

void MainWindow::updateFPS() {
    if (cameraThread) fpsLabel->setText(QString("FPS: %1").arg(cameraThread->getAndResetFrameCount()));
}

void MainWindow::setExposure(int value) {
    if (cameraID < 0) return;
    int actualValue = exposureInMs ? (value * 1000) : value;
    ASISetControlValue(cameraID, ASI_EXPOSURE, actualValue, ASI_FALSE);
    QString unit = exposureInMs ? "ms" : "μs";
    exposureLabel->setText(QString("Exposure: %1 %2").arg(value).arg(unit));
}

void MainWindow::setGain(int value) {
        if (cameraID < 0) return;
        ASISetControlValue(cameraID, ASI_GAIN, value, ASI_FALSE);
        gainLabel->setText(QString("Gain: %1").arg(value));
}

void MainWindow::switchExposureUnit(int index) {
    bool wasMs = exposureInMs;
    exposureInMs = (index == 1);
    int currentValue = exposureSlider->value();

    if (exposureInMs && !wasMs) {
        exposureSlider->setRange(1, 1000);
        exposureSlider->setValue(qBound(1, currentValue / 1000, 1000));
    } else if (!exposureInMs && wasMs) {
        exposureSlider->setRange(1, 1000);
        exposureSlider->setValue(qBound(1, currentValue * 1000, 1000));
    }
    setExposure(exposureSlider->value());
}

void MainWindow::selectCamera(int index) {
    // En gros on arrête le thread en cours
    if (cameraThread) {
        resetEverything();
    }

    if (index <= 0) {
        cameraID = -1;
        videoLabel->clear(); // Clear any pixmap
        videoLabel->setText("No camera selected");
        videoLabel->setStyleSheet("background-color: black; color: white; font-size: 16px; qproperty-alignment: AlignCenter;");
        exposureSlider->setEnabled(false);
        gainSlider->setEnabled(false);
        updateGuidingTabState();
        return;
    }

    cameraID = index - 1;

    if (initCamera()) {
        setupCameraThread();
        videoLabel->setText("");
        videoLabel->setStyleSheet("");
        exposureSlider->setEnabled(true);
        gainSlider->setEnabled(true);
    } else {
        cameraID = -1;
        videoLabel->clear(); // Clear any pixmap
        videoLabel->setText("No camera selected");
        videoLabel->setStyleSheet("background-color: black; color: white; font-size: 16px; qproperty-alignment: AlignCenter;");
        exposureSlider->setEnabled(false);
        gainSlider->setEnabled(false);
        updateGuidingTabState();
    }
}

void MainWindow::updateCameraList() {
    static int lastNumCameras = -1;
    int numCameras = ASIGetNumOfConnectedCameras();

    // Only update if the number of cameras has changed
    if (numCameras == lastNumCameras) {
        // Even if count hasn't changed, check if current camera is still valid
        if (cameraID >= 0 && cameraID >= numCameras) {
            std::cout << "Current camera " << cameraID << " is no longer available" << std::endl;
            cameraSelectCombo->setCurrentIndex(0);
            selectCamera(0);
        }
        return;
    }

    std::cout << "Camera count changed from " << lastNumCameras << " to " << numCameras << std::endl;
    lastNumCameras = numCameras;

    cameraSelectCombo->blockSignals(true);
    int currentIndex = cameraSelectCombo->currentIndex();
    bool wasCameraSelected = (currentIndex > 0);

    cameraSelectCombo->clear();
    cameraSelectCombo->addItem("No Camera");

    for (int i = 0; i < numCameras; ++i) {
        ASI_CAMERA_INFO info;
        ASIGetCameraProperty(&info, i);
        cameraSelectCombo->addItem(QString("Camera %1: %2").arg(i).arg(info.Name));
    }

    // Handle camera disconnection
        if (wasCameraSelected && currentIndex > numCameras) {
            // Camera was disconnected, switch to "No Camera"
            cameraSelectCombo->setCurrentIndex(0);
            selectCamera(0); // Force update
        } else if (currentIndex > 0 && currentIndex <= numCameras) {
            // Camera still exists, restore selection
            cameraSelectCombo->setCurrentIndex(currentIndex);
        } else {
            // No camera was selected or invalid selection
            cameraSelectCombo->setCurrentIndex(0);
        }
        
        cameraSelectCombo->blockSignals(false);
}

void MainWindow::resetEverything() {

    if (cameraThread) {
        cameraThread->stopCapture();
        cameraThread->wait();
        delete cameraThread;
        cameraThread = nullptr;
    }

    if (cameraID >= 0) {
        ASIStopVideoCapture(cameraID);
        ASICloseCamera(cameraID);
        cameraID = -1;
    }

    isChasingFrame = false;
    chaseTargetButton -> setChecked(false);
    isGuiding = false;
    trackingStatusLabel->setText("Not chasing.");

    upperLeftButton->setEnabled(false);
    lowerRightButton->setEnabled(false);

    resetPrimaryROI();

    updateGuidingTabState();
}

void MainWindow::chaseTarget() {

    if (isGuiding) { return; }

    if (isChasingFrame) {
        trackingStatusLabel->setText("Not chasing.");
        chaseTargetButton -> setText("Chase target");
    } else {
        chaseTargetButton -> setText("Stop chasing");
    }

    isChasingFrame = !isChasingFrame;
    chaseTargetButton -> setChecked(isChasingFrame);

    upperLeftButton->setEnabled(isChasingFrame);
    lowerRightButton->setEnabled(isChasingFrame);
}

void MainWindow::defineUL() {
    if (!isChasingFrame || Vision::primaryROISet || isGuiding) return;
    
    if (Vision::ROICornerBx != -1) {
        if (Vision::satelliteX > Vision::ROICornerBx || Vision::satelliteY > Vision::ROICornerBy) {
            int tempX = Vision::ROICornerBx;
            int tempY = Vision::ROICornerBy;
            Vision::ROICornerBx = Vision::satelliteX;
            Vision::ROICornerBy = Vision::satelliteY;
            Vision::ROICornerAx = tempX;
            Vision::ROICornerAy = tempY;
        } else {
            Vision::ROICornerAx = Vision::satelliteX;
            Vision::ROICornerAy = Vision::satelliteY;
        }

        Vision::primaryCenterX = (Vision::ROICornerAx + Vision::ROICornerBx) / 2;
        Vision::primaryCenterY = (Vision::ROICornerAy + Vision::ROICornerBy) / 2;
        Vision::primaryROISet = true;
        upperLeftButton->setEnabled(false);
        lowerRightButton->setEnabled(false);
    } else {
        Vision::ROICornerAx = Vision::satelliteX;
        Vision::ROICornerAy = Vision::satelliteY;
    }

    upperLeftButton -> setText("X: " + QString::number(Vision::ROICornerAx) + " Y: " + QString::number(Vision::ROICornerAy));
}

void MainWindow::defineLR() {
    if (!isChasingFrame || Vision::primaryROISet  || isGuiding) return;
    
    if (Vision::ROICornerAx != -1) {
        if (Vision::satelliteX < Vision::ROICornerAx || Vision::satelliteY < Vision::ROICornerAy) {
            int tempX = Vision::ROICornerAx;
            int tempY = Vision::ROICornerAy;
            Vision::ROICornerAx = Vision::satelliteX;
            Vision::ROICornerAy = Vision::satelliteY;
            Vision::ROICornerBx = tempX;
            Vision::ROICornerBy = tempY;
        } else {
            Vision::ROICornerBx = Vision::satelliteX;
            Vision::ROICornerBy = Vision::satelliteY;
        }

        Vision::primaryCenterX = (Vision::ROICornerAx + Vision::ROICornerBx) / 2;
        Vision::primaryCenterY = (Vision::ROICornerAy + Vision::ROICornerBy) / 2;
        Vision::primaryROISet = true;
        upperLeftButton->setEnabled(false);
        lowerRightButton->setEnabled(false);
    } else {
        Vision::ROICornerBx = Vision::satelliteX;
        Vision::ROICornerBy = Vision::satelliteY;
    } 

    lowerRightButton -> setText("X: " + QString::number(Vision::ROICornerBx) + " Y: " + QString::number(Vision::ROICornerBy));
}

void MainWindow::resetPrimaryROI() {

    if (isGuiding) return;

    Vision::ROICornerAx = -1;
    Vision::ROICornerAy = -1;
    Vision::ROICornerBx = -1;
    Vision::ROICornerBy = -1;
    Vision::primaryCenterX = -1;
    Vision::primaryCenterY = -1;
    Vision::primaryROISet = false;
    upperLeftButton->setText("Upper Left");
    upperLeftButton->setEnabled(true);
    lowerRightButton->setText("Lower Right");
    lowerRightButton->setEnabled(true);
}

void MainWindow::updateGuidingTabState() {
    bool cameraRunning = (cameraThread != nullptr && cameraThread->isRunning());
    tabWidget->setTabEnabled(1, cameraRunning); // Guiding tab is index 1
}