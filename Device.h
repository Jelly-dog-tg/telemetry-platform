#pragma once

#include <ctime>

struct Device {
    int id;
    double temperature;
    bool online;
    int sequence;

    std::time_t lastHeartbeat = 0;
};
