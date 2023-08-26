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
    if (!availableModems.empty()) {
        while (true) {
            CellularManager::State currentState = cellularManager.getState(availableModems[0]);
            int currentRSSI = 0;  // Initialize to a suitable default value

            switch (currentState) {
                case CellularManager::DISABLED:
                    std::cout << "Modem is DISABLED. Enabling..." << std::endl;
                    cellularManager.enableModem(availableModems[0]);
                    cellularManager.setupSignalChecking(availableModems[0]);
                    break;
                case CellularManager::SEARCHING:
                    // Handle SEARCHING state
                    break;
                case CellularManager::REGISTERED:
                    currentRSSI = cellularManager.getModemSignalStrength(availableModems[0]);
                    if (currentRSSI >= cellularManager.getMinRSSILevel()) {
                        std::cout << "Modem is REGISTERED and RSSI level is sufficient (" << currentRSSI << "). Connecting..." << std::endl;
                        cellularManager.connectModem(availableModems[0]);
                    } else {
                        std::cout << "Modem is REGISTERED but RSSI level (" << currentRSSI << ") is insufficient. Waiting..." << std::endl;
                    }
                    break;
                case CellularManager::CONNECTED:
                    cellularManager.getModemSignalStrength(availableModems[0]);
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