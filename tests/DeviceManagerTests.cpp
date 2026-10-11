#include "DeviceManager.h"

#include <iostream>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "[PASS] " << name << '\n';
        ++passed;
    } else {
        std::cout << "[FAIL] " << name << '\n';
        ++failed;
    }
}

int main() {
    std::vector<Device> devices = {
        {1, 36.5, true},
        {2, 82.0, true},
        {3, 25.0, false},
        {4, 80.0, true}
    };

    // Test 1: Online device count
    check(
        "Online device count",
        countOnlineDevices(devices) == 3
    );

    // Test 2: Alarm threshold is strictly greater than 80
    check(
        "Alarm count at 80 C",
        countActiveAlarms(devices, 80.0) == 1
    );

    // Test 3: A different threshold produces a different result
    check(
        "Alarm count at 70 C",
        countActiveAlarms(devices, 70.0) == 2
    );

    // Test 4: Add a new device
    bool added = addDevice(devices, {5, 42.0, true});

    check("Add new device", added);
    check("Device count after adding", devices.size() == 5);
    check("Online count after adding", countOnlineDevices(devices) == 4);

    // Test 5: Reject a duplicate ID
    const std::size_t sizeBeforeDuplicate = devices.size();

    bool duplicateAdded = addDevice(devices, {2, 99.0, true});

    check("Reject duplicate device ID", !duplicateAdded);
    check(
        "Duplicate does not change device count",
        devices.size() == sizeBeforeDuplicate
    );

    // Test 6: Reject an invalid ID
    bool invalidAdded = addDevice(devices, {0, 20.0, true});

    check("Reject non-positive device ID", !invalidAdded);
    check(
        "Invalid ID does not change device count",
        devices.size() == sizeBeforeDuplicate
    );

    std::cout << "\n========== Test Summary ==========\n";
    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}


