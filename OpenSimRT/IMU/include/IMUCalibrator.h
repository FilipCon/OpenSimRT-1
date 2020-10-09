#pragma once

#include "InverseKinematics.h"
#include "NGIMUData.h"
#include "NGIMUInputDriver.h"
#include "internal/IMUExports.h"

#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/CoordinateAxis.h>
#include <SimTKcommon/internal/Quaternion.h>
#include <SimTKcommon/internal/ReferencePtr.h>
#include <SimTKcommon/internal/Rotation.h>
#include <SimTKcommon/internal/State.h>
#include <Simulation/Model/Model.h>
#include <Simulation/Model/PhysicalFrame.h>
#include <iostream>
#include <string>
#include <vector>

namespace OpenSimRT {

class IMU_API IMUCalibrator {
 public:
    IMUCalibrator(const OpenSim::Model& otherModel,
                  InputDriver<NGIMUData>* const driver,
                  const std::vector<std::string>& observationOrder);

    void record(const double& timeout);

    void computeheadingRotation(
            const std::string& baseImuName,
            const SimTK::CoordinateDirection baseHeadingDirection);
    void calibrateIMUTasks(std::vector<InverseKinematics::IMUTask>& imuTasks);

    InverseKinematics::Input
    transform(const std::pair<double, std::vector<NGIMUData>>& imuData,
              const std::vector<SimTK::Vec3>& markerData);

 private:
    void computeAvgStaticPose();
    SimTK::Vec3 computeHeadingCorrection(
            const std::string& baseImuName,
            const SimTK::CoordinateDirection baseHeadingDirection);

    OpenSim::Model model;
    SimTK::State state;
    SimTK::ReferencePtr<InputDriver<NGIMUData>> m_driver;
    std::vector<NGIMUData> initIMUData;
    std::vector<std::vector<NGIMUData>> quatTable;
    std::map<std::string, SimTK::Rotation> imuBodiesInGround;
    std::vector<std::string> imuBodiesObservationOrder;
    SimTK::Rotation R_GoGi;
    SimTK::Rotation R_heading;

};
} // namespace OpenSimRT