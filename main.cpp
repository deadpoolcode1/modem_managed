#include "CellularManager.h"
#include <iostream>
#include <chrono>   
#include <thread>   

int main() {
    CellularManager cellularManager;

    std::vector<int> availableModems = cellularManager.getAvailableModems();

    if (!availableModems.empty()) {
        while (true) {
            CellularManager::State currentState = cellularManager.getState(availableModems[0]);

            switch (currentState) {
                case CellularManager::DISABLED:
                    std::cout << "Modem is DISABLED. Enabling..." << std::endl;
                    cellularManager.enableModem(availableModems[0]);
                    break;
                case CellularManager::SEARCHING:
                    // Handle SEARCHING state
                    break;
                case CellularManager::REGISTERED:
                    // Handle REGISTERED state
                    break;
                case CellularManager::CONNECTED:
                    // Handle CONNECTED state
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

