#ifndef LOGIC_H
#define LOGIC_H

#include <chrono>
#include "CellularManager.h"

class Logic {
public:
    static void handleDisabledState(CellularManager& cellularManager, int modem);
    static void handleSearchingState(CellularManager& cellularManager, std::chrono::steady_clock::time_point& searchStartTime, int modem);
    static void handleRegisteredState(CellularManager& cellularManager, int modem, int currentRSSI);
    static void handleConnectedState(CellularManager& cellularManager, int modem, int currentRSSI);
};

#endif // LOGIC_H