#include "DeviceManager.h"

#include <iostream>

int countOnlineDevices(
    const std::vector<Device>& devices
) {
    int count = 0;

    for (const Device& device : devices) {
        if (device.online) {
            ++count;
        }
    }

    return count;
}

int countActiveAlarms(
    const std::vector<Device>& devices,
    double threshold
) {
    int count = 0;

    for (const Device& device : devices) {
        if (device.online &&
            device.temperature > threshold) {
            ++count;
        }
    }

    return count;
}

bool addDevice(
    std::vector<Device>& devices,
    const Device& newDevice
) {
    if (newDevice.id <= 0) {
        std::cerr << "Device ID must be positive.\n";
        return false;
    }

    for (const Device& device : devices) {
        if (device.id == newDevice.id) {
            std::cerr << "Device ID already exists.\n";
            return false;
        }
    }

    devices.push_back(newDevice);
    return true;
}