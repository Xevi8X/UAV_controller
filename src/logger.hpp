#pragma once
#include <Eigen/Dense>
#include <iostream>
#include <fstream>
#include <initializer_list>

#define LOGGER_MASK 5

class Logger
{
public:
    Logger(std::string path, std::string fmt, uint8_t group = 0);
    ~Logger();

    void log(double time, std::initializer_list<Eigen::Vector3d> args);

private:
    std::ofstream file;
    const uint8_t group;
};

