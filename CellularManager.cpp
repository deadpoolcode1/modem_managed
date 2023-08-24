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
    
    FILE *fp = popen("mmcli -L", "r");
    if (fp == nullptr) {
        std::cerr << "Failed to run mmcli -L" << std::endl;
        return modems;
    }
    
    char path[1035];
    while (fgets(path, sizeof(path) - 1, fp) != nullptr) {
        std::string line(path);
        
        std::regex regex("/org/freedesktop/ModemManager1/Modem/(\\d+)");
        std::smatch match;
        if (std::regex_search(line, match, regex) && match.size() > 1) {
            modems.push_back(std::stoi(match.str(1)));
        }
    }
    
    pclose(fp);
    
    return modems;
}
CellularManager::State CellularManager::getState(int modemIndex) {
    std::string cmd = "mmcli --modem=" + std::to_string(modemIndex) + " | grep -E '\\|\\s+state:' | awk '{print $NF}'";
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }

    pclose(pipe);

    // Trim the new line at the end
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());

    // Convert string to enum
    if (result == "disabled") {
        connectionStatus = State::DISABLED;
    } else if (result == "searching") {
        connectionStatus = State::SEARCHING;
    } else if (result == "registered") {
        connectionStatus = State::REGISTERED;
    } else if (result == "connected") {
        connectionStatus = State::CONNECTED;
    } else {
        connectionStatus =  State::UNKNOWN;
    }

    return connectionStatus;
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

CellularManager::State getConnectionStatus(CellularManager::State connectionStatus) {
    return connectionStatus;
}