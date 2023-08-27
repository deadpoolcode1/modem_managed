#include "CellularManager.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <chrono>   
#include <thread>   

int main(int argc, char *argv[]) {
    CellularManager cellularManager;
    cellularManager.parseCommandLine(argc, argv);
    std::vector<int> availableModems = cellularManager.getAvailableModems();
    int currentRSSI = -100;  // Initialize to a suitable default value

    if (!availableModems.empty()) {
        while (true) {
            CellularManager::State currentState = cellularManager.getState(availableModems[0]);
            currentRSSI = cellularManager.getModemSignalStrength(availableModems[0]);

            switch (currentState) {
                case CellularManager::DISABLED:
                    std::cout << "Modem is DISABLED. Enabling..." << std::endl;
                    cellularManager.enableModem(availableModems[0]);
                    cellularManager.setupSignalChecking(availableModems[0]);
                    break;
                case CellularManager::SEARCHING:
                    {
                        std::cout << "Modem is SEARCHING for network. Current RSSI level: " << currentRSSI << std::endl;
                        static std::chrono::steady_clock::time_point searchStartTime = std::chrono::steady_clock::now();

                        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - searchStartTime).count() > cellularManager.getMaxConnectTime()) {
                            std::cout << "Max connect time exceeded. Resetting hardware..." << std::endl;
                            cellularManager.resetHw();
                            searchStartTime = std::chrono::steady_clock::now(); // Reset the timer
                        }
                    }
                    break;
                case CellularManager::REGISTERED:
                    if (currentRSSI >= cellularManager.getMinRSSILevel()) {
                        std::cout << "Modem is REGISTERED and RSSI level is sufficient (" << currentRSSI << "). Connecting..." << std::endl;
                        cellularManager.connectModem(availableModems[0]);
                    } else {
                        std::cout << "Modem is REGISTERED but RSSI level (" << currentRSSI << ") is insufficient. Waiting..." << std::endl;
                    }
                    break;
                case CellularManager::CONNECTED:
                    break;
                case CellularManager::UNKNOWN:
                    // Handle UNKNOWN state
                    break;
            }

            // Sleep for a second before checking the state again
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 0;
}