#include "controller.hpp"

Controller::Controller(zmq::context_t *ctx, std::string uav_address,int controlPort):
state(ctx, controlPort),
gps(ctx, uav_address),
gyro(ctx, uav_address),
control(ctx, uav_address)
{
    mode = ControllerMode::angle;
    loadPIDs("");
    jobs[ControllerMode::angle] = [&]()
		{
		Eigen::Vector3d pos = gps.getGPSPos();
		Eigen::Vector3d vel = gps.getGPSVel();
		Eigen::Vector3d ori = gps.getAH();
		Eigen::Vector3d angVel = gyro.getAngularVel();

		double demandedW = pids.at("Z").calc(state.demandedZ - pos(2));
		double demandedP = pids.at("Fi").calc(state.demandedFi - ori(0));
		double demandedQ = pids.at("Theta").calc(state.demandedTheta - ori(1));
		double demandedR = pids.at("Psi").calc(state.demandedPsi - ori(2));

        double climb_rate = pids.at("W").calc(demandedW-vel(2));
		double roll_rate = pids.at("Roll").calc(demandedP-angVel(0));
		double pitch_rate = pids.at("Pitch").calc(demandedQ-angVel(1));
		double yaw_rate = pids.at("Yaw").calc(demandedR-angVel(2));
		Eigen::VectorXd vec = controlMixer4(climb_rate,roll_rate,pitch_rate,yaw_rate);
		control.sendSpeed(vec); };

    jobs[ControllerMode::acro] = [&]()
		{
		Eigen::Vector3d angVel = gyro.getAngularVel();

        double climb_rate = state.throttle;
		double roll_rate = pids.at("Roll").calc(state.demandedP-angVel(0));
		double pitch_rate = pids.at("Pitch").calc(state.demandedQ-angVel(1));
		double yaw_rate = pids.at("Yaw").calc(state.demandedR-angVel(2));
		Eigen::VectorXd vec = controlMixer4(climb_rate,roll_rate,pitch_rate,yaw_rate);
		control.sendSpeed(vec); };

    //TODO add position mode.
}

Controller::~Controller()
{
}

void Controller::run()
{
}

void Controller::loadPIDs(std::string configPath)
{
    if(configPath.empty())
    {
        pids.insert(std::make_pair("Z",PID(step_time / 1000.0, 2.122, 0.035, -0.387, -1000, 1000)));
        pids.insert(std::make_pair("Fi",PID(step_time / 1000.0, 9.584, 0.798, 0.192, -1000, 1000)));
        pids.insert(std::make_pair("Theta",PID(step_time / 1000.0, 5.191, 0.228, 0.127, -1000, 1000)));
        pids.insert(std::make_pair("Psi",PID(step_time / 1000.0, 5.288, 0.230, -0.151, -1000, 1000)));

        pids.insert(std::make_pair("W",PID(step_time / 1000.0, -3556.149, -538.572, -112.917, 0, 1000)));
        pids.insert(std::make_pair("Roll",PID(step_time / 1000.0, -6.249, -0.904, -0.219, -250, 250)));
        pids.insert(std::make_pair("Pitch",PID(step_time / 1000.0, 6.304, 1.174, 0.433, -250, 250)));
        pids.insert(std::make_pair("Yaw",PID(step_time / 1000.0, 112.662, 22.778, 3.419, -250, 250)));
    }
}

void Controller::setMode(ControllerMode new_mode)
{
    for(auto pid: pids)
    {
        pid.second.clear();
    }
    mode = new_mode;
}