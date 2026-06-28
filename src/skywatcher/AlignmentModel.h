// AlignmentModel.h
#pragma once
#include <vector>
#include <Eigen/Dense>

struct AltAzCoords {
    double alt; // degrees
    double az;  // degrees
};

class AlignmentModel {
public:
    void addSample(const AltAzCoords& trueSky, const AltAzCoords& mount);
    bool solve(); // returns false if < 2 samples
    bool solved = false;
    AltAzCoords mountToTrueSky(const AltAzCoords& mount) const; // corrected sky Alt/Az
    AltAzCoords trueSkyToMount(const AltAzCoords& sky) const;

private:
    std::vector<Eigen::Vector3d> skyVecs_;
    std::vector<Eigen::Vector3d> mountVecs_;
    Eigen::Matrix3d R_ = Eigen::Matrix3d::Identity();

    static Eigen::Vector3d altAzToVec(const AltAzCoords& aa);
    static AltAzCoords vecToAltAz(const Eigen::Vector3d& v);
};

