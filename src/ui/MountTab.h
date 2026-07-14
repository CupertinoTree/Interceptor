#ifndef MOUNTTAB_H
#define MOUNTTAB_H

#include <QWidget>

class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;

class MountTab : public QWidget {
    Q_OBJECT
public:
    explicit MountTab(QWidget* parent = nullptr);

    void setMountConnected();

signals:
    void mountConnectRequested(std::string port);
    void slewFasterRequested();
    void slewSlowerRequested();
    void slewUpPressed();
    void slewLeftPressed();
    void slewRightPressed();
    void slewDownPressed();
    void slewBtnReleased();
    void stopSlewPressed();

private:
    void setupConnections();
    void setupUI();

    QLabel* slewSpeedLabel;
    QLabel* latitudeLabel;
    QLabel* longitudeLabel;
    QLabel* starSelected;
    QLabel* alignResult;

    QPushButton* connectButton;
    QPushButton* setLocationButton;
    QPushButton* syncToStar;
    QPushButton* solveAlignment;
    QPushButton* resetAlignment;
    QPushButton* gotoTarget;
    QPushButton* gotoHome;
    QPushButton* slewFasterButton;
    QPushButton* slewSlowerButton;
    QPushButton* slewUpButton;
    QPushButton* slewLeftButton;
    QPushButton* slewRightButton;
    QPushButton* slewDownButton;
    QPushButton* stopButton;

    QLineEdit* portPathLine;
    QLineEdit* latitudeLine;
    QLineEdit* longitudeLine;
    QLineEdit* raHour;
    QLineEdit* raMin;
    QLineEdit* raSec;
    QLineEdit* deDeg;
    QLineEdit* deMin;
    QLineEdit* deSec;

    QComboBox* starSelectCombo;
};

#endif