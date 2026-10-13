#include "DeviceManager.h"
#include <cmath>
#include <iostream>
#include <ctime>

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
Device deviceToAdd = newDevice;

if (deviceToAdd.lastHeartbeat == 0) {
    deviceToAdd.lastHeartbeat = std::time(nullptr);
}

devices.push_back(deviceToAdd);

return true;
}
void checkHeartbeatTimeout(
    std::vector<Device>& devices,
    int timeoutSeconds
)
{
    const std::time_t now = std::time(nullptr);

    for (Device& device : devices) {

        if (!device.online) {
            continue;
        }


        if (now - device.lastHeartbeat > timeoutSeconds) {

            device.online = false;

            std::cout
                << "[WARNING] Device "
                << device.id
                << " heartbeat timeout, set OFFLINE\n";
        }
    }
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
bool deviceExists(
    const std::vector<Device>& devices,
    int deviceId
) {
    for (const Device& device : devices) {
        if (device.id == deviceId) {
            return true;
        }
    }

    return false;
}

bool updateDeviceHeartbeat(
    std::vector<Device>& devices,
    int deviceId,
    int sequence
) {
    if (deviceId <= 0 || sequence <= 0) {
        return false;
    }

    for (Device& device : devices) {
        if (device.id == deviceId) {
            // Reject duplicate or stale messages.
            if (sequence <= device.sequence) {
                return false;
            }

            device.online = true;
            device.sequence = sequence;
            device.lastHeartbeat = std::time(nullptr);
return true;
        }
    }

    return false;
}
