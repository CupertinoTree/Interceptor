#include "MainWindow.h"
#include "CameraTab.h"
#include "Vision.h"
#include <QLabel>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QDialog>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    auto layout = new QHBoxLayout(this);

    videoLabel = new QLabel("No camera selected");
    videoLabel->setMinimumSize(600, 450);
    videoLabel->setStyleSheet("background-color: black; color: white; font-size: 16px; qproperty-alignment: AlignCenter;");
    layout->addWidget(videoLabel, 1);

    tabWidget = new QTabWidget(this);
    tabWidget->setMaximumWidth(300);

    cameraTab = new CameraTab(this);
    tabWidget->addTab(cameraTab, "Camera");
    layout->addWidget(tabWidget);

    setLayout(layout);

    setWindowTitle("Interceptor: Optical Satellite Tracker");
    resize(900, 600);
}

void MainWindow::setupConnections()
{
    connect(cameraTab, &CameraTab::cameraConnectRequested,
            this, &MainWindow::cameraConnectRequested);

    connect(cameraTab, &CameraTab::exposureChangeRequested,
            this, &MainWindow::exposureChangeRequested);

    connect(cameraTab, &CameraTab::gainChangeRequested,
            this, &MainWindow::gainChangeRequested);

    connect(cameraTab, &CameraTab::exposureUnitChangeRequested,
            this, &MainWindow::exposureUnitChangeRequested);

    connect(cameraTab, &CameraTab::defineULRequested,
            this, &MainWindow::defineULRequested);

    connect(cameraTab, &CameraTab::defineLRRequested,
            this, &MainWindow::defineLRRequested);

    connect(cameraTab, &CameraTab::roiResetRequested,
            this, &MainWindow::roiResetRequested);

    connect(cameraTab, &CameraTab::autodetectTargetRequested,
            this, &MainWindow::autodetectTargetRequested);
}

void MainWindow::showErrorDialog(const QString &message)
{
    if (!errorDialog)
    {
        errorDialog = new QDialog(this);
        errorDialog->setWindowTitle("Error");
        errorDialog->setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(errorDialog);

        errorLabel = new QLabel(errorDialog);
        errorLabel->setWordWrap(true);
        layout->addWidget(errorLabel);

        QPushButton *dismissButton = new QPushButton("Dismiss", errorDialog);
        layout->addWidget(dismissButton);

        connect(dismissButton, &QPushButton::clicked, errorDialog, &QDialog::hide);
    }

    errorLabel->setText(message);
    errorDialog->show();
    errorDialog->raise();
    errorDialog->activateWindow();
}


//
// Called by AppController → updates UI
//
void MainWindow::setFrame(const QPixmap& frame)
{
    QSize targetSize = videoLabel -> size();
    QPixmap scaled = frame.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    videoLabel->setPixmap(scaled);
}

void MainWindow::updateVisionResult(VisionResult vr) {
    cameraTab->updateVisionResult(vr);
}

void MainWindow::setUpperLeft(QString coords) {
    cameraTab->setUpperLeft(coords);
}

void MainWindow::setLowerRight(QString coords) {
    cameraTab->setLowerRight(coords);
}

void MainWindow::setROI() {
    cameraTab->setROI();
}

void MainWindow::setFPS(int fps)
{
    cameraTab->setFPS(fps);
}

void MainWindow::setAutodetect(bool autodetect) {
    cameraTab->setAutodetect(autodetect);
}

void MainWindow::onGainApplied(int value)
{
    cameraTab->setGainValue(value);
}

void MainWindow::onExposureTimeApplied(int value, QString unit)
{
    cameraTab->setExposureValue(value, unit);
}

void MainWindow::setCameraConnected() {
    videoLabel->setText("");
    videoLabel->setStyleSheet("");

    cameraTab->setCameraConnected();
}

void MainWindow::setCameraDisconnected() {
    videoLabel->clear(); // Clear any pixmap
    videoLabel->setText("No camera selected");
    videoLabel->setStyleSheet("background-color: black; color: white; font-size: 16px; qproperty-alignment: AlignCenter;");

    cameraTab->setCameraDisconnected();
    //updateGuidingTabState();
}

void MainWindow::setCameraListEmpty() {
    cameraTab->setCameraListEmpty();
}

void MainWindow::setCameraList(QStringList list) {
    cameraTab->setCameraList(list);
}