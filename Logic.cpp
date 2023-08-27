#include "Logic.h"
#include <iostream>

int Logic::unknownStateCounter = 0; 

void Logic::handleDisabledState(CellularManager& cellularManager, int modem) {
    std::cout << "Modem is DISABLED. Enabling..." << std::endl;
    cellularManager.enableModem(modem);
    cellularManager.setupSignalChecking(modem);
}

void Logic::handleSearchingState(CellularManager& cellularManager, std::chrono::steady_clock::time_point& searchStartTime, int modem, int currentRSSI) {
    std::cout << "Modem is SEARCHING for network. Current RSSI level: " << currentRSSI << std::endl;

    if (currentRSSI < cellularManager.getMinRSSILevel()) {
        std::cout << "Low RSSI detected. Resetting the timer." << std::endl;
        searchStartTime = std::chrono::steady_clock::now(); // Reset the timer
        return;
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - searchStartTime).count() > cellularManager.getMaxConnectTime()) {
        std::cout << "Max connect time exceeded. Resetting hardware..." << std::endl;
        cellularManager.resetHw();
        searchStartTime = std::chrono::steady_clock::now();
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

void Logic::handleUnknownState(CellularManager& cellularManager, int modem) {
    std::cout << "Modem is in UNKNOWN state." << std::endl;

    ++unknownStateCounter;  // Increment the unknownStateCounter

    if (unknownStateCounter >= 3) {
        std::cout << "Modem stayed in UNKNOWN state for too long. Resetting hardware..." << std::endl;
        cellularManager.resetHw();
        unknownStateCounter = 0;  // Reset the counter
    }
}

void Logic::resetUnknownStateCounter() {
    unknownStateCounter = 0;
}