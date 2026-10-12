#include "DeviceManager.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

int main() {
    // Test 1: Update an existing device.
    {
        std::vector<Device> devices = {
            {1, 36.5, true, 0},
            {2, 25.0, false, 4}
        };

        bool result = updateDeviceTelemetry(
            devices, 1, 92.5, true, 1
        );

        check(result, "Update existing device succeeds");

        check(
            std::abs(devices[0].temperature - 92.5) < 1e-9,
            "Updated temperature is correct"
        );

        check(
            devices[0].online,
            "Updated online status is correct"
        );

        check(
            devices[0].sequence == 1,
            "Updated sequence is correct"
        );

        check(
            devices[1].temperature == 25.0 &&
            !devices[1].online &&
            devices[1].sequence == 4,
            "Other devices remain unchanged"
        );
    }

    // Test 2: A nonexistent device must not be created.
    {
        std::vector<Device> devices = {
            {1, 36.5, true, 0}
        };

        bool result = updateDeviceTelemetry(
            devices, 99, 92.5, true, 1
        );

        check(
            !result,
            "Updating nonexistent device fails"
        );

        check(
            devices.size() == 1 &&
            devices[0].temperature == 36.5 &&
            devices[0].sequence == 0,
            "Failed update leaves data unchanged"
        );
    }

    // Test 3: Reject an invalid device ID.
    {
        std::vector<Device> devices = {
            {1, 36.5, true, 0}
        };

        check(
            !updateDeviceTelemetry(
                devices, 0, 92.5, true, 1
            ),
            "Zero device ID is rejected"
        );
    }

    // Test 4: Reject invalid temperatures.
    {
        std::vector<Device> devices = {
            {1, 36.5, true, 0}
        };

        double nanTemperature =
            std::numeric_limits<double>::quiet_NaN();

        double infiniteTemperature =
            std::numeric_limits<double>::infinity();

        check(
            !updateDeviceTelemetry(
                devices, 1, nanTemperature, true, 1
            ),
            "NaN temperature is rejected"
        );

        check(
            !updateDeviceTelemetry(
                devices, 1, infiniteTemperature, true, 1
            ),
            "Infinite temperature is rejected"
        );
    }

    // Test 5: Reject duplicate and stale sequences.
    {
        std::vector<Device> devices = {
            {1, 40.0, true, 5}
        };

        check(
            !updateDeviceTelemetry(
                devices, 1, 50.0, true, 5
            ),
            "Duplicate sequence is rejected"
        );

        check(
            !updateDeviceTelemetry(
                devices, 1, 45.0, true, 4
            ),
            "Older sequence is rejected"
        );

        check(
            devices[0].temperature == 40.0 &&
            devices[0].sequence == 5,
            "Rejected stale data does not change device"
        );

        check(
            updateDeviceTelemetry(
                devices, 1, 55.0, false, 6
            ),
            "Newer sequence is accepted"
        );

        check(
            devices[0].temperature == 55.0 &&
            !devices[0].online &&
            devices[0].sequence == 6,
            "Newer message correctly updates device"
        );
    }

    std::cout << "\nPassed: " << passed
              << ", Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
