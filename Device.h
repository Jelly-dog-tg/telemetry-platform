#pragma once

struct Device {
    int id;
    double temperature;
    bool online;
    int sequence = 0;
};
