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


/*bool AlignmentModel::solve() {
    const std::size_t N = skyVecs_.size();
    if (N < 2 || mountVecs_.size() != N) return false;

    // Build cross-covariance H directly (no centroids)
    Eigen::Matrix3d H = Eigen::Matrix3d::Zero();
    for (std::size_t i = 0; i < N; ++i) {
        // map mount -> sky
        H += mountVecs_[i] * skyVecs_[i].transpose();
    }

    // SVD
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix3d U = svd.matrixU();
    Eigen::Matrix3d V = svd.matrixV();
    Eigen::Vector3d S = svd.singularValues();

    Eigen::Matrix3d R = U * V.transpose();

    // Fix reflection if needed
    if (R.determinant() < 0.0) {
        Eigen::Matrix3d D = Eigen::Matrix3d::Identity();
        D(2,2) = -1.0;
        R = U * D * V.transpose();
    }

    R_ = R;
    solved = true;

    // Diagnostics: residuals (angular errors) and SVD info
    double maxErrDeg = 0.0;
    double sumErrDeg = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        Eigen::Vector3d pred = R_ * mountVecs_[i];
        double dot = std::clamp(pred.normalized().dot(skyVecs_[i].normalized()), -1.0, 1.0);
        double ang = std::acos(dot) * RAD2DEG;
        sumErrDeg += ang;
        maxErrDeg = std::max(maxErrDeg, ang);
    }
    double meanErrDeg = sumErrDeg / static_cast<double>(N);

    std::cout << "Alignment solved: N=" << N
              << "  singulars=[" << S.transpose() << "]"
              << "  det(R)=" << R_.determinant()
              << "  meanErrDeg=" << meanErrDeg
              << "  maxErrDeg=" << maxErrDeg << "\n";

    return true;
}*/


/*bool AlignmentModel::solve() {
    const std::size_t N = skyVecs_.size();
    if (N < 2 || mountVecs_.size() != N)
        return false;

    // Compute centroids
    Eigen::Vector3d cSky  = Eigen::Vector3d::Zero();
    Eigen::Vector3d cMount = Eigen::Vector3d::Zero();
    for (std::size_t i = 0; i < N; ++i) {
        cSky  += skyVecs_[i];
        cMount += mountVecs_[i];
    }
    cSky  /= static_cast<double>(N);
    cMount /= static_cast<double>(N);

    // Build covariance matrix H
    Eigen::Matrix3d H = Eigen::Matrix3d::Zero();
    for (std::size_t i = 0; i < N; ++i) {
        Eigen::Vector3d ps = skyVecs_[i]   - cSky;
        Eigen::Vector3d pm = mountVecs_[i] - cMount;
        H += pm * ps.transpose(); // map mount -> sky
    }

    // SVD: H = U * S * V^T
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix3d U = svd.matrixU();
    Eigen::Matrix3d V = svd.matrixV();

    Eigen::Matrix3d R = U * V.transpose();

    // Ensure proper rotation (det = +1)
    if (R.determinant() < 0.0) {
        Eigen::Matrix3d D = Eigen::Matrix3d::Identity();
        D(2, 2) = -1.0;
        R = U * D * V.transpose();
    }

    R_ = R;
    solved = true;
    return true;
}*/

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


