#include "CsvStorage.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

bool loadDevicesFromCsv(
    const std::string& filename,
    std::vector<Device>& devices
) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open: "
                  << filename << '\n';
        return false;
    }

    std::string line;

    // Read and skip the CSV header.
    if (!std::getline(file, line)) {
        std::cerr << "CSV file is empty.\n";
        return false;
    }

    std::vector<Device> loadedDevices;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream row(line);

        Device device{};
        char comma1 = '\0';
        char comma2 = '\0';
        int onlineValue = -1;

        if (!(row >> device.id
                  >> comma1
                  >> device.temperature
                  >> comma2
                  >> onlineValue)
            || comma1 != ','
            || comma2 != ','
            || (onlineValue != 0 && onlineValue != 1)) {
            std::cerr << "Invalid CSV row: "
                      << line << '\n';
            continue;
        }

        // Reject unexpected extra data.
        row >> std::ws;

        if (!row.eof()) {
            std::cerr << "Extra data in row: "
                      << line << '\n';
            continue;
        }

        device.online = (onlineValue == 1);
        loadedDevices.push_back(device);
    }

    if (loadedDevices.empty()) {
        std::cerr << "No valid device records found.\n";
        return false;
    }

    devices = loadedDevices;
    return true;
}

bool saveDevicesToCsv(
    const std::vector<Device>& devices,
    const std::string& filename
) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to write: "
                  << filename << '\n';
        return false;
    }

    file << "id,temperature,online\n";
    file << std::fixed << std::setprecision(1);

    for (const Device& device : devices) {
        file << device.id << ','
             << device.temperature << ','
             << (device.online ? 1 : 0)
             << '\n';
    }

    file.flush();
    return file.good();
}