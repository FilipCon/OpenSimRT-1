#include "IMUCalibrator.h"
#include "INIReader.h"
#include "InverseKinematics.h"
#include "NGIMUInputDriver.h"
#include "OpenSimUtils.h"

#include "Settings.h"
#include "Simulation.h"
#include "Visualization.h"

#include <Common/TimeSeriesTable.h>
#include <OpenSim/Common/CSVFileAdapter.h>
#include <OpenSim/Common/STOFileAdapter.h>
#include <SimTKcommon/SmallMatrix.h>
#include <SimTKcommon/internal/BigMatrix.h>
#include <SimTKcommon/internal/State.h>
#include <Simulation/Model/Model.h>
#include <chrono>
#include <cmath>
#include <exception>
#include <future>
#include <iostream>
#include <simbody/internal/Visualizer.h>
#include <thread>

using namespace std;
using namespace OpenSim;
using namespace OpenSimRT;
using namespace SimTK;

void run() {
    INIReader ini(INI_FILE);
    auto section = "LOWER_BODY_NGIMU";
    auto IMU_IP = ini.getVector(section, "IMU_IP", vector<string>());
    auto LISTEN_IP = ini.getString(section, "LISTEN_IP", "0.0.0.0");
    auto SEND_PORTS = ini.getVector(section, "IMU_SEND_PORTS", vector<int>());
    auto LISTEN_PORTS =
            ini.getVector(section, "IMU_LISTEN_PORTS", vector<int>());
    auto IMU_BODIES = ini.getVector(section, "IMU_BODIES", vector<string>());
    auto subjectDir = DATA_DIR + ini.getString(section, "SUBJECT_DIR", "");
    auto modelFile = subjectDir + ini.getString(section, "MODEL_FILE", "");

    // initial timetag (as decimal) of simulation
    const auto initTime =
            static_cast<double>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::high_resolution_clock::now()
                                    .time_since_epoch())
                            .count()) /
            1000000000;

    // setup model
    Model model(modelFile);

    // add marker to pelvis center to track model position from imus
    auto pelvisMarker =
            Marker("PelvisCenter", model.getBodySet().get("pelvis"), Vec3(0));
    model.addMarker(&pelvisMarker);
    model.finalizeConnections();

    State state = model.initSystem();
    model.realizePosition(state);
    auto height = model.getBodySet().get("pelvis").findStationLocationInGround(
            state, Vec3(0));

    // marker tasks
    vector<InverseKinematics::MarkerTask> markerTasks;
    vector<string> markerObservationOrder;
    InverseKinematics::createMarkerTasksFromMarkerNames(
            model, vector<string>{"PelvisCenter"}, markerTasks,
            markerObservationOrder);

    // imu tasks
    vector<InverseKinematics::IMUTask> imuTasks;
    vector<string> imuObservationOrder(IMU_BODIES);
    InverseKinematics::createIMUTasksFromObservationOrder(
            model, imuObservationOrder, imuTasks);

        // driver
    NGIMUInputDriver driver;
    driver.setupInput(imuObservationOrder,
                      vector<string>(LISTEN_PORTS.size(), LISTEN_IP),
                      LISTEN_PORTS);
    driver.setupTransmitters(IMU_IP, SEND_PORTS, LISTEN_IP, LISTEN_PORTS);
    thread listen(&NGIMUInputDriver::startListening, &driver);
    auto imuLogger = driver.initializeLogger();

    // calibrator
    IMUCalibrator clb = IMUCalibrator(model, &driver, imuObservationOrder);
    clb.record(3.0); // record for 3 seconds
    clb.computeheadingRotation("pelvis", SimTK::CoordinateDirection(SimTK::ZAxis, 1));
    clb.calibrateIMUTasks(imuTasks);

    // initialize ik (lower constraint weight and accuracy -> faster tracking)
    InverseKinematics ik(model, markerTasks, imuTasks, SimTK::Infinity, 1e-5);
    auto qLogger = ik.initializeLogger();

    // visualizer
    BasicModelVisualizer visualizer(model);

    try { // main loop
        while (true) {
            // get input from imus
            auto imuDataFrame = driver.getFrame();

            // solve ik
            auto pose = ik.solve(
                clb.transform(imuDataFrame, vector<SimTK::Vec3>{Vec3(0, 0, 0) + height}));

            // visualize
            visualizer.update(pose.q);

            // record
            qLogger.appendRow(pose.t - initTime, ~pose.q);
            imuLogger.appendRow(imuDataFrame.first - initTime,
                                ~driver.asVector(imuDataFrame));
        }
    } catch (std::exception& e) {
        cout << e.what() << endl;
        driver.stopListening();
        listen.join();

        // store results
        STOFileAdapter::write(
                qLogger, subjectDir + "real_time/inverse_kinematics/q.sto");
        CSVFileAdapter::write(
                imuLogger, subjectDir + "experimental_data/ngimu_data.csv");
    }
}

int main(int argc, char* argv[]) {
    try {
        run();
    } catch (exception& e) {
        cout << e.what() << endl;
        return -1;
    }
    return 0;
}