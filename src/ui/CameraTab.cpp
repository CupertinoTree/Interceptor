#include "CameraTab.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVariant>
#include "model/Enums.h"

namespace {

QGroupBox* createStyledGroupBox(const QString& title)
{
    QGroupBox* box = new QGroupBox(title);
    box->setStyleSheet(
        "QGroupBox {"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
    );
    return box;
}

} // namespace

CameraTab::CameraTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

void CameraTab::setupUI()
{   
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 0);

    QWidget* contentWidget = new QWidget;
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(6, 6, 6, 0);
    contentLayout->setAlignment(Qt::AlignTop);

    QLabel* cameraLabel = new QLabel("Camera Tab");
    cameraLabel->setStyleSheet(
        "font-weight: bold;"
        "font-size: 18px"
    );
    contentLayout->addWidget(cameraLabel);
    contentLayout->addSpacing(5);

    cameraSelectCombo = new QComboBox();
    cameraSelectCombo->addItem("No Camera");
    contentLayout->addWidget(cameraSelectCombo);
    contentLayout->addSpacing(10);

    QGroupBox* exposureGroup = createStyledGroupBox("Exposure");
    QVBoxLayout* exposureLayout = new QVBoxLayout(exposureGroup);
    exposureLayout->setContentsMargins(6, 6, 6, 6);

    exposureUnitCombo = new QComboBox();
    exposureUnitCombo->addItem("Exposure (μs)", QVariant::fromValue(ExposureUnit::μs));
    exposureUnitCombo->addItem("Exposure (ms)", QVariant::fromValue(ExposureUnit::ms));
    exposureUnitCombo->setEnabled(false);
    exposureLayout->addWidget(exposureUnitCombo);

    exposureSlider = new QSlider(Qt::Horizontal);
    exposureSlider->setRange(1, 1000);
    exposureSlider->setValue(1);
    exposureSlider->setEnabled(false);
    exposureLayout->addWidget(exposureSlider);

    exposureLabel = new QLabel("Exposure: 1 μs");
    exposureLayout->addWidget(exposureLabel);

    contentLayout->addWidget(exposureGroup);
    contentLayout->addSpacing(10);

    QGroupBox* gainGroup = createStyledGroupBox("Gain");
    QVBoxLayout* gainLayout = new QVBoxLayout(gainGroup);
    gainLayout->setContentsMargins(6, 6, 6, 6);

    gainSlider = new QSlider(Qt::Horizontal);
    gainSlider->setRange(0, 600);
    gainSlider->setValue(0);
    gainSlider->setEnabled(false);
    gainLayout->addWidget(gainSlider);

    gainLabel = new QLabel("Gain: 0");
    gainLayout->addWidget(gainLabel);

    contentLayout->addWidget(gainGroup);
    contentLayout->addSpacing(10);

    fpsLabel = new QLabel("FPS: 0", this);
    contentLayout->addWidget(fpsLabel);

    autodetectTargetButton = new QPushButton("Auto-detect target");
    autodetectTargetButton->setCheckable(false);
    autodetectTargetButton->setEnabled(false);
    contentLayout->addWidget(autodetectTargetButton);

    autodetectStatusLabel = new QLabel("Auto-detecting off.");
    contentLayout->addWidget(autodetectStatusLabel);
    contentLayout->addSpacing(10);

    QGroupBox* ROIGroup = createStyledGroupBox("Primary camera FOV:");
    QVBoxLayout* ROILayout = new QVBoxLayout(ROIGroup);
    ROILayout->setContentsMargins(6, 6, 6, 0);
    ROILayout->setSpacing(4);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();

    upperLeftButton = new QPushButton("Upper Left");
    upperLeftButton->setEnabled(false);
    buttonsLayout->addWidget(upperLeftButton);

    lowerRightButton = new QPushButton("Lower Right");
    lowerRightButton->setEnabled(false);
    buttonsLayout->addWidget(lowerRightButton);

    ROILayout->addLayout(buttonsLayout);

    resetROIButton = new QPushButton("Reset");
    ROILayout->addWidget(resetROIButton);

    contentLayout->addWidget(ROIGroup);

    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameStyle(QFrame::NoFrame);
    scroll->setWidget(contentWidget);

    mainLayout->addWidget(scroll);
    setLayout(mainLayout);
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