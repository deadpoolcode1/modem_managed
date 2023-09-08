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
    
    std::vector<int> availableModems;
    int currentRSSI = -100;
    std::chrono::steady_clock::time_point searchStartTime; 
    
    CellularManager::State previousState = CellularManager::UNKNOWN;  
    
    while (true) {
        availableModems = cellularManager.getAvailableModems();
        
        if (availableModems.empty()) {
            std::cout << "No modems found. Rescanning in 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue; 
        }
        
        while (!availableModems.empty()) {
            int modem = availableModems[0]; 
            CellularManager::State currentState = cellularManager.getState(modem);
            currentRSSI = cellularManager.getModemSignalStrength(modem);
            
            if (currentState == CellularManager::CONNECTED && previousState != CellularManager::CONNECTED) {
                cellularManager.assignIp(modem);
            }

            if (currentState != previousState) {
                Logic::resetUnknownStateCounter();
            }
            
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
                    break;
            }
            
            previousState = currentState;

            availableModems = cellularManager.getAvailableModems();
            
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    return 0;
}
