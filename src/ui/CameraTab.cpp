#include "CameraTab.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include "QComboBox"
#include "QPushButton"
#include <QVariant>
#include "model/Enums.h"

CameraTab::CameraTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

void CameraTab::setupUI()
{   
    auto cameraLayout = new QVBoxLayout(this);

    cameraLabel = new QLabel("Camera Settings");
    cameraLabel->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(cameraLabel);

    cameraSelectCombo = new QComboBox();
    cameraSelectCombo->addItem("No Camera");
    cameraLayout->addWidget(cameraSelectCombo);

    QLabel* exposureTitle = new QLabel("Exposure");
    exposureTitle->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(exposureTitle);

    exposureUnitCombo = new QComboBox();
    exposureUnitCombo->addItem("Exposure (μs)", QVariant::fromValue(ExposureUnit::μs));
    exposureUnitCombo->addItem("Exposure (ms)", QVariant::fromValue(ExposureUnit::ms));
    cameraLayout->addWidget(exposureUnitCombo);

    exposureSlider = new QSlider(Qt::Horizontal);
    exposureSlider->setRange(1, 1000);
    exposureSlider->setValue(1);
    exposureSlider->setEnabled(false);
    cameraLayout->addWidget(exposureSlider);

    exposureLabel = new QLabel("Exposure: 1 μs");
    cameraLayout->addWidget(exposureLabel);

    QLabel* gainTitle = new QLabel("Gain");
    gainTitle->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(gainTitle);

    gainLabel = new QLabel("Gain: 0");
    cameraLayout->addWidget(gainLabel);

    gainSlider = new QSlider(Qt::Horizontal);
    gainSlider->setRange(0, 600);
    gainSlider->setValue(0);
    gainSlider->setEnabled(false);
    cameraLayout->addWidget(gainSlider);

    fpsLabel = new QLabel("FPS: 0", this);
    cameraLayout->addWidget(fpsLabel);

    autodetectTargetButton = new QPushButton("Auto-detect target");
    autodetectTargetButton->setCheckable(false);
    cameraLayout->addWidget(autodetectTargetButton);

    autodetectStatusLabel = new QLabel("Auto-detecting off.");
    cameraLayout->addWidget(autodetectStatusLabel);

    QLabel* primaryCameraFOVLabel = new QLabel("Primary camera FOV:");
    primaryCameraFOVLabel->setStyleSheet("font-weight: bold;");
    cameraLayout->addWidget(primaryCameraFOVLabel);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();

    upperLeftButton = new QPushButton("Upper Left");
    upperLeftButton->setEnabled(false);
    buttonsLayout->addWidget(upperLeftButton);

    lowerRightButton = new QPushButton("Lower Right");
    lowerRightButton->setEnabled(false);
    buttonsLayout->addWidget(lowerRightButton);

    cameraLayout->addLayout(buttonsLayout);

    resetROIButton = new QPushButton("Reset");
    cameraLayout->addWidget(resetROIButton);

    cameraLayout->addStretch();
    setLayout(cameraLayout);
}

void CameraTab::setupConnections()
{
    connect(cameraSelectCombo, &QComboBox::currentIndexChanged,
            this, &CameraTab::cameraConnectRequested);

    connect(exposureSlider, &QSlider::valueChanged,
            this, &CameraTab::exposureChangeRequested);

    connect(gainSlider, &QSlider::valueChanged,
            this, &CameraTab::gainChangeRequested);

    connect(exposureUnitCombo, &QComboBox::currentIndexChanged,
            this, [this](int index) {
                auto unit = exposureUnitCombo->itemData(index).value<ExposureUnit>();

            exposureUnitChangeRequested(unit);
            });

    connect(upperLeftButton, &QPushButton::clicked,
            this, &CameraTab::defineULRequested);

    connect(lowerRightButton, &QPushButton::clicked,
            this, &CameraTab::defineLRRequested);

    connect(resetROIButton, &QPushButton::clicked,
            this, &CameraTab::resetROI);

    connect(autodetectTargetButton, &QPushButton::clicked,
            this, &CameraTab::autodetectTargetRequested);
}

void CameraTab::setGainValue(int value)
{
    gainLabel->setText(QString("Gain: %1").arg(value));
    if (gainSlider->value() != value) { gainSlider->setValue(value); }
}

void CameraTab::setExposureValue(int value, QString unit)
{   
    exposureLabel->setText(QString("Exposure: %1 %2").arg(value).arg(unit));

    if (exposureSlider->value() != value) { exposureSlider->setValue(value); }

    //Check that the exposure unit combo box is set to the correct unit
    if (exposureUnitCombo->currentData().value<ExposureUnit>() == ExposureUnit::μs && unit == "ms") {
        exposureUnitCombo->setCurrentIndex(static_cast<int>(ExposureUnit::ms));
    } else if (exposureUnitCombo->currentData().value<ExposureUnit>() == ExposureUnit::ms && unit == "μs") {
        exposureUnitCombo->setCurrentIndex(static_cast<int>(ExposureUnit::μs));
    }
}

void CameraTab::updateVisionResult(VisionResult vr) {
    if (vr.found) {
        autodetectStatusLabel->setText(QString("FWHM %1px Ecc %2").arg(int(vr.fwhm)).arg(vr.ecc, 0, 'f', 2));
    } else {
        autodetectStatusLabel->setText("Couldn't lock on target.");
    }
}

void CameraTab::setUpperLeft(QString coords) {
    upperLeftButton -> setText(coords);
    upperLeftButton->setChecked(true);
    resetROIButton->setEnabled(true);
}

void CameraTab::setLowerRight(QString coords) {
    lowerRightButton -> setText(coords);
    lowerRightButton->setChecked(true);
    resetROIButton->setEnabled(true);
}

void CameraTab::setROI() {
    upperLeftButton->setEnabled(false);
    upperLeftButton->setChecked(false);
    lowerRightButton->setEnabled(false);
    lowerRightButton->setChecked(false);

    resetROIButton->setEnabled(false);
}

void CameraTab::resetROI() {
    //if (isGuiding) return;

    upperLeftButton->setText("Upper Left");
    upperLeftButton->setChecked(false);
    lowerRightButton->setText("Lower Right");
    lowerRightButton->setChecked(false);

    emit roiResetRequested();
}

void CameraTab::setFPS(int fps)
{
    fpsLabel->setText(QString("FPS: %1").arg(fps));
}

void CameraTab::setAutodetect(bool autodetect) {

    if (autodetect) {
        autodetectTargetButton -> setText("Autodetecting...");
    } else {
        autodetectStatusLabel->setText("Autodetect off.");
        autodetectTargetButton -> setText("Autodetect target");
    }

    autodetectTargetButton -> setChecked(autodetect);

    upperLeftButton->setEnabled(autodetect);
    lowerRightButton->setEnabled(autodetect);
}

void CameraTab::setCameraConnected()
{
    exposureSlider->setEnabled(true);
    gainSlider->setEnabled(true);
    exposureUnitCombo->setEnabled(true);
    autodetectTargetButton->setEnabled(true);
}

void CameraTab::setCameraDisconnected()
{
    fpsLabel->setText("FPS: 0");
    exposureSlider->setEnabled(false);
    gainSlider->setEnabled(false);
    exposureUnitCombo->setEnabled(false);
    upperLeftButton->setEnabled(false);
    lowerRightButton->setEnabled(false);
    autodetectTargetButton->setEnabled(false);

    cameraSelectCombo->setCurrentIndex(0); // Reset to "No Camera"
}

void CameraTab::setCameraListEmpty() {
    cameraSelectCombo->clear();
    cameraSelectCombo->addItem("No Camera");
}

void CameraTab::setCameraList(QStringList list) {
    cameraSelectCombo->clear();
    cameraSelectCombo->addItem("No Camera");

    for (const QString& camera : list) {
        cameraSelectCombo->addItem(camera);
    }
}