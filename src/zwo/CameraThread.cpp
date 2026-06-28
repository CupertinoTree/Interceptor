#include "CameraThread.h"

CameraThread::CameraThread(int cameraID, QObject *parent)
    : QThread(parent), cameraID(cameraID), running(false), frameCount(0) {}

void CameraThread::startCapture() {
    running = true;
    start();
}

void CameraThread::stopCapture() {
    running = false;
    wait();
}

cv::Mat CameraThread::getLatestFrame() {
    QMutexLocker locker(&mutex);
    return latestFrame.clone();
}

int CameraThread::getAndResetFrameCount() {
    QMutexLocker locker(&countMutex);
    int count = frameCount;
    frameCount = 0;
    return count;
}

void CameraThread::run() {
    int width, height, bin;
    ASI_IMG_TYPE imgType;
    ASIGetROIFormat(cameraID, &width, &height, &bin, &imgType);
    int bufferSize = width * height * 3; // RGB24
    unsigned char* pBuffer = new unsigned char[bufferSize];

    while (running) {
        ASI_ERROR_CODE err = ASIGetVideoData(cameraID, pBuffer, bufferSize, 100); // Short timeout
        if (err == ASI_SUCCESS) {

            cv::Mat frame;

            if (!SIMULATION_MODE) {
                frame = cv::Mat(height, width, CV_8UC3, pBuffer);
                cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
            } else {
                // -----------------------------------------
                // Generate synthetic star frame
                // -----------------------------------------
                frame = cv::Mat::zeros(height, width, CV_8UC3);

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
            }

            {
                QMutexLocker locker(&mutex);
                latestFrame = frame.clone();
            }
            emit frameReady();
            {
                QMutexLocker locker(&countMutex);
                frameCount++;
                std::cout << "Emit frame ready" << std::endl;
            }
        }
    }
    delete[] pBuffer;
}