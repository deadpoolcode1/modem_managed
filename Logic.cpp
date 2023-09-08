#include "Logic.h"
#include <iostream>
#include <syslog.h>

int Logic::unknownStateCounter = 0; 

void Logic::handleDisabledState(CellularManager& cellularManager, int modem) {
    syslog(LOG_INFO, "Modem is DISABLED. Enabling...");
    cellularManager.enableModem(modem);
    cellularManager.setupSignalChecking(modem);
}

void Logic::handleSearchingState(CellularManager& cellularManager, std::chrono::steady_clock::time_point& searchStartTime, int modem, int currentRSSI) {
    syslog(LOG_INFO, "Modem is SEARCHING for network. Current RSSI level: %d", currentRSSI);

    if (currentRSSI < cellularManager.getMinRSSILevel()) {
        syslog(LOG_INFO, "Low RSSI detected. Resetting the timer.");
        searchStartTime = std::chrono::steady_clock::now(); // Reset the timer
        return;
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - searchStartTime).count() > cellularManager.getMaxConnectTime()) {
        syslog(LOG_INFO, "Max connect time exceeded. Resetting hardware...");
        cellularManager.resetHw();
        searchStartTime = std::chrono::steady_clock::now();
    }
}

void Logic::handleRegisteredState(CellularManager& cellularManager, int modem, int currentRSSI) {
    if (currentRSSI >= cellularManager.getMinRSSILevel()) {
        syslog(LOG_INFO, "Modem is REGISTERED and RSSI level is sufficient (%d). Connecting...", currentRSSI);
        cellularManager.connectModem(modem);
    } else {
        syslog(LOG_INFO, "Modem is REGISTERED but RSSI level is insufficient (%d). Waiting...", currentRSSI);
    }
}

void Logic::handleConnectedState(CellularManager& cellularManager, int modem, int currentRSSI) {
    syslog(LOG_INFO, "Modem is CONNECTED. Current RSSI level: (%d). Waiting...", currentRSSI);
}

void Logic::handleUnknownState(CellularManager& cellularManager, int modem) {
    syslog(LOG_INFO, "Modem is in UNKNOWN state.");    

    ++unknownStateCounter;  // Increment the unknownStateCounter

    if (unknownStateCounter >= 3) {
        syslog(LOG_ERR, "Modem stayed in UNKNOWN state for too long. Resetting hardware...");
        cellularManager.resetHw();
        unknownStateCounter = 0;  // Reset the counter
    }
}

void Logic::resetUnknownStateCounter() {
    unknownStateCounter = 0;
}