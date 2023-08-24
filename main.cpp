#include "CellularManager.h"
#include <iostream>



int main() {
    CellularManager cellularManager;

    std::vector<int> availableModems = cellularManager.getAvailableModems();

    if (availableModems.empty()) {
        std::cout << "No modems available.\n";
    } else {
        std::cout << "Available modems:\n";
        for (int modem : availableModems) {
            std::cout << "  Modem index: " << modem << '\n';
        }
    }
    if (!availableModems.empty()) {
        cellularManager.getState(availableModems[0]);
    }
    
    return 0;
}

