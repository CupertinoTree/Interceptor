// AlignmentModel.cpp
#include "AlignmentModel.h"
#include <cmath>
#include <iostream>

static constexpr double DEG2RAD = M_PI / 180.0;
static constexpr double RAD2DEG = 180.0 / M_PI;

Eigen::Vector3d AlignmentModel::altAzToVec(const AltAzCoords& aa) {
    double alt = aa.alt * DEG2RAD;
    double az  = aa.az  * DEG2RAD;
    double x = std::cos(alt) * std::cos(az);
    double y = std::cos(alt) * std::sin(az);
    double z = std::sin(alt);
    return Eigen::Vector3d(x, y, z);
}

AltAzCoords AlignmentModel::vecToAltAz(const Eigen::Vector3d& v) {
    Eigen::Vector3d n = v.normalized();
    double alt = std::asin(n.z()) * RAD2DEG;
    double az  = std::atan2(n.y(), n.x()) * RAD2DEG;
    if (az < 0.0) az += 360.0;
    return AltAzCoords{alt, az};
}

void AlignmentModel::addSample(const AltAzCoords& trueSky, const AltAzCoords& mount) {
    skyVecs_.push_back(altAzToVec(trueSky));
    mountVecs_.push_back(altAzToVec(mount));
}

bool AlignmentModel::solve() {
    const std::size_t N = skyVecs_.size();
    if (N != 3 || mountVecs_.size() != N)
        return false; // this version is for exactly 3 stars

    // Build matrices: columns are vectors
    Eigen::Matrix3d M; // mount
    Eigen::Matrix3d S; // sky
    for (int i = 0; i < 3; ++i) {
        M.col(i) = mountVecs_[i].normalized();
        S.col(i) = skyVecs_[i].normalized();
    }

    // Direct rotation mapping: mount -> sky
    Eigen::Matrix3d Rraw = S * M.inverse();

    // Orthonormalize via polar decomposition: R = (Rraw) * ( (Rraw^T Rraw)^(-1/2) )
    Eigen::Matrix3d C = Rraw.transpose() * Rraw;
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(C);
    Eigen::Matrix3d D = es.eigenvalues().asDiagonal();
    Eigen::Matrix3d V = es.eigenvectors();

    // C^{-1/2}
    Eigen::Matrix3d Dinvhalf = Eigen::Matrix3d::Zero();
    for (int i = 0; i < 3; ++i) {
        double lambda = D(i,i);
        if (lambda > 1e-12)
            Dinvhalf(i,i) = 1.0 / std::sqrt(lambda);
    }
    Eigen::Matrix3d Cinvhalf = V * Dinvhalf * V.transpose();

    Eigen::Matrix3d R = Rraw * Cinvhalf;

    // Ensure det(R) = +1
    if (R.determinant() < 0.0) {
        R.col(2) = -R.col(2);
    }

    R_ = R;
    solved = true;

    // Diagnostics
    double maxErrDeg = 0.0, sumErrDeg = 0.0;
    for (int i = 0; i < 3; ++i) {
        Eigen::Vector3d pred = R_ * mountVecs_[i];
        double dot = std::clamp(pred.normalized().dot(skyVecs_[i].normalized()), -1.0, 1.0);
        double ang = std::acos(dot) * RAD2DEG;
        sumErrDeg += ang;
        maxErrDeg = std::max(maxErrDeg, ang);
    }
    std::cout << "3-star alignment: meanErrDeg=" << (sumErrDeg/3.0)
              << " maxErrDeg=" << maxErrDeg
              << " det(R)=" << R_.determinant() << "\n";

    return true;
}

double AlignmentModel::levelErrorDeg() const {
    // mount zenith in mount frame
    Eigen::Vector3d z_mount(0.0, 0.0, 1.0);

    // where that zenith points in true sky frame
    Eigen::Vector3d z_sky = R_ * z_mount;
    z_sky.normalize();

    // true zenith in sky frame
    Eigen::Vector3d z_true(0.0, 0.0, 1.0);

    double dot = std::clamp(z_sky.dot(z_true), -1.0, 1.0);
    double angRad = std::acos(dot);
    return angRad * RAD2DEG;   // this is your level error
}


AltAzCoords AlignmentModel::mountToTrueSky(const AltAzCoords& mount) const {
    Eigen::Vector3d vm = altAzToVec(mount);
    Eigen::Vector3d vs = R_ * vm; // corrected sky direction
    return vecToAltAz(vs);
}

AltAzCoords AlignmentModel::trueSkyToMount(const AltAzCoords& sky) const {
    Eigen::Vector3d vs = altAzToVec(sky);
    Eigen::Vector3d vm = R_.transpose() * vs;   // inverse rotation
    return vecToAltAz(vm);
}


