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


bool updateDeviceTelemetry(
    std::vector<Device>& devices,
    int deviceId,
    double temperature,
    bool online,
    int sequence
);


bool updateDeviceHeartbeat(
    std::vector<Device>& devices,
    int deviceId,
    int sequence
);


bool deviceExists(
    const std::vector<Device>& devices,
    int deviceId
);


void checkHeartbeatTimeout(
    std::vector<Device>& devices,
    int timeoutSeconds
);
