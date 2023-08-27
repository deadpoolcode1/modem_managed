#include "CellularManager.h"
#include "Logic.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <chrono>   
#include <thread>   

int main(int argc, char *argv[]) {
    CellularManager cellularManager;
    cellularManager.parseCommandLine(argc, argv);
    std::vector<int> availableModems = cellularManager.getAvailableModems();
    int currentRSSI = -100; // Initialize to a suitable default value
    std::chrono::steady_clock::time_point searchStartTime; // Declare it here for SEARCHING state
    
    CellularManager::State previousState = CellularManager::UNKNOWN;  // Add a variable to keep track of the previous state
    
    if (!availableModems.empty()) {
        while (true) {
            int modem = availableModems[0]; // Or however you decide which modem to use
            CellularManager::State currentState = cellularManager.getState(modem);
            currentRSSI = cellularManager.getModemSignalStrength(modem);
            
            if (currentState == CellularManager::CONNECTED && previousState != CellularManager::CONNECTED) {
                // Call assignIP only when just changing state to CONNECTED
                cellularManager.assignIp(modem);
            }

            if (currentState != previousState) {
                Logic::resetUnknownStateCounter();
            }
            
            // Switch-case block
            switch (currentState) {
                case CellularManager::DISABLED:
                    Logic::handleDisabledState(cellularManager, modem);
                    break;
                    
                case CellularManager::SEARCHING:
                    Logic::handleSearchingState(cellularManager, searchStartTime, modem, currentRSSI);
                    break;
                    
                case CellularManager::REGISTERED:
                    Logic::handleRegisteredState(cellularManager, modem, currentRSSI);
                    break;
                    
                case CellularManager::CONNECTED:
                    Logic::handleConnectedState(cellularManager, modem, currentRSSI);
                    break;
                    
                case CellularManager::UNKNOWN:
                    // Handle UNKNOWN state, if necessary.
                    break;
            }

            // Update the previous state to the current state for the next iteration
            previousState = currentState;
            
            // Sleep before checking the state again
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    return 0;
}