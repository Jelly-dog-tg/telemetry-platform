#pragma once

#include "Device.h"
#include <string>
#include <vector>

bool loadDevicesFromCsv(
    const std::string& filename,
    std::vector<Device>& devices
);

bool saveDevicesToCsv(
    const std::vector<Device>& devices,
    const std::string& filename
);