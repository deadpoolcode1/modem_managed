#include "Logic.h"
#include <iostream>

void Logic::handleDisabledState(CellularManager& cellularManager, int modem) {
    std::cout << "Modem is DISABLED. Enabling..." << std::endl;
    cellularManager.enableModem(modem);
    cellularManager.setupSignalChecking(modem);
}

void Logic::handleSearchingState(CellularManager& cellularManager, std::chrono::steady_clock::time_point& searchStartTime, int modem) {
    std::cout << "Modem is SEARCHING for network. Current RSSI level: " << cellularManager.getModemSignalStrength(modem) << std::endl;

    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - searchStartTime).count() > cellularManager.getMaxConnectTime()) {
        std::cout << "Max connect time exceeded. Resetting hardware..." << std::endl;
        cellularManager.resetHw();
        searchStartTime = std::chrono::steady_clock::now(); // Reset the timer
    }
}

void Logic::handleRegisteredState(CellularManager& cellularManager, int modem, int currentRSSI) {
    if (currentRSSI >= cellularManager.getMinRSSILevel()) {
        std::cout << "Modem is REGISTERED and RSSI level is sufficient (" << currentRSSI << "). Connecting..." << std::endl;
        cellularManager.connectModem(modem);
    } else {
        std::cout << "Modem is REGISTERED but RSSI level (" << currentRSSI << ") is insufficient. Waiting..." << std::endl;
    }
}

void Logic::handleConnectedState(CellularManager& cellularManager, int modem, int currentRSSI) {
    std::cout << "Modem is CONNECTED. Current RSSI level: " << currentRSSI << std::endl;
}