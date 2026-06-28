#include "Skywatcher.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdexcept>
#include <cctype>
#include <iostream>

Skywatcher::Skywatcher() = default;

Skywatcher::~Skywatcher()
{
    // We do NOT close PortFD here; caller owns the FD.
}

void Skywatcher::setPortFD(int value)
{
    PortFD = value;
}

bool Skywatcher::Handshake()
{
    if (PortFD < 0)
        throw std::runtime_error("Skywatcher::Handshake: PortFD not set");

    // Query motor board version
    dispatch_command(InquireMotorBoardVersion, Axis1, nullptr);
    uint32_t tmpMCVersion = Revu24str2long(response + 1);

    MCVersion = ((tmpMCVersion & 0xFF) << 16) |
                ((tmpMCVersion & 0xFF00)) |
                ((tmpMCVersion & 0xFF0000) >> 16);

    MountCode = MCVersion & 0xFF;

    // Reject known unsupported codes (same logic as original)
    if (MountCode == 0x80 || MountCode == 0x81 || MountCode == 0x90)
    {
        throw std::runtime_error("Skywatcher::Handshake: mount not supported by this driver");
    }

    // Default minperiods; some mounts override later if needed
    minperiods[Axis1] = 6;
    minperiods[Axis2] = 6;

    return true;
}

/*void Skywatcher2::Init()
{
    wasinitialized = false;
    ReadMotorStatus(Axis1);
    ReadMotorStatus(Axis2);

    if (!RAInitialized && !DEInitialized)
    {
        //Read initial stepper values
        dispatch_command(GetAxisPosition, Axis1, nullptr);
        //read_eqmod();
        RAStepInit = Revu24str2long(response + 1);
        dispatch_command(GetAxisPosition, Axis2, nullptr);
        //read_eqmod();
        DEStepInit = Revu24str2long(response + 1);

        // Energize motors
        dispatch_command(InitializeCmd, Axis1, nullptr);
        //read_eqmod();
        dispatch_command(InitializeCmd, Axis2, nullptr);
        //read_eqmod();
        RAStepHome = RAStepInit;
        DEStepHome = DEStepInit + (DESteps360 / 4);
        std::cout << "Mount has NOT already been initialized" << std::endl;
    }
    else
    {
        std::cout << "Mount already initialized by another driver / driver instance" << std::endl;

        // Mount already initialized by another driver / driver instance
        // use default configuration && leave unchanged encoder values
        wasinitialized = true;
        RAStepInit     = 0x800000;
        DEStepInit     = 0x800000;
        RAStepHome     = RAStepInit;
        DEStepHome     = DEStepInit + (DESteps360 / 4);
    }

    InquireEncoderInfo(Axis1);
    InquireEncoderInfo(Axis2);
    GetRAEncoder();
    GetDEEncoder();
}*/

void Skywatcher::Init()
{
    wasinitialized = false;
    ReadMotorStatus(Axis1);
    ReadMotorStatus(Axis2);

    InquireEncoderInfo(Axis1);
    InquireEncoderInfo(Axis2);

    if (!RAInitialized && !DEInitialized)
    {
        RAStepInit     = 0;
        DEStepInit     = 0;
        std::cout << RAStepInit << std::endl;

        dispatch_command(Initialize, Axis1, nullptr);

        dispatch_command(Initialize, Axis2, nullptr);

        char cmdarg[7];
        cmdarg[6] = '\0';
        long2Revu24str(RAStepInit, cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis1, cmdarg);

        cmdarg[6] = '\0';
        long2Revu24str(DEStepInit, cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis2, cmdarg);
    } else {
        dispatch_command(GetAxisPosition, Axis1, nullptr);
        RAStepInit = Revu24str2long(response + 1);

        dispatch_command(GetAxisPosition, Axis2, nullptr);
        DEStepInit = Revu24str2long(response + 1);
    }

    wasinitialized = true;

    //Park status
    /*if (telescope->InitPark() == false)
    {
        telescope->SetAxis1Park(RAStepHome);
        telescope->SetAxis1ParkDefault(RAStepHome);

        telescope->SetAxis2Park(DEStepHome);
        telescope->SetAxis2ParkDefault(DEStepHome);

        LOGF_WARN("Loading parking data failed. Setting parking axis1: %d axis2: %d", RAStepHome, DEStepHome);

        // JM 2018-11-26: Save current position as parked position
        telescope->saveInitialParkPosition();
    }
    else
    {
        telescope->SetAxis1ParkDefault(RAStepHome);
        telescope->SetAxis2ParkDefault(DEStepHome);
    }

    if (telescope->isParked())
    {
        //TODO get Park position, set corresponding encoder values, mark mount as parked
        //parkSP->sp[0].s==ISS_ON
        LOGF_DEBUG("%s() : Mount was parked", __FUNCTION__);
        //if (wasinitialized) {
        //  LOGF_DEBUG("%s() : leaving encoders unchanged",
        //	     __FUNCTION__);
        //} else {
        char cmdarg[7];
        LOGF_DEBUG("%s() : Mount in Park position -- setting encoders RA=%ld DE = %ld",
                   __FUNCTION__, static_cast<long>(telescope->GetAxis1Park()), static_cast<long>(telescope->GetAxis2Park()));
        cmdarg[6] = '\0';
        long2Revu24str(telescope->GetAxis1Park(), cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis1, cmdarg);
        //read_eqmod();
        cmdarg[6] = '\0';
        long2Revu24str(telescope->GetAxis2Park(), cmdarg);
        dispatch_command(SetAxisPositionCmd, Axis2, cmdarg);
        //read_eqmod();
        //}
    }
    else
    {
        LOGF_DEBUG("%s() : Mount was not parked", __FUNCTION__);
        if (wasinitialized)
        {
            LOGF_DEBUG("%s() : leaving encoders unchanged", __FUNCTION__);
        }
        else
        {
            //mount is supposed to be in the home position (pointing Celestial Pole)
            char cmdarg[7];
            LOGF_DEBUG("%s() : Mount in Home position -- setting encoders RA=%ld DE = %ld",
                       __FUNCTION__, static_cast<long>(RAStepHome), static_cast<long>(DEStepHome));
            cmdarg[6] = '\0';
            long2Revu24str(DEStepHome, cmdarg);
            dispatch_command(SetAxisPositionCmd, Axis2, cmdarg);
            //read_eqmod();
            //LOGF_WARN("%s() : Mount is supposed to point North/South Celestial Pole", __FUNCTION__);
            //TODO mark mount as unparked?
        }
    }*/
}

/*void Skywatcher::Init()
{
    // Initialize both axes (this is in the original driver)
    dispatch_command(InitializeCmd, Axis1, nullptr);
    dispatch_command(InitializeCmd, Axis2, nullptr);

    // Read encoder / gearing info for both axes
    InquireEncoderInfo(Axis1);
    InquireEncoderInfo(Axis2);

    // Read initial encoder positions
    GetRAEncoder();
    GetDEEncoder();
}*/

uint32_t Skywatcher::GetRAEncoder()
{
    // Axis Position
    dispatch_command(GetAxisPosition, Axis1, nullptr);

    uint32_t steps = Revu24str2long(response + 1);
    if (steps & 0x80000000)
        std::cout << "GetRAEncoder : Ignoring invalid response " << response << std::endl;
    else
        RAStep = steps;

    gettimeofday(&lastreadmotorposition[Axis1], nullptr);

    return RAStep;
}

uint32_t Skywatcher::GetDEEncoder()
{
    // Axis Position
    dispatch_command(GetAxisPosition, Axis2, nullptr);

    uint32_t steps = Revu24str2long(response + 1);
    if (steps & 0x80000000)
        std::cout << "GetDEEncoder : Ignoring invalid response " << response << std::endl;
    else
        DEStep = steps;
    gettimeofday(&lastreadmotorposition[Axis2], nullptr);
    
    return DEStep;
}

/*void Skywatcher2::ReadMotorStatus(SkywatcherAxis axis)
{
    dispatch_command(GetAxisStatus, axis, nullptr);

    switch (axis)
    {
    case Axis1:
        RAInitialized = (response[3] & 0x01);
        RARunning     = (response[2] & 0x01);
        RAStatus.slewmode  = (response[1] & 0x01) ? SLEW : GOTO;
        RAStatus.direction = (response[1] & 0x02) ? BACKWARD : FORWARD;
        RAStatus.speedmode = (response[1] & 0x04) ? HIGHSPEED : LOWSPEED;
        break;

    case Axis2:
        DEInitialized = (response[3] & 0x01);
        DERunning     = (response[2] & 0x01);
        DEStatus.slewmode  = (response[1] & 0x01) ? SLEW : GOTO;
        DEStatus.direction = (response[1] & 0x02) ? BACKWARD : FORWARD;
        DEStatus.speedmode = (response[1] & 0x04) ? HIGHSPEED : LOWSPEED;
        break;

    default:
        break;
    }

    gettimeofday(&lastreadmotorstatus[axis], nullptr);
}*/

void Skywatcher::ReadMotorStatus(SkywatcherAxis axis)
{
    dispatch_command(GetAxisStatus, axis, nullptr);
    //read_eqmod();
    switch (axis)
    {
        case Axis1:
            RAInitialized = (response[3] & 0x01);
            RARunning     = (response[2] & 0x01);
            if (response[1] & 0x01)
                RAStatus.slewmode = SLEW;
            else
                RAStatus.slewmode = GOTO;
            if (response[1] & 0x02)
                RAStatus.direction = BACKWARD;
            else
                RAStatus.direction = FORWARD;
            if (response[1] & 0x04)
                RAStatus.speedmode = HIGHSPEED;
            else
                RAStatus.speedmode = LOWSPEED;
            break;
        case Axis2:
            DEInitialized = (response[3] & 0x01);
            DERunning     = (response[2] & 0x01);
            if (response[1] & 0x01)
                DEStatus.slewmode = SLEW;
            else
                DEStatus.slewmode = GOTO;
            if (response[1] & 0x02)
                DEStatus.direction = BACKWARD;
            else
                DEStatus.direction = FORWARD;
            if (response[1] & 0x04)
                DEStatus.speedmode = HIGHSPEED;
            else
                DEStatus.speedmode = LOWSPEED;
            break;
        default:
            break;
    }
    gettimeofday(&lastreadmotorstatus[axis], nullptr);
}

/*void Skywatcher2::CheckMotorStatus(SkywatcherAxis axis)
{
    struct timeval now;
    gettimeofday(&now, nullptr);

    double dt = (now.tv_sec - lastreadmotorstatus[axis].tv_sec) +
                (now.tv_usec - lastreadmotorstatus[axis].tv_usec) / 1e6;

    if (dt > 0.5) // SKYWATCHER_MAXREFRESH
        ReadMotorStatus(axis);
}*/

void Skywatcher::CheckMotorStatus(SkywatcherAxis axis)
{
    struct timeval now;

    gettimeofday(&now, nullptr);
    if (((now.tv_sec - lastreadmotorstatus[axis].tv_sec) + ((now.tv_usec - lastreadmotorstatus[axis].tv_usec) / 1e6)) >
            SKYWATCHER_MAXREFRESH)
        ReadMotorStatus(axis);
}

void Skywatcher::InquireEncoderInfo(SkywatcherAxis axis)
{
    uint32_t *Steps360       = nullptr;
    uint32_t *StepsWorm      = nullptr;
    uint32_t *HighspeedRatio = nullptr;

    if (axis == Axis1)
    {
        Steps360       = &RASteps360;
        StepsWorm      = &RAStepsWorm;
        HighspeedRatio = &RAHighspeedRatio;
    }
    else
    {
        Steps360       = &DESteps360;
        StepsWorm      = &DEStepsWorm;
        HighspeedRatio = &DEHighspeedRatio;
    }

    // Steps per 360 degrees
    dispatch_command(InquireGridPerRevolution, axis, nullptr);
    *Steps360 = Revu24str2long(response + 1);

    // Steps per worm
    dispatch_command(InquireTimerInterruptFreq, axis, nullptr);
    *StepsWorm = Revu24str2long(response + 1);

    // Highspeed ratio
    dispatch_command(InquireHighSpeedRatio, axis, nullptr);
    *HighspeedRatio = Highstr2long(response + 1);
}

void Skywatcher::SlewRA(double rate)
{
    double absrate       = fabs(rate);
    uint32_t period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    if (RARunning && (RAStatus.slewmode == GOTO))
    {
        std::cerr << "Can not slew while goto is in progress" << std::endl;
    }

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        std::cerr << "Speed rate out of limits: " << absrate << " (min=" << MIN_RATE << ", max=" << MAX_RATE << ")" << std::endl;
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / RAHighspeedRatio;
        useHighspeed = true;
        std::cout << "Skywatcher::SlewRa: Using highspeed mode, ratio = " << absrate << std::endl;
    }
    //}
    period = static_cast<uint32_t>(((SKYWATCHER_STELLAR_DAY * RAStepsWorm) / static_cast<double>(RASteps360)) / absrate);
    
    if (rate >= 0.0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    SetMotion(Axis1, newstatus);
    SetSpeed(Axis1, period);
    if (!RARunning) {
        StartMotor(Axis1);
    }
}

void Skywatcher::SlewDE(double rate)
{
    double absrate       = fabs(rate);
    uint32_t period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    if (DERunning && (DEStatus.slewmode == GOTO))
    {
        std::cerr << "Can not slew while goto is in progress" << std::endl;
    }

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        std::cerr << "Speed rate out of limits: " << absrate << " (min=" << MIN_RATE << ", max=" << MAX_RATE << ")" << std::endl;
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / DEHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period = (long)(((SKYWATCHER_STELLAR_DAY * (double)DEStepsWorm) / (double)DESteps360) / absrate);

    if (rate >= 0.0)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    SetMotion(Axis2, newstatus);
    SetSpeed(Axis2, period);
    if (!DERunning)
        StartMotor(Axis2);
}

void Skywatcher::SetRARate(double rate)
{
    double absrate       = fabs(rate);
    uint32_t period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        std::cerr << "Speed rate out of limits: " << absrate << " (min=" << MIN_RATE << ", max=" << MAX_RATE << ")" << std::endl;
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / RAHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period              = static_cast<uint32_t>(((SKYWATCHER_STELLAR_DAY * RAStepsWorm) / static_cast<double>
                          (RASteps360)) / absrate);
    newstatus.direction = ((rate >= 0.0) ? FORWARD : BACKWARD);
    //newstatus.slewmode=RAStatus.slewmode;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    ReadMotorStatus(Axis1);
    if (RARunning)
    {
        if (newstatus.speedmode != RAStatus.speedmode)
            std::cerr << "Can not change rate while motor is running (speedmode differs)." << std::endl;
        if (newstatus.direction != RAStatus.direction)
            std::cerr << "Can not change rate while motor is running (direction differs)." << std::endl;
    }
    SetMotion(Axis1, newstatus);
    SetSpeed(Axis1, period);
}

void Skywatcher::SetDERate(double rate)
{
    double absrate       = fabs(rate);
    uint32_t period = 0;
    bool useHighspeed    = false;
    SkywatcherAxisStatus newstatus;

    if ((absrate < get_min_rate()) || (absrate > get_max_rate()))
    {
        std::cerr << "Speed rate out of limits: " << absrate << " (min=" << MIN_RATE << ", max=" << MAX_RATE << ")" << std::endl;
    }
    //if (MountCode != 0xF0) {
    if (absrate > SKYWATCHER_LOWSPEED_RATE)
    {
        absrate      = absrate / DEHighspeedRatio;
        useHighspeed = true;
    }
    //}
    period              = static_cast<uint32_t>(((SKYWATCHER_STELLAR_DAY * DEStepsWorm) / static_cast<double>
                          (DESteps360)) / absrate);
    newstatus.direction = ((rate >= 0.0) ? FORWARD : BACKWARD);
    //newstatus.slewmode=DEStatus.slewmode;
    newstatus.slewmode = SLEW;
    if (useHighspeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    ReadMotorStatus(Axis2);
    if (DERunning)
    {
        if (newstatus.speedmode != DEStatus.speedmode)
            std::cerr << "Can not change rate while motor is running (speedmode differs)." << std::endl;
        if (newstatus.direction != DEStatus.direction)
            std::cerr << "Can not change rate while motor is running (direction differs)." << std::endl;
    }
    SetMotion(Axis2, newstatus);
    SetSpeed(Axis2, period);
}

void Skywatcher::StopRA()
{
    StopWaitMotor(Axis1);
}

void Skywatcher::StopDE()
{
    StopWaitMotor(Axis2);
}

bool Skywatcher::IsRARunning()
{
    CheckMotorStatus(Axis1);
    return (RARunning);
}

bool Skywatcher::IsDERunning()
{
    CheckMotorStatus(Axis2);
    return (DERunning);
}

/*void Skywatcher2::SetMotion(SkywatcherAxis axis, const SkywatcherAxisStatus &newstatus)
{
    SkywatcherAxisStatus *currentstatus = (axis == Axis1) ? &RAStatus : &DEStatus;

    std::cout << currentstatus->direction << " " << currentstatus->slewmode << " " << currentstatus->speedmode << " -> "
              << newstatus.direction << " " << newstatus.slewmode << " " << newstatus.speedmode << "\n";

    char motioncmd[3];
    motioncmd[2] = '\0';

    switch (newstatus.slewmode)
    {
    case SLEW:
        motioncmd[0] = (newstatus.speedmode == LOWSPEED) ? '1' : '3';
        break;
    case GOTO:
        motioncmd[0] = (newstatus.speedmode == LOWSPEED) ? '2' : '0';
        break;
    default:
        motioncmd[0] = '1';
        break;
    }

    motioncmd[1] = (newstatus.direction == FORWARD) ? '0' : '1';

    // If any of direction/speedmode/slewmode changed, stop and reprogram
    //if (newstatus.direction != currentstatus->direction ||
    //    newstatus.speedmode != currentstatus->speedmode ||
    //    newstatus.slewmode  != currentstatus->slewmode)
    //{
        //StopWaitMotor(axis);
        dispatch_command(SetMotionMode, axis, motioncmd);
    //}

    *currentstatus = newstatus;
}*/

void Skywatcher::SetMotion(SkywatcherAxis axis, SkywatcherAxisStatus newstatus)
{
    char motioncmd[3];
    SkywatcherAxisStatus *currentstatus;
    
    CheckMotorStatus(axis);
    if (axis == Axis1)
        currentstatus = &RAStatus;
    else
        currentstatus = &DEStatus;
    motioncmd[2] = '\0';
    switch (newstatus.slewmode)
    {
        case SLEW:
            if (newstatus.speedmode == LOWSPEED)
                motioncmd[0] = '1';
            else
                motioncmd[0] = '3';
            break;
        case GOTO:
            if (newstatus.speedmode == LOWSPEED)
                motioncmd[0] = '2';
            else
                motioncmd[0] = '0';
            break;
        default:
            motioncmd[0] = '1';
            break;
    }
    if (newstatus.direction == FORWARD)
        motioncmd[1] = '0';
    else
        motioncmd[1] = '1';
    /*
    #ifdef STOP_WHEN_MOTION_CHANGED
    StopWaitMotor(axis);
    dispatch_command(SetMotionMode, axis, motioncmd);
    //read_eqmod();
    #else
    */
    if ((newstatus.direction != currentstatus->direction) || (newstatus.speedmode != currentstatus->speedmode) ||
            (newstatus.slewmode != currentstatus->slewmode))
    {
        StopWaitMotor(axis);
        dispatch_command(SetMotionMode, axis, motioncmd);
        //read_eqmod();
    }
    //#endif
    NewStatus[axis] = newstatus;
}

/*void Skywatcher2::SetSpeed(SkywatcherAxis axis, uint32_t period)
{
    SkywatcherAxisStatus *currentstatus = (axis == Axis1) ? &RAStatus : &DEStatus;

    if (currentstatus->speedmode == HIGHSPEED && period < minperiods[axis])
        period = minperiods[axis];

    if (axis == Axis1 && RARunning &&
        (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED))
        throw std::runtime_error("SetSpeed: cannot change speed while RA in GOTO or highspeed");

    if (axis == Axis2 && DERunning &&
        (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED))
        throw std::runtime_error("SetSpeed: cannot change speed while DE in GOTO or highspeed");

    char cmd[7];

    /////////////////////////////////////////////////////TEMP
    //if (period < 20)
    //period = 20;

    long2Revu24str(period, cmd);

    if (axis == Axis1)
        RAPeriod = period;
    else
        DEPeriod = period;

    dispatch_command(SetStepPeriod, axis, cmd);
}*/

void Skywatcher::SetSpeed(SkywatcherAxis axis, uint32_t period)
{
    char cmd[7];
    SkywatcherAxisStatus *currentstatus;

    ReadMotorStatus(axis);
    if (axis == Axis1)
        currentstatus = &RAStatus;
    else
        currentstatus = &DEStatus;
    if ((currentstatus->speedmode == HIGHSPEED) && (period < minperiods[axis]))
    {
        //LOGF_WARN("Setting axis %c period to minimum. Requested is %d, minimum is %d\n", AxisCmd[axis],
                  //period, minperiods[axis]);
        period = minperiods[axis];
    }
    long2Revu24str(period, cmd);

    if ((axis == Axis1) && (RARunning && (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED)))
        //throw EQModError(EQModError::ErrInvalidParameter,
                        // "Can not change speed while motor is running and in goto or highspeed slew.");
    if ((axis == Axis2) && (DERunning && (currentstatus->slewmode == GOTO || currentstatus->speedmode == HIGHSPEED)))
        //throw EQModError(EQModError::ErrInvalidParameter,
                        // "Can not change speed while motor is running and in goto or highspeed slew.");

    if (axis == Axis1) { RAPeriod = period; } else { DEPeriod = period; }
    
    dispatch_command(SetStepPeriod, axis, cmd);
    //read_eqmod();
}

void Skywatcher::StartMotor(SkywatcherAxis axis)
{
    dispatch_command(StartMotion, axis, nullptr);
}

void Skywatcher::StopMotor(SkywatcherAxis axis)
{
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;
        
    dispatch_command(NotInstantAxisStop, axis, nullptr);
    //read_eqmod();
}

void Skywatcher::InstantStopMotor(SkywatcherAxis axis)
{
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;

    dispatch_command(InstantAxisStop, axis, nullptr);
    //read_eqmod();
}

void Skywatcher::StopWaitMotor(SkywatcherAxis axis)
{
    bool *motorrunning;
    struct timespec wait;
    ReadMotorStatus(axis);
    if (axis == Axis1 && RARunning)
        LastRunningStatus[Axis1] = RAStatus;
    if (axis == Axis2 && DERunning)
        LastRunningStatus[Axis2] = DEStatus;

    dispatch_command(NotInstantAxisStop, axis, nullptr);
    //read_eqmod();
    if (axis == Axis1)
        motorrunning = &RARunning;
    else
        motorrunning = &DERunning;
    wait.tv_sec  = 0;
    wait.tv_nsec = 100000000; // 100ms
    ReadMotorStatus(axis);
    while (*motorrunning)
    {
        nanosleep(&wait, nullptr);
        ReadMotorStatus(axis);
    }
}

/*void Skywatcher2::AbsSlewTo(SkywatcherAxis axis, uint32_t target)
{
    // 1. Stop axis if running
    std::cout << "AbsSlewTo:: StopWaitMotor:" << std::endl;
    StopWaitMotor(axis);
    std::cout << "AbsSlewTo:: SetMotion:" << std::endl;

    // 2. Set GOTO mode
    SkywatcherAxisStatus st;
    st.direction = FORWARD;   // direction is ignored for GOTO
    st.slewmode  = GOTO;
    st.speedmode = LOWSPEED;  // GOTO always starts in lowspeed
    SetMotion(axis, st);

    // 3. Program target
    std::cout << "AbsSlewTo:: SetGotoTarget:" << std::endl;
    char buf[7];
    long2Revu24str(target, buf);
    dispatch_command(SetGotoTarget, axis, buf);

    // 4. Program break point (same as target)
    std::cout << "AbsSlewTo:: SetBreakStep:" << std::endl;
    dispatch_command(SetBreakStep, axis, buf);

    // 5. Start motion
    std::cout << "AbsSlewTo:: StartMotion:" << std::endl;
    dispatch_command(StartMotion, axis, nullptr);

    // 6. Wait until motor stops
    std::cout << "AbsSlewTo:: StopWaitMotor:" << std::endl;
    StopWaitMotor(axis);
}*/

void Skywatcher::SetAbsTargetBreaks(SkywatcherAxis axis, uint32_t breakstep)
{
    char cmd[7];
    
    long2Revu24str(breakstep, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetBreakStep, axis, cmd);
    //read_eqmod();
    TargetBreaks[axis] = breakstep;
}

void Skywatcher::SetAbsTarget(SkywatcherAxis axis, uint32_t target)
{
    char cmd[7];
    
    long2Revu24str(target, cmd);
    //IDLog("Setting target for axis %c  to %d\n", AxisCmd[axis], increment);
    dispatch_command(SetGotoTarget, axis, cmd);
    //read_eqmod();
    Target[axis] = target;
}

void Skywatcher::AbsSlewTo(uint32_t raencoder, uint32_t deencoder, bool raup, bool deup)
{
    SkywatcherAxisStatus newstatus;
    bool useHighSpeed = false;
    int32_t deltaraencoder, deltadeencoder;
    uint32_t lowperiod = 18, lowspeedmargin = 20000, breaks = 400;
    /* highperiod = RA 450X DE (+5) 200x, low period 32x */

    deltaraencoder = static_cast<int32_t>(raencoder - RAStep);
    deltadeencoder = static_cast<int32_t>(deencoder - DEStep);

    newstatus.slewmode = GOTO;
    if (raup)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltaraencoder < 0)
        deltaraencoder = -deltaraencoder;
    if (deltaraencoder > static_cast<int32_t>(lowspeedmargin))
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltaraencoder > 0)
    {
        SetMotion(Axis1, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis1, minperiods[Axis1]);
        else
            SetSpeed(Axis1, lowperiod);
        SetAbsTarget(Axis1, raencoder);
        if (useHighSpeed)
            breaks = ((deltaraencoder > 3200) ? 3200 : deltaraencoder / 10);
        else
            breaks = ((deltaraencoder > 200) ? 200 : deltaraencoder / 10);
        breaks = (raup ? (raencoder - breaks) : (raencoder + breaks));
        SetAbsTargetBreaks(Axis1, breaks);
        StartMotor(Axis1);
    }

    if (deup)
        newstatus.direction = FORWARD;
    else
        newstatus.direction = BACKWARD;
    if (deltadeencoder < 0)
        deltadeencoder = -deltadeencoder;
    if (deltadeencoder > static_cast<int32_t>(lowspeedmargin))
        useHighSpeed = true;
    else
        useHighSpeed = false;
    if (useHighSpeed)
        newstatus.speedmode = HIGHSPEED;
    else
        newstatus.speedmode = LOWSPEED;
    if (deltadeencoder > 0)
    {
        SetMotion(Axis2, newstatus);
        if (useHighSpeed)
            SetSpeed(Axis2, minperiods[Axis2]);
        else
            SetSpeed(Axis2, lowperiod);
        SetAbsTarget(Axis2, deencoder);
        if (useHighSpeed)
            breaks = ((deltadeencoder > 3200) ? 3200 : deltadeencoder / 10);
        else
            breaks = ((deltadeencoder > 200) ? 200 : deltadeencoder / 10);
        breaks = (deup ? (deencoder - breaks) : (deencoder + breaks));
        SetAbsTargetBreaks(Axis2, breaks);
        StartMotor(Axis2);
    }
}


bool Skywatcher::dispatch_command(SkywatcherCommand cmd, SkywatcherAxis axis, const char *arg)
{
    //std::cout << "Dispatching command: " << cmd << " Axis: " << axis << " Arg: " << (arg ? arg : "null") << "\n";

    if (Skywatcher::SIMULATION_MODE && PortFD < 0) {
        if (arg == nullptr)
            std::snprintf(command, sizeof(command), "%c%c%c%c",
                          SkywatcherLeadingChar, cmd, AxisCmd[axis], SkywatcherTrailingChar);
        else
            std::snprintf(command, sizeof(command), "%c%c%c%s%c",
                          SkywatcherLeadingChar, cmd, AxisCmd[axis], arg, SkywatcherTrailingChar);

        std::cout << "[" << command << "]\n";

        return true;
    }

    if (PortFD < 0)
        throw std::runtime_error("dispatch_command: PortFD not set");

    for (uint8_t i = 0; i < EQMOD_MAX_RETRY; ++i)
    {
        // Build command
        if (arg == nullptr)
            std::snprintf(command, sizeof(command), "%c%c%c%c",
                          SkywatcherLeadingChar, cmd, AxisCmd[axis], SkywatcherTrailingChar);
        else
            std::snprintf(command, sizeof(command), "%c%c%c%s%c",
                          SkywatcherLeadingChar, cmd, AxisCmd[axis], arg, SkywatcherTrailingChar);

        if (Skywatcher::SIMULATION_MODE) { std::cout << "[" << command << "]\n"; }

        size_t len = std::strlen(command);
        ssize_t written = ::write(PortFD, command, len);
        if (written < 0)
        {
            if (i == EQMOD_MAX_RETRY - 1)
                throw std::runtime_error("dispatch_command: write failed");
            usleep(100000); // 100 ms
            continue;
        }

        if (read_eqmod())
        {
            if (Skywatcher::SIMULATION_MODE) { std::cout << "Reading response: [" << response << "]\n"; }
            return true;
        }

        // read_eqmod failed but did not throw: retry
        usleep(100000);
    }

    return false;
}

bool Skywatcher::read_eqmod()
{
    // Read until CR or timeout
    int nbytes_read = 0;
    response[0] = '\0';

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(PortFD, &rfds);

    struct timeval tv;
    tv.tv_sec  = EQMOD_TIMEOUT_US / 1000000;
    tv.tv_usec = EQMOD_TIMEOUT_US % 1000000;

    int ret = select(PortFD + 1, &rfds, nullptr, nullptr, &tv);
    if (ret <= 0)
        throw std::runtime_error("read_eqmod: timeout or select error");

    while (nbytes_read < static_cast<int>(sizeof(response) - 1))
    {
        char c;
        ssize_t r = ::read(PortFD, &c, 1);
        if (r < 0)
            throw std::runtime_error("read_eqmod: read error");

        response[nbytes_read++] = c;
        if (c == SkywatcherTrailingChar)
            break;
    }

    if (nbytes_read <= 0)
        throw std::runtime_error("read_eqmod: no data");

    // Remove CR
    response[nbytes_read - 1] = '\0';

    switch (response[0])
    {
    case '=':
        // Validate hex payload
        for (const char *p = &response[1]; *p != '\0'; ++p)
        {
            if (!(std::isxdigit(static_cast<unsigned char>(*p)) && !std::islower(static_cast<unsigned char>(*p))))
                throw std::runtime_error("read_eqmod: invalid hex in response");
        }
        break;

    case '!':
        throw std::runtime_error("read_eqmod: command failed");

    default:
        throw std::runtime_error("read_eqmod: invalid response header");
    }

    return true;
}

uint32_t Skywatcher::Revu24str2long(char *s)
{
    uint32_t res = 0;
    res  = HEX(s[4]);
    res <<= 4;
    res |= HEX(s[5]);
    res <<= 4;
    res |= HEX(s[2]);
    res <<= 4;
    res |= HEX(s[3]);
    res <<= 4;
    res |= HEX(s[0]);
    res <<= 4;
    res |= HEX(s[1]);
    return res;
}

uint32_t Skywatcher::Highstr2long(char *s)
{
    uint32_t res = 0;
    res  = HEX(s[0]);
    res <<= 4;
    res |= HEX(s[1]);
    return res;
}

void Skywatcher::long2Revu24str(uint32_t n, char *str)
{
    static const char hexa[16] =
        {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    str[0] = hexa[(n & 0xF0) >> 4];
    str[1] = hexa[(n & 0x0F)];
    str[2] = hexa[(n & 0xF000) >> 12];
    str[3] = hexa[(n & 0x0F00) >> 8];
    str[4] = hexa[(n & 0xF00000) >> 20];
    str[5] = hexa[(n & 0x0F0000) >> 16];
    str[6] = '\0';
}

uint32_t Skywatcher::GetRAEncoderZero()
{
    return RAStepInit;
}

uint32_t Skywatcher::GetRAEncoderTotal()
{
    return RASteps360;
}


uint32_t Skywatcher::GetDEEncoderZero()
{
    return DEStepInit;
}

uint32_t Skywatcher::GetDEEncoderTotal()
{
    return DESteps360;
}