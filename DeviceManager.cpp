#include "DeviceManager.h"
#include <cmath>
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
bool updateDeviceTelemetry(
    std::vector<Device>& devices,
    int deviceId,
    double temperature,
    bool online,
    int sequence
) {
    // Reject invalid input.
    if (deviceId <= 0 ||
        !std::isfinite(temperature) ||
        sequence <= 0) {
        return false;
    }

    // Find the existing device.
    for (Device& device : devices) {
        if (device.id == deviceId) {

            // Reject duplicate or stale messages.
            if (sequence <= device.sequence) {
                return false;
            }

            // Update the device state.
            device.temperature = temperature;
            device.online = online;
            device.sequence = sequence;

            return true;
        }
    }

    // The device does not exist.
    return false;
}
