#include "CellularManager.h"
#include <dbus-c++/dispatcher.h> 
#include <dbus-c++/dbus.h>
#include <dbus-c++/types.h>
#include <iostream> // For logging
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <regex>

static const char* MODEM_MANAGER_PATH = "org/freedesktop/ModemManager";
static const char* MODEM_MANAGER_SERVICE = "org.freedesktop.ModemManager";
static const char* MODEM_MANAGER_INTERFACE = "org.freedesktop.ModemManager";


CellularManager::CellularManager() 
{
}

CellularManager::~CellularManager()
{
}


std::vector<int> CellularManager::getAvailableModems() {
    std::vector<int> modems;
    
    // Open the command for reading.
    FILE *fp = popen("mmcli -L", "r");
    if (fp == nullptr) {
        std::cerr << "Failed to run mmcli -L" << std::endl;
        return modems;
    }
    
    // Read the output a line at a time.
    char path[1035];
    while (fgets(path, sizeof(path) - 1, fp) != nullptr) {
        std::string line(path);
        
        // Regular expression to extract the index number.
        std::regex regex("/org/freedesktop/ModemManager1/Modem/(\\d+)");
        std::smatch match;
        if (std::regex_search(line, match, regex) && match.size() > 1) {
            // If the line contains a modem index, store it.
            modems.push_back(std::stoi(match.str(1)));
        }
    }
    
    // Close the file pointer.
    pclose(fp);
    
    return modems;
}


bool CellularManager::connectModem(const std::string& modemIdentifier, const std::string& apn, const std::string& username, const std::string& password) {
    std::cout << "Connecting to modem: " << modemIdentifier << std::endl;
    return true;
}

void CellularManager::disconnectModem(const std::string& modemIdentifier) {
    std::cout << "Disconnecting modem: " << modemIdentifier << std::endl;
}

bool CellularManager::isConnectionValidForCriticalData() const {
    return true;
}

void CellularManager::maintainConnection() {
    std::cout << "Maintaining connection..." << std::endl;
}

void CellularManager::logIssue(const std::string& issue) {
    std::cerr << "Issue: " << issue << std::endl;
}

int CellularManager::getModemSignalStrength(const std::string& modemIdentifier) const {
    return 0;
}

int CellularManager::getModemBER(const std::string& modemIdentifier) const {
    return 0;
}

void CellularManager::registerUnsolicitedListener(const UnsolicitedCallback& callback) {
    unsolicitedCallback = callback;
}

void CellularManager::unregisterUnsolicitedListener() {
    unsolicitedCallback = nullptr;
}

void CellularManager::handleUnsolicitedIndication(const std::string& message) {
    if (unsolicitedCallback) {
        unsolicitedCallback(message);
    }
}