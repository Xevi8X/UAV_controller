#include "sensors.hpp"
#include <Eigen/Dense>
#include <random>
#include "environment.hpp"

template <class T>
std::mt19937 Sensor<T>::gen = std::mt19937(std::random_device()());

template <class T>
Sensor<T>::Sensor(Environment &env, double sd, T bias,
    std::string path, std::string fmt):
    env{env}, dist(0.0,sd), bias{bias}, logger(path,fmt,1)
{
    value = T();
}

template <class T>
double Sensor<T>::error()
{
    return dist(gen);
}

Accelerometer::Accelerometer(Environment &env, double sd):
    Sensor<Eigen::Vector3d>(env, sd, Eigen::Vector3d(0.0,0.0,0.0), "accelerometer.csv", "Time,AccX,AccY,AccZ"),
    g(0.0,0.0,9.81)
{}

void Accelerometer::update() 
{
    double time = env.getTime();
    auto rnb = env.getRnb();
    auto accel = env.getLinearAcceleration();

    value = accel + rnb*g + Eigen::Vector3d(error(),error(),error()) + bias;
    logger.log(time,{value});
}

Gyroscope::Gyroscope(Environment &env, double sd):
    Sensor<Eigen::Vector3d>(env, sd, Eigen::Vector3d(0.01,-0.02,0.03), "gyroscope.csv", "Time,GyrX,GyrY,GyrZ")
{}

void Gyroscope::update() 
{
    double time = env.getTime();
    value = env.getAngularVelocity() + Eigen::Vector3d(error(),error(),error()) + bias;
    logger.log(time,{value});
}

Magnetometer::Magnetometer(Environment &env, double sd):
    Sensor<Eigen::Vector3d>(env, sd, Eigen::Vector3d(0.0,0.0,0.0), "magnetometer.csv", "Time,MagX,MagY,MagZ"),
    mag(60.0,0.0,0.0)
{}

void Magnetometer::update() 
{
    double time = env.getTime();
    auto rnb = env.getRnb();
    value = rnb*mag + Eigen::Vector3d(error(),error(),error()) + bias;
    logger.log(time,{value});
}

Barometer::Barometer(Environment &env, double sd):
    Sensor<double>(env, sd, 0.0, "barometer.csv", "Time,Height")
{}

void Barometer::update()
{
    double time = env.getTime();
    auto pos = env.getPosition();
    value = pos(2);
    logger.log(time,{value});
}
