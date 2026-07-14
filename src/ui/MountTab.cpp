#include "MountTab.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QComboBox>

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

    QSizePolicy createFixedPolicy()
    {
        return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    QPushButton* createArrowButton(const QString& text, const QSize& size)
    {
        QPushButton* button = new QPushButton(text);
        button->setEnabled(false);
        button->setFixedSize(size);
        button->setSizePolicy(createFixedPolicy());
        button->setStyleSheet(
            "QPushButton {"
            "font-size: 20px;"
            "font-weight: bold;"
            "padding: 0px;"
            "border: 1px solid #b0b0b0;"
            "background: #e0e0e0;"
            "}"
        );
        return button;
    }

    QPushButton* createBottomButton(const QString& text, const QSize& size)
    {
        QPushButton* button = new QPushButton(text);
        button->setEnabled(false);
        button->setFixedSize(size);
        button->setSizePolicy(createFixedPolicy());
        button->setStyleSheet(
            "QPushButton {"
            "font-size: 13px;"
            "border: 1px solid #b0b0b0;"
            "background: #e0e0e0;"
            "}"
        );
        return button;
    }

}

MountTab::MountTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

void MountTab::setupUI() {
    auto mountLayout = new QVBoxLayout(this);
    mountLayout->setContentsMargins(6, 6, 6, 6);

    QWidget* contentWidget = new QWidget;
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    //contentLayout->setSpacing(0);
    contentLayout->setAlignment(Qt::AlignTop);

    QLabel* mountLabel = new QLabel("Mount Tab");
    mountLabel->setStyleSheet(
        "font-weight: bold;"
        "font-size: 18px"
    );
    contentLayout->addWidget(mountLabel);
    contentLayout->addSpacing(10);

    //
    // Mount connection
    //
    QGroupBox* connectionGroup = createStyledGroupBox("Connection");
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);
    connectionLayout->setContentsMargins(6, 6, 6, 6);

    portPathLine = new QLineEdit("/dev/cu.usbmodem8F8B53710E311");
    portPathLine->setPlaceholderText("/dev/cu.usbmodem8F8B53710E311");
    connectionLayout->addWidget(portPathLine);

    connectButton = new QPushButton("Connect");
    connectionLayout->addWidget(connectButton);

    contentLayout->addWidget(connectionGroup);
    contentLayout->addSpacing(10);

    //
    // Observer location
    //
    QGroupBox* locationGroup = createStyledGroupBox("Location");
    QVBoxLayout* locationLayout = new QVBoxLayout(locationGroup);
    locationLayout->setContentsMargins(6, 6, 6, 6);
    locationLayout->setSpacing(4);

    QGridLayout* locationTxtLayout = new QGridLayout();
    locationTxtLayout->setContentsMargins(4, 4, 4, 4);
    locationTxtLayout->setHorizontalSpacing(6);
    locationTxtLayout->setVerticalSpacing(6);

    latitudeLine = new QLineEdit("0.0");
    latitudeLine->setPlaceholderText("Latitude");

    longitudeLine = new QLineEdit("0.0");
    longitudeLine->setPlaceholderText("Longitude");

    latitudeLabel = new QLabel("Lat : 0.0°");
    longitudeLabel = new QLabel("Long : 0.0°");

    locationTxtLayout->addWidget(latitudeLine, 0, 0);
    locationTxtLayout->addWidget(longitudeLine, 0, 1);
    locationTxtLayout->addWidget(latitudeLabel, 1, 0);
    locationTxtLayout->addWidget(longitudeLabel, 1, 1);

    locationLayout->addLayout(locationTxtLayout);

    setLocationButton = new QPushButton("Set location");

    locationLayout->addWidget(setLocationButton);

    contentLayout->addWidget(locationGroup);
    contentLayout->addSpacing(10);

    //
    // Mount alignment
    //
    QGroupBox* alignementGroup = createStyledGroupBox("Star alignment");
    QVBoxLayout* alignmentLayout = new QVBoxLayout(alignementGroup);
    alignmentLayout->setContentsMargins(6, 6, 6, 6);
    alignmentLayout->setSpacing(4);

    starSelectCombo = new QComboBox();
    starSelectCombo->addItem("Select star");
    alignmentLayout->addWidget(starSelectCombo);

    starSelected = new QLabel("No star selected");
    alignmentLayout->addWidget(starSelected);

    syncToStar = new QPushButton("Sync to star");
    alignmentLayout->addWidget(syncToStar);

    QHBoxLayout* solveLayout = new QHBoxLayout();
    
    solveAlignment = new QPushButton("Solve model");
    solveAlignment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    resetAlignment = new QPushButton("Reset");
    resetAlignment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    solveLayout->addWidget(solveAlignment, 2);
    solveLayout->addWidget(resetAlignment, 1);

    alignmentLayout->addLayout(solveLayout);

    alignResult = new QLabel("Model not solved.");
    alignmentLayout->addWidget(alignResult);

    contentLayout->addWidget(alignementGroup);
    contentLayout->addSpacing(10);

    //
    // GOTO Interface
    //
    QGroupBox* gotoGroup = createStyledGroupBox("GOTO Controls");
    QVBoxLayout* gotoLayout = new QVBoxLayout(gotoGroup);

    QLabel* raLabel = new QLabel("Target RA HMS:");
    gotoLayout->addWidget(raLabel);

    QHBoxLayout* raLayout = new QHBoxLayout();

    raHour = new QLineEdit("0");
    raHour->setPlaceholderText("0");
    QLabel* raHourLabel = new QLabel("H");
    raMin = new QLineEdit("0");
    raMin->setPlaceholderText("0");
    QLabel* raMinLabel = new QLabel("M");
    raSec = new QLineEdit("0.0");
    raSec->setPlaceholderText("0.0");
    QLabel* raSecLabel = new QLabel("S");
    raLayout->addWidget(raHour);
    raLayout->addWidget(raHourLabel);
    raLayout->addWidget(raMin);
    raLayout->addWidget(raMinLabel);
    raLayout->addWidget(raSec);
    raLayout->addWidget(raSecLabel);

    gotoLayout->addLayout(raLayout);

    QLabel* deLabel = new QLabel("Target DE DMS:");
    gotoLayout->addWidget(deLabel);

    QHBoxLayout* deLayout = new QHBoxLayout();

    deDeg = new QLineEdit("0");
    deDeg->setPlaceholderText("0");
    QLabel* deDegLabel = new QLabel("D");
    deMin = new QLineEdit("0");
    deMin->setPlaceholderText("0");
    QLabel* deMinLabel = new QLabel("M");
    deSec = new QLineEdit("0.0");
    deSec->setPlaceholderText("0.0");
    QLabel* deSecLabel = new QLabel("S");
    deLayout->addWidget(deDeg);
    deLayout->addWidget(deDegLabel);
    deLayout->addWidget(deMin);
    deLayout->addWidget(deMinLabel);
    deLayout->addWidget(deSec);
    deLayout->addWidget(deSecLabel);

    gotoLayout->addLayout(deLayout);

    gotoTarget = new QPushButton("GOTO Target");
    gotoLayout->addWidget(gotoTarget);

    gotoHome = new QPushButton("GOTO Home position");
    gotoLayout->addWidget(gotoHome);

    contentLayout->addWidget(gotoGroup);

    //
    // End of scrollable part
    //
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameStyle(QFrame::NoFrame);
    scroll->setWidget(contentWidget);

    mountLayout->addWidget(scroll);

    //
    // Slew controls (anchored to bottom)
    //
    QGroupBox* slewGroup = new QGroupBox("Slew controls");
    slewGroup->setStyleSheet(
        "QGroupBox {"
        "font-size: 14px;"
        "font-weight: bold;"
        "background-color: #f9f9f9;"
        "}"
    );

    QGridLayout* slewBtnLayout = new QGridLayout(slewGroup);
    slewBtnLayout->setContentsMargins(4, 4, 4, 4);
    slewBtnLayout->setHorizontalSpacing(6);
    slewBtnLayout->setVerticalSpacing(6);

    const QSize btnSize(60, 40);

    slewUpButton    = createArrowButton("↑", btnSize);
    slewLeftButton  = createArrowButton("←", btnSize);
    slewRightButton = createArrowButton("→", btnSize);
    slewDownButton  = createArrowButton("↓", btnSize);

    slewSpeedLabel = new QLabel("800x");
    slewSpeedLabel->setAlignment(Qt::AlignCenter);
    slewSpeedLabel->setFixedSize(btnSize);
    slewSpeedLabel->setSizePolicy(createFixedPolicy());
    slewSpeedLabel->setStyleSheet(
        "QLabel {"
        "font-size: 13px;"
        "font-weight: bold;"
        "border: 1px solid #b0b0b0;"
        "background: #e0e0e0;"
        "}"
    );

    // Layout
    slewBtnLayout->addWidget(slewUpButton,    0, 1);
    slewBtnLayout->addWidget(slewLeftButton,  1, 0);
    slewBtnLayout->addWidget(slewSpeedLabel,  1, 1);
    slewBtnLayout->addWidget(slewRightButton, 1, 2);
    slewBtnLayout->addWidget(slewDownButton,  2, 1);

    // Bottom row buttons
    slewSlowerButton = createBottomButton("Slower", btnSize);
    slewFasterButton = createBottomButton("Faster", btnSize);

    stopButton = new QPushButton("STOP");
    stopButton->setEnabled(false);
    stopButton->setFixedSize(btnSize);
    stopButton->setSizePolicy(createFixedPolicy());
    stopButton->setStyleSheet(
        "QPushButton {"
        "background-color: #b00000;"
        "color: white;"
        "font-size: 14px;"
        "font-weight: bold;"
        "border: 1px solid #888;"
        "}"
    );

    slewBtnLayout->addWidget(slewSlowerButton, 3, 0);
    slewBtnLayout->addWidget(stopButton,       3, 1);
    slewBtnLayout->addWidget(slewFasterButton, 3, 2);

    mountLayout->addWidget(slewGroup);



    setLayout(mountLayout);
}

void MountTab::setupConnections() {

    connect(connectButton, &QPushButton::clicked, this, [this] {
        emit mountConnectRequested(portPathLine->text().toStdString());
    });

}

void MountTab::setMountConnected() {
    portPathLine->setReadOnly(true);
    connectButton->setChecked(true);
    connectButton->setText("Connected");
}