#include <iostream>
#include "CellularManager.h"
int main() {
    DBus::BusDispatcher dispatcher;
    DBus::default_dispatcher = &dispatcher;
    CellularManager cm;
    std::vector<std::string> availableModems = cm.getAvailableModems();
    std::cout << "Available modems: ";
    for (const auto& modem : availableModems) {
        std::cout << modem << " ";
    }
    std::cout << std::endl;
    return 0;
}