#pragma once

#include <string>
#include "Skywatcher.h"
#include "AlignmentModel.h"

class WaveMount
{
public:
    explicit WaveMount(const std::string &portPath,
                       int baud = 115200);

    ~WaveMount();

    // Open serial, handshake, init encoders
    bool connect();
    void disconnect();

    bool isConnected() const { return connected; }

    double currentAzDeg = 0.0, currentAltDeg = 0.0;

    // Raw GOTO in encoder steps (you convert RA/DEC or Alt/Az yourself)
    bool gotoSteps(uint32_t raSteps, uint32_t deSteps);

    // GOTO in RA/DEC (hours/degrees) with basic spherical astronomy conversion
    bool gotoRaDec(double raHours, double decDegrees,
               double latitudeDeg, double longitudeDeg);

    // Continuous rates in deg/sec
    bool slewAtRatesDeg(double raRateDeg, double deRateDeg);

    // Continuous rates in "sidereal multiples" (e.g. 1.0 = sidereal, 800.0 = 800x)
    bool slewAtRates(double raRate, double deRate);

    bool isSlewing();
    bool isInSafeAltitude();

    // Stop both axes immediately (emergency stop)
    void stopAll();

    // Read encoders

    double minRate() const { return mount.get_min_rate(); }
    double maxRate() const { return mount.get_max_rate(); }

    void gotoAltAzDeg(double altDeg, double azDeg);

    AlignmentModel alignmentModel;
    
    AltAzCoords getCurrentTrueAltAz();
    AltAzCoords getCurrentRawAltAz();
    void syncToAltAz(double trueAltDeg, double trueAzDeg);
    bool solveAlignmentModel();

private:
    uint32_t raEncoder();
    uint32_t deEncoder();
    double EncoderToDegrees(uint32_t step, uint32_t initstep, uint32_t totalstep);
    void getEncodersDeg(double &azDeg, double &altDeg);
    long altDegToSteps(double altDeg);
    long azDegToSteps(double azDeg);

private:
    bool openSerial();
    void closeSerial();

public:
    std::string portPath;
    int         baud;
    int         fd        = -1;
    bool        connected = false;

    Skywatcher  mount;

    uint32_t currentAzEncoder, zeroAzEncoder, totalAzEncoder;
    uint32_t currentAltEncoder, zeroAltEncoder, totalAltEncoder;
};

