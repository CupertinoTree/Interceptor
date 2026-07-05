#include "CameraWorker.h"
#include <opencv2/opencv.hpp>

CameraWorker::CameraWorker(int cameraID) :
    cameraID(cameraID), running(false) {}

bool CameraWorker::openCamera() {

    if (SIMULATION_MODE) return true;

    ASI_ERROR_CODE err = ASIOpenCamera(cameraID);
    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("openCamera:: Failed to open camera. " + asiErrorToString(err));
        return false;
    }

    err = ASIInitCamera(cameraID);
    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("openCamera:: Failed to init camera. " + asiErrorToString(err));
        return false;
    }

    ASI_CAMERA_INFO info;
    err = ASIGetCameraProperty(&info, cameraID);
    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("openCamera:: Failed to get camera properties. " + asiErrorToString(err));
        return false;
    }
    isColorCamera = (info.IsColorCam == ASI_TRUE);

    err = ASISetROIFormat(cameraID, info.MaxWidth, info.MaxHeight, 1, ASI_IMG_RGB24);
    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("openCamera:: Failed to set ROI. " + asiErrorToString(err));
        return false;
    }

    return true;
}

bool CameraWorker::closeCamera() {
    ASI_ERROR_CODE err = ASICloseCamera(cameraID);
    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("closeCamera:: Failed to close camera. " + asiErrorToString(err));
        return false;
    }
    return true;
}

int CameraWorker::getNumOfCameras() {
    return ASIGetNumOfConnectedCameras();
}

QStringList CameraWorker::getCamerasList(int numCameras)
{
    QStringList list;

    for (int i = 0; i < numCameras; ++i) {
        ASI_CAMERA_INFO info;
        ASI_ERROR_CODE perr = ASIGetCameraProperty(&info, i);

        if (perr == ASI_SUCCESS) {
            list << QString("Camera %1: %2").arg(i).arg(info.Name);
        } else if (perr == ASI_ERROR_INVALID_INDEX) {
            // Skip invalid index (rare but possible)
            continue;
        } else {
            // Unknown error → skip but keep scanning
            continue;
        }
    }

    return list;
}


bool CameraWorker::startVideoCapture() {
    if (SIMULATION_MODE) return true;

    ASI_ERROR_CODE err = ASIStartVideoCapture(cameraID);

    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("startVideo:: Failed to start video capture. " + asiErrorToString(err));
        return false;
    }

    return true;
}

bool CameraWorker::stopVideoCapture() {
    ASI_ERROR_CODE err = ASIStopVideoCapture(cameraID);

    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("stopVideo:: Failed to stop video capture. " + asiErrorToString(err));
        return false;
    }

    return true;
}

void CameraWorker::startCaptureLoop() {
    running = true;
    captureLoop();
}

void CameraWorker::stopCaptureLoop() {
    running = false;
}

void CameraWorker::captureLoop() {

    if (SIMULATION_MODE) {
        simulationCaptureLoop();
        return;
    }

    int width, height, bin;
    ASI_IMG_TYPE imgType;
    ASIGetROIFormat(cameraID, &width, &height, &bin, &imgType);

    int bufferSize = width * height * 3;
    std::unique_ptr<unsigned char[]> pBuffer(new unsigned char[bufferSize]);

    while (running) {
        ASI_ERROR_CODE err = ASIGetVideoData(cameraID, pBuffer.get(), bufferSize, 100);
        if (err == ASI_SUCCESS) {
            cv::Mat frame;

            if (imgType == ASI_IMG_RAW8) {
                // RAW8 is 1-channel for both mono and color cameras.
                cv::Mat raw(height, width, CV_8UC1, pBuffer.get());
                if (raw.empty()) { continue; }

                if (isColorCamera) {
                    cv::Mat debayered;
                    cv::cvtColor(raw, debayered, cv::COLOR_BayerRG2BGR);

                    // Convert BGR → mono (green channel)
                    cv::Mat mono;
                    cv::extractChannel(debayered, mono, 1);   // G channel
                    frame = mono;  
                } else {
                    frame = raw; 
                }
            }   else if (imgType == ASI_IMG_RGB24) {

                cv::Mat rgb(height, width, CV_8UC3, pBuffer.get());
                if (rgb.empty()) { continue; }

                // Convert RGB → BGR for OpenCV consistency
                cv::Mat bgr;
                cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);

                // Convert BGR → mono (green channel)
                cv::Mat mono;
                cv::extractChannel(bgr, mono, 1);
                frame = mono;
            }
            else {
                return;
            }

            emit frameReady(frame.clone());

        } else if (err == ASI_ERROR_TIMEOUT) {
            emit captureLoopTimeout();
            continue;
        } else {
            running = false;
            emit captureLoopErrorOccured(asiErrorToString(err));
            return;
        }
    }
}

bool CameraWorker::setGain(int value)
{
    if (SIMULATION_MODE) return true;

    ASI_ERROR_CODE err = ASISetControlValue(cameraID, ASI_GAIN, value, ASI_FALSE);

    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("setGain:: Failed to set gain. " + asiErrorToString(err));
        return false;
    }

    return true;
}

bool CameraWorker::setExposure(int value)
{
    if (SIMULATION_MODE) return true;

    ASI_ERROR_CODE err = ASISetControlValue(cameraID, ASI_EXPOSURE, value, ASI_FALSE);

    if (err != ASI_SUCCESS) {
        emit ASIErrorMessage("setExposure:: Failed to set exposure. " + asiErrorToString(err));
        return false;
    }

    return true;
}

QString CameraWorker::asiErrorToString(ASI_ERROR_CODE code)
{
    switch (code)
    {
        case ASI_SUCCESS: return "Success";
        case ASI_ERROR_CAMERA_CLOSED: return  "ASI_ERROR_CAMERA_CLOSED> Camera didn't open";
        case ASI_ERROR_INVALID_ID: return "ASI_ERROR_INVALID_ID>  No camera of this ID is connected or ID value is out of boundary";
        case ASI_ERROR_INVALID_CONTROL_TYPE: return "ASI_ERROR_INVALID_CONTROL_TYPE> Invalid control type";
        case ASI_ERROR_GENERAL_ERROR: return "ASI_ERROR_GENERAL_ERROR> General error, e.g., value is out of valid range; operation to camera hardware failed";
        default: return "Unknown ASI error";
    }
}

int CameraWorker::getASIDefaultGain() {
    ASI_CONTROL_CAPS controlCaps;
    if (ASIGetControlCaps(cameraID, ASI_GAIN, &controlCaps) == ASI_SUCCESS) {
        return controlCaps.DefaultValue;
    }
    return 135;
}

//
// Simulation mode methods
//

void CameraWorker::simulationCaptureLoop() {
    int width = 1304, height = 976;

    int bufferSize = width * height * 3;
    std::unique_ptr<unsigned char[]> pBuffer(new unsigned char[bufferSize]);

    while (running) {
        cv::Mat frame = cv::Mat::zeros(height, width, CV_8UC1);

        // Random star position
        int x = 500 + (rand() % 3 - 1);
        int y = 500 + (rand() % 3 - 1);

        // Random radius (tiny)
        int radius = 8 + (rand() % 2 - 1);

        // Random brightness
        int brightness = 235 + (rand() % 7 - 3); // 180–255

        // Draw a small white dot
        cv::circle(frame, cv::Point(x, y), radius, cv::Scalar(int(0.9 * brightness), brightness, int(0.8 * brightness)), -1);

        // Apply Gaussian blur to simulate star glow
        cv::GaussianBlur(frame, frame, cv::Size(9, 9), 20.0, 20.0);

        emit frameReady(frame.clone());
    }
}