/* Copyright 2012 Geehalel
 *
 * This file is a reduced version of the Skywatcher Protocol INDI driver,
 * adapted for direct mount control without INDI/EQMod.
 *
 * License: GNU GPL v3 or later (same as original).
 */

#pragma once

#include <cstdint>
#include <sys/time.h>
#include <time.h>

class Skywatcher
{
public:
    Skywatcher();
    ~Skywatcher();

    enum SkywatcherAxis
    {
        Axis1 = 0, // RA/AZ
        Axis2 = 1, // DE/ALT
        NUMBER_OF_SKYWATCHERAXIS
    };

    // You must set an already-opened serial FD (8N1, 9600 or 115200 as per mount).
    void setPortFD(int value);

    // Low-level protocol init
    bool Handshake();   // reads MCVersion / MountCode, basic sanity check
    void Init();        // reads encoder info, sets home/period defaults

    // Encoders
    uint32_t GetRAEncoder();
    uint32_t GetDEEncoder();

    uint32_t GetRAEncoderZero();
    uint32_t GetRAEncoderTotal();
    uint32_t GetDEEncoderZero();
    uint32_t GetDEEncoderTotal();

    bool wasinitialized;

    uint32_t RAStepInit; // Initial RA position in step
    uint32_t DEStepInit; // Initial DE position in step

    // Continuous slewing in units of "sidereal rate multiples"
    // rate > 0 => forward, rate < 0 => backward
    void SlewRA(double rate);
    void SlewDE(double rate);

    // Change rate of an already-running slew (same direction & speedmode)
    void SetRARate(double rate);
    void SetDERate(double rate);

    // Stop axes
    void StopRA();
    void StopDE();

    // Absolute GOTO in encoder steps (0 to 0xFFFFFF)
    void AbsSlewTo(uint32_t raencoder, uint32_t deencoder, bool raup, bool deup);

    void SetAbsTarget(SkywatcherAxis axis, uint32_t target);
    void SetAbsTargetBreaks(SkywatcherAxis axis, uint32_t breakstep);

    // Status
    bool IsRARunning();
    bool IsDERunning();

    // Limits in sidereal multiples
    double get_min_rate() const { return MIN_RATE; }
    double get_max_rate() const { return MAX_RATE; }

    static constexpr double SIDEREAL_DEG_PER_SEC = 15.041067 / 3600.0;

public:
    bool getStepsPer360(long &raSteps, long &decSteps) const {
        if (RASteps360 == 0 || DESteps360 == 0)
            return false;
        raSteps  = RASteps360;
        decSteps = DESteps360;
        return true;
    }

private:
    // Protocol constants
    static const char SkywatcherLeadingChar = ':';
    static const char SkywatcherTrailingChar = 0x0d;
    static constexpr double MIN_RATE = 0.05;
    static constexpr double MAX_RATE = 800.0;

    static constexpr double SKYWATCHER_STELLAR_DAY   = 86164.098903691;
    static constexpr double SKYWATCHER_STELLAR_SPEED = 15.041067179;
    static constexpr double SKYWATCHER_LOWSPEED_RATE = 128.0;

    uint32_t Target[NUMBER_OF_SKYWATCHERAXIS];
    uint32_t TargetBreaks[NUMBER_OF_SKYWATCHERAXIS];

    static inline uint8_t HEX(char c)
    {
        return (c < 'A') ? (c - '0') : (c - 'A' + 10);
    }

    enum SkywatcherCommand
        {
            Initialize                = 'F',
            InquireMotorBoardVersion  = 'e',
            InquireGridPerRevolution  = 'a',
            InquireTimerInterruptFreq = 'b',
            InquireHighSpeedRatio     = 'g',
            InquirePECPeriod          = 's',
            InstantAxisStop           = 'L',
            NotInstantAxisStop        = 'K',
            SetAxisPositionCmd        = 'E',
            GetAxisPosition           = 'j',
            GetAxisStatus             = 'f',
            SetSnapPort               = 'O', // EQ8/AZEQ6/AZEQ5/EQ6-R only
            SetMotionMode             = 'G',
            SetGotoTargetIncrement    = 'H',
            SetBreakPointIncrement    = 'M',
            SetGotoTarget             = 'S',
            SetBreakStep              = 'U',
            SetStepPeriod             = 'I',
            StartMotion               = 'J',
            GetStepPeriod             = 'D', // See Merlin protocol http://www.papywizard.org/wiki/DevelopGuide
            ActivateMotor             = 'B', // See eq6direct implementation http://pierre.nerzic.free.fr/INDI/
            SetST4GuideRateCmd        = 'P',
            GetHomePosition           = 'd', // Get Home position encoder count (default at startup)
            SetFeatureCmd             = 'W', // EQ8/AZEQ6/AZEQ5 only
            GetFeatureCmd             = 'q', // EQ8/AZEQ6/AZEQ5 only
            InquireAuxEncoder         = 'd', // EQ8/AZEQ6/AZEQ5 only
            SetPolarScopeLED          = 'V',
    };

    char AxisCmd[2] {'1', '2'};

    enum SkywatcherDirection
    {
        BACKWARD = 0,
        FORWARD  = 1
    };

    enum SkywatcherSlewMode
    {
        SLEW = 0,
        GOTO = 1
    };

    enum SkywatcherSpeedMode
    {
        LOWSPEED  = 0,
        HIGHSPEED = 1
    };

    struct SkywatcherAxisStatus
    {
        SkywatcherDirection direction;
        SkywatcherSlewMode  slewmode;
        SkywatcherSpeedMode speedmode;
    };

    // Internal helpers
    void InquireEncoderInfo(SkywatcherAxis axis);
    void ReadMotorStatus(SkywatcherAxis axis);
    void CheckMotorStatus(SkywatcherAxis axis);

    void SetMotion(SkywatcherAxis axis, SkywatcherAxisStatus newstatus);
    void SetSpeed(SkywatcherAxis axis, uint32_t period);
    void StartMotor(SkywatcherAxis axis);
    void StopMotor(SkywatcherAxis axis);
    void StopWaitMotor(SkywatcherAxis axis);
    void InstantStopMotor(SkywatcherAxis axis);

    bool dispatch_command(SkywatcherCommand cmd, SkywatcherAxis axis, const char *arg);
    bool read_eqmod();

    uint32_t Revu24str2long(char *s);
    uint32_t Highstr2long(char *s);
    void     long2Revu24str(uint32_t n, char *str);

public:
    // Basic mount info
    uint32_t MCVersion   = 0;
    uint32_t MountCode   = 0;

    // Encoder / gearing
    uint32_t RASteps360  = 0;
    uint32_t DESteps360  = 0;
    uint32_t RAStepsWorm = 0;
    uint32_t DEStepsWorm = 0;
    uint32_t RAHighspeedRatio = 1;
    uint32_t DEHighspeedRatio = 1;

    // Current encoder positions
    uint32_t RAStep = 0;
    uint32_t DEStep = 0;

    // Current worm periods
    uint32_t RAPeriod = 256;
    uint32_t DEPeriod = 256;

    // Min periods for highspeed
    uint32_t minperiods[2] {6, 6};

    // Status
    bool RAInitialized = false;
    bool DEInitialized = false;
    bool RARunning     = false;
    bool DERunning     = false;

    SkywatcherAxisStatus RAStatus {};
    SkywatcherAxisStatus DEStatus {};

    // Timing
    struct timeval lastreadmotorstatus[NUMBER_OF_SKYWATCHERAXIS] {};
    struct timeval lastreadmotorposition[NUMBER_OF_SKYWATCHERAXIS] {};
    SkywatcherAxisStatus LastRunningStatus[NUMBER_OF_SKYWATCHERAXIS];

    // Serial
    int  PortFD = -1;
    char command[16];
    char response[16];

    // IO / retry
    static const long   EQMOD_TIMEOUT_US = 200000; // 200 ms
    static const uint8_t EQMOD_MAX_RETRY = 10;

    SkywatcherAxisStatus NewStatus[NUMBER_OF_SKYWATCHERAXIS];

    #define SKYWATCHER_LOWSPEED_RATE 128
    #define SKYWATCHER_MAXREFRESH    0.5

    bool SIMULATION_MODE = false;

    //uint32_t parkRAEncoder, parkDEEncoder;
};

