#include "WaveMount.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

WaveMount::WaveMount(const std::string &portPath, int baud)
    : portPath(portPath), baud(baud)
{
}

WaveMount::~WaveMount()
{
    disconnect();
}

bool WaveMount::openSerial()
{
    fd = open(portPath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("open");
        return false;
    }

    termios tty {};
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetattr");
        close(fd);
        fd = -1;
        return false;
    }

    cfmakeraw(&tty);

    speed_t spd = B115200;
    if (baud == 9600)   spd = B9600;
    if (baud == 19200)  spd = B19200;
    if (baud == 38400)  spd = B38400;
    if (baud == 57600)  spd = B57600;
    if (baud == 115200) spd = B115200;

    cfsetspeed(&tty, spd);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("tcsetattr");
        close(fd);
        fd = -1;
        return false;
    }

    return true;
}

void WaveMount::closeSerial()
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

bool WaveMount::connect()
{
    if (connected)
        return true;

    if (!openSerial())
        return false;

    mount.setPortFD(fd);

    try
    {
        if (!mount.Handshake())
        {
            std::cerr << "Handshake failed.\n";
            closeSerial();
            return false;
        }

        mount.Init();
        zeroAzEncoder  = mount.GetRAEncoderZero();
        totalAzEncoder = mount.GetRAEncoderTotal();
        zeroAltEncoder  = mount.GetDEEncoderZero();
        totalAltEncoder = mount.GetDEEncoderTotal();
    }
    catch (const std::exception &e)
    {
        std::cerr << "WaveMount::connect exception: " << e.what() << "\n";
        closeSerial();
        return false;
    }

    connected = true;
    return true;
}

void WaveMount::disconnect()
{
    if (!connected)
        return;

    try
    {
        stopAll();
    }
    catch (...) { }

    closeSerial();
    connected = false;
}

void WaveMount::gotoAltAzDeg(double altDeg, double azDeg)
{

    if (altDeg < 0.0 || altDeg > 90.0) {
        std::cerr << "Altitude degrees must be in the range [0, 90]" << std::endl;
        return;
    }

    if (azDeg < 0.0 || azDeg > 360.0) {
        std::cerr << "Azimuth degrees must be in the range [0, 360]" << std::endl;
        return;
    }

    if (alignmentModel.solved) {
        AltAzCoords trueCoords{altDeg, azDeg};
        AltAzCoords mountCoords = alignmentModel.trueSkyToMount(trueCoords);
        altDeg = mountCoords.alt;
        azDeg = mountCoords.az;

        std::cout << "gotoAltAzDeg:: Corrected Mount Alt/Az: Altitude = " << altDeg << ", Azimuth = " << azDeg << std::endl;
    }

    long altSteps = altDegToSteps(altDeg);
    long azSteps  = azDegToSteps(azDeg);
    std::cout << "gotoAltAzDeg:: Going to Alt/Az: Altitude = " << altSteps << "°, Azimuth = " << azSteps << "°" << std::endl;
    gotoSteps(azSteps, altSteps);
}

long WaveMount::altDegToSteps(double altDeg)
{
    if (altDeg < 0.0 || altDeg > 90.0) {
        std::cerr << "Altitude degrees must be in the range [0, 90]" << std::endl;
    }

    long steps = static_cast<long>((altDeg / 360) * totalAltEncoder);
    steps = (steps + zeroAltEncoder) % totalAltEncoder;

    return steps;
}

long WaveMount::azDegToSteps(double azDeg)
{
    long steps = static_cast<long>((azDeg / 360) * totalAzEncoder);
    steps = (steps + zeroAzEncoder) % totalAzEncoder;

    return steps;
}

bool WaveMount::gotoSteps(uint32_t raSteps, uint32_t deSteps)
{
    if (!connected)
        return false;

    try
    {
        mount.AbsSlewTo(raSteps, deSteps, true, true);
    }
    catch (const std::exception &e)
    {
        std::cerr << "gotoSteps exception: " << e.what() << "\n";
        return false;
    }

    return true;
}

bool WaveMount::slewAtRatesDeg(double raRateDeg, double deRateDeg)
{
    double raRateSidereal = raRateDeg / Skywatcher::SIDEREAL_DEG_PER_SEC;
    double deRateSidereal = deRateDeg / Skywatcher::SIDEREAL_DEG_PER_SEC;

    return slewAtRates(raRateSidereal, deRateSidereal);
}

bool WaveMount::slewAtRates(double raRate, double deRate)
{
    if (!connected)
        return false;

    try
    {
        // 0.0 means "stop" for that axis
        if (raRate == 0.0)
            mount.StopRA();
        else if (!mount.IsRARunning()) {
            mount.SlewRA(raRate);
        } else {
            std::cerr << "RA is already running, can't use SlewRA.\n";
        }

        if (deRate == 0.0)
            mount.StopDE();
        else if (!mount.IsDERunning()) {
            mount.SlewDE(deRate);
        } else {
            std::cerr << "DE is already running, can't use SlewDE.\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "slewAtRates exception: " << e.what() << "\n";
        std::cout << "Slew Rates failed" << std::endl; 
        return false;
    }

    return true;
}

bool WaveMount::isSlewing()
{
    if (!connected)
        return false;

    return mount.IsRARunning() || mount.IsDERunning();
}

void WaveMount::stopAll()
{
    if (!connected)
        return;

    try
    {
        mount.StopRA();
    }
    catch (...) { }

    try
    {
        mount.StopDE();
    }
    catch (...) { }
}

uint32_t WaveMount::raEncoder()
{
    if (!connected)
        throw std::runtime_error("raEncoder: not connected");
    return mount.GetRAEncoder();
}

uint32_t WaveMount::deEncoder()
{
    if (!connected)
        throw std::runtime_error("deEncoder: not connected");
    return mount.GetDEEncoder();
}

//bool WaveMount::isInSafeAltitude() {
//    return currentAltDeg >= 0.0 && currentAltDeg <= 90.0;
//}

AltAzCoords WaveMount::getCurrentTrueAltAz() {
    double azDeg, altDeg;
    getEncodersDeg(azDeg, altDeg);

    std::cout << "Current Mount Alt/Az: Altitude NOT CORRECTED = " << altDeg << ", Azimuth = " << azDeg << std::endl;

    if (alignmentModel.solved) {
        AltAzCoords mountCoords{altDeg, azDeg};
        AltAzCoords trueCoords = alignmentModel.mountToTrueSky(mountCoords);
        currentAzDeg = trueCoords.az;
        currentAltDeg = trueCoords.alt;
        return trueCoords;
    } else {
        return getCurrentRawAltAz();
    }
}

AltAzCoords WaveMount::getCurrentRawAltAz() {
    double azDeg, altDeg;
    getEncodersDeg(azDeg, altDeg);

    currentAzDeg = azDeg;
    currentAltDeg = altDeg;

    return AltAzCoords{altDeg, azDeg};
}

void WaveMount::getEncodersDeg(double &azDeg, double &altDeg)
{

    uint32_t azEncoder = mount.GetRAEncoder();
    uint32_t altEncoder = mount.GetDEEncoder();

    azDeg = EncoderToDegrees(azEncoder, zeroAzEncoder, totalAzEncoder);
    altDeg = EncoderToDegrees(altEncoder, zeroAltEncoder, totalAltEncoder);

    if (altDeg < 0) {
        std::cerr << "Warning: Altitude encoder value is negative (" << altDeg << "°). Check mount initialization and encoder readings.\n";
    } else if (altDeg > 90) {
        std::cerr << "Warning: Altitude encoder value exceeds 90° (" << altDeg << "°). Check mount initialization and encoder readings.\n";
    }
}

double WaveMount::EncoderToDegrees(uint32_t step, uint32_t initstep, uint32_t totalstep)
{
    // Normalize encoder difference into [0, totalstep)
    int64_t diff = static_cast<int64_t>(step) - static_cast<int64_t>(initstep);
    diff = (diff % totalstep + totalstep) % totalstep;

    double angle = (static_cast<double>(diff) / totalstep) * 360.0;

    return angle;   // Always 0–360°, no hemisphere logic
}

void WaveMount::syncToAltAz(double trueAltDeg, double trueAzDeg) {
    getCurrentRawAltAz();
    alignmentModel.addSample(AltAzCoords{trueAltDeg, trueAzDeg}, AltAzCoords{currentAltDeg, currentAzDeg});
}

bool WaveMount::solveAlignmentModel() {
    return alignmentModel.solve();
}