#include "sensor.hpp"
#include <zmq.hpp>
#include <thread>
#include <Eigen/Dense>
#include <mutex>
#include <random>
#include <iostream>
#include "NS.hpp"

Sensor::Sensor(zmq::context_t *ctx, std::string uav_address, NS &navisys, bool &run, double sd1, double sd2, std::string name , std::string topic):
    _ctx{ctx}, _uav_address{uav_address}, _navisys{navisys}, _run{run}, _dist1(0.0,sd1), _dist2(0.0,sd2), _name{name}, _topic{topic}
{
    listener = std::thread([this] () {listenerJob();});
}

Sensor::~Sensor() 
{
    listener.join();
    std::cout << "Exiting " << _name <<  "!" << std::endl;
}

double Sensor::error1()
{
    //TODO: use _dist
    return 0.0;
}

double Sensor::error2()
{
    //TODO: use _dist
    return 0.0;
}

void Sensor::setPosition(Eigen::Vector3d newValue) {_navisys.setPosition(newValue);}

void Sensor::setOrientation(Eigen::Vector3d newValue) {_navisys.setOrientation(newValue);}

void Sensor::setLinearVelocity(Eigen::Vector3d newValue) {_navisys.setLinearVelocity(newValue);}

void Sensor::setAngularVelocity(Eigen::Vector3d newValue) {_navisys.setAngularVelocity(newValue);}

void Sensor::setLinearAcceleration(Eigen::Vector3d newValue) {_navisys.setLinearAcceleration(newValue);}

void Sensor::setAngularAcceleraton(Eigen::Vector3d newValue) {_navisys.setAngularAcceleraton(newValue);}

void Sensor::listenerJob() 
{
    std::cout << "Starting " << _name << " listener: " + _uav_address + "\n";
    zmq::socket_t sock = zmq::socket_t(*_ctx, zmq::socket_type::sub);
    sock.set(zmq::sockopt::rcvtimeo,200);
    sock.set(zmq::sockopt::conflate,1);
    sock.connect(_uav_address);
    sock.set(zmq::sockopt::subscribe, _topic);
    while(_run)
    {
        zmq::message_t msg;
        const auto res = sock.recv(msg, zmq::recv_flags::none);
        if(!res)
        {
            if(zmq_errno() != EAGAIN) std::cerr << _name << " listener recv error" << std::endl;
            continue;
        } 
        std::string msg_str =  std::string(static_cast<char*>(msg.data()), msg.size());
        handleMsg(msg_str);
    }
    sock.close();
    std::cout << "Ending " << _name << " listener: " << _uav_address << std::endl;
}

GNSS::GNSS(zmq::context_t* ctx, std::string uav_address, NS& navisys, bool& run, double sd):
    Sensor(ctx,uav_address,navisys,run,sd,0.0,"GNSS","pos:")
{
}

void GNSS::handleMsg(std::string msg) 
{
    std::istringstream f(msg.substr(4));
    std::string s;
    Eigen::Vector3d pos;
    int i; 
    for (i = 0; i < 6; i++)
    {
        if(!getline(f, s, ','))
        {
            std::cerr << "Invalid pos msg" << std::endl;
            break;
        }

        if(i < 3)
        {
            pos(i) = std::stod(s)+ error1();
        }
    }
    if(i != 6)
    {
        std::cerr << "Invalid pos msg" << std::endl;
        return;
    }
    setPosition(pos);
}

Gyroscope::Gyroscope(zmq::context_t* ctx, std::string uav_address, NS& navisys, bool& run, double sd):
    Sensor(ctx,uav_address,navisys,run,sd,0.0,"Gyroscope","vb:")
{
}

void Gyroscope::handleMsg(std::string msg)
{
    std::istringstream f(msg.substr(3));
    std::string s;
    Eigen::Vector3d angularVel;
    int i; 
    for (i = 0; i < 6; i++)
    {
        if(!getline(f, s, ','))
        {
            std::cerr << "Invalid vb msg" << std::endl;
            break;
        }

        if(i >= 3)
        {
            angularVel(i-3) = std::stod(s) + error1();
        }
    }
    if(i != 6)
    {
        std::cerr << "Invalid vb msg" << std::endl;
        return;
    }
    setAngularVelocity(angularVel);
}


Accelerometer::Accelerometer(zmq::context_t* ctx, std::string uav_address, NS& navisys, bool& run, double sd1, double sd2):
    Sensor(ctx,uav_address,navisys,run,sd1,sd2,"Accelerometer","ab:")
{
}

void Accelerometer::handleMsg(std::string msg)
{
    std::istringstream f(msg.substr(3));
    std::string s;
    Eigen::Vector3d linAcc,angAcc;
    int i; 
    for (i = 0; i < 6; i++)
    {
        if(!getline(f, s, ','))
        {
            std::cerr << "Invalid ab msg" << std::endl;
            break;
        }

        if(i < 3)
        {
            linAcc(i) = std::stod(s) + error1();
        }
        else
        {
            angAcc(i-3) = std::stod(s) + error2();
        }
    }
    if(i != 6)
    {
        std::cerr << "Invalid ab msg" << std::endl;
        return;
    }
    setLinearAcceleration(linAcc);
    setAngularAcceleraton(angAcc);
}

MagicOrientationSensor::MagicOrientationSensor(zmq::context_t *ctx, std::string uav_address, NS &navisys, bool &run):
    Sensor(ctx,uav_address,navisys,run,0.0,0.0,"Magic ori","pos:")
{
}

void MagicOrientationSensor::handleMsg(std::string msg) 
{
    std::istringstream f(msg.substr(4));
    std::string s;
    Eigen::Vector3d ori;
    int i; 
    for (i = 0; i < 6; i++)
    {
        if(!getline(f, s, ','))
        {
            std::cerr << "Invalid pos msg" << std::endl;
            break;
        }

        if(i >= 3)
        {
            ori(i-3) = std::stod(s);
        }
    }
    if(i != 6)
    {
        std::cerr << "Invalid pos msg" << std::endl;
        return;
    }
    setOrientation(ori);
}

MagicLinearVelocitySensor::MagicLinearVelocitySensor(zmq::context_t *ctx, std::string uav_address, NS &navisys, bool &run):
    Sensor(ctx,uav_address,navisys,run,0.0,0.0,"Magic vel","vb:")
{
}

void MagicLinearVelocitySensor::handleMsg(std::string msg)
{
    std::istringstream f(msg.substr(3));
    std::string s;
    Eigen::Vector3d linVel;
    int i; 
    for (i = 0; i < 6; i++)
    {
        if(!getline(f, s, ','))
        {
            std::cerr << "Invalid vb msg" << std::endl;
            break;
        }

        if(i < 3)
        {
            linVel(i) = std::stod(s);
        }
    }
    if(i != 6)
    {
        std::cerr << "Invalid vb msg" << std::endl;
        return;
    }
    setLinearVelocity(linVel);
}
