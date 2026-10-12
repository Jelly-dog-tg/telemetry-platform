#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include "Device.h"
#include "CsvStorage.h"
#include "DeviceManager.h"

void printDevices(
    const std::vector<Device>& devices,
    double threshold
) {
    for (const Device& device : devices) {
        std::cout << "Device #" << device.id
                  << " | Temperature: "
                  << std::fixed << std::setprecision(1)
                  << device.temperature << " C"
                  << " | Sequence: " << device.sequence
                  << " | Status: ";

        if (device.online) {
            std::cout << "ONLINE";
        } else {
            std::cout << "OFFLINE";
        }

        if (device.online && device.temperature > threshold) {
            std::cout << " | TEMPERATURE ALARM";
        }

        std::cout << '\n';
    }
}

bool readNewDevice(Device& newDevice) {
    int id;
    double temperature;
    int onlineInput;

    std::cout << "Enter device ID: ";
    if (!(std::cin >> id)) {
        std::cerr << "Invalid device ID.\n";
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
        return false;
    }

    std::cout << "Enter temperature: ";
    if (!(std::cin >> temperature)) {
        std::cerr << "Invalid temperature.\n";
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
        return false;
    }

    std::cout << "Enter online status (1=online, 0=offline): ";
    if (!(std::cin >> onlineInput) ||
        (onlineInput != 0 && onlineInput != 1)) {
        std::cerr << "Invalid online status.\n";
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );
        return false;
    }

    newDevice = {id, temperature, onlineInput == 1};
    return true;
}


int main() {
    std::vector<Device> devices;

    if (!loadDevicesFromCsv("devices.csv", devices)) {
        return 1;
    }

    const double alarmThreshold = 80.0;

    while (true) {
        std::cout << "\n===== Device Management =====\n";
        std::cout << "1. List devices\n";
        std::cout << "2. Add device\n";
        std::cout << "0. Exit\n";
        std::cout << "Select an option: ";

        int choice;

        if (!(std::cin >> choice)) {
            std::cerr << "Invalid menu input.\n";
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(),
                '\n'
            );
            continue;
        }

        if (choice == 1) {
            printDevices(devices, alarmThreshold);

            std::cout << "\nTotal devices: "
                      << devices.size() << '\n';

            std::cout << "Online devices: "
                      << countOnlineDevices(devices) << '\n';

            std::cout << "Active alarms: "
                      << countActiveAlarms(
                             devices, alarmThreshold
                         ) << '\n';
        }
        else if (choice == 2) {
            Device newDevice{};

            if (!readNewDevice(newDevice)) {
                continue;
            }

            if (!addDevice(devices, newDevice)) {
                continue;
            }

            if (!saveDevicesToCsv(devices, "devices.csv")) {
                std::cerr << "Failed to save device data.\n";
                return 1;
            }

            std::cout << "Device added and saved successfully.\n";
        }
        else if (choice == 0) {
            std::cout << "Exiting device manager.\n";
            return 0;
        }
        else {
            std::cout << "Invalid option. Please select 1, 2 or 0.\n";
        }
    }
}
