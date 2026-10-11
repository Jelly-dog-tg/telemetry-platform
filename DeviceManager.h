#pragma once

#include "Device.h"
#include <vector>

int countOnlineDevices(
    const std::vector<Device>& devices
);

int countActiveAlarms(
    const std::vector<Device>& devices,
    double threshold
);

bool addDevice(
    std::vector<Device>& devices,
    const Device& newDevice
);