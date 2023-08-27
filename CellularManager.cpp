#include "CellularManager.h"
#include <iostream> // For logging
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <regex>
#include <algorithm>
#include <getopt.h>
#include <cstdlib>

static const char* MODEM_MANAGER_PATH = "org/freedesktop/ModemManager";
static const char* MODEM_MANAGER_SERVICE = "org.freedesktop.ModemManager";
static const char* MODEM_MANAGER_INTERFACE = "org.freedesktop.ModemManager";
const std::string CellularManager::DEFAULT_APN = "rem8.com";
const std::string CellularManager::DEFAULT_IPTYPE = "ipv4";

CellularManager::CellularManager() 
{
}

CellularManager::~CellularManager()
{
}

std::string CellularManager::getModemApn(int modemIndex) {
    std::string cmd = "mmcli --modem=" + std::to_string(modemIndex) + " | grep -E '\\|\\s+initial bearer apn:' | awk '{print $NF}'";
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    try {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw std::runtime_error("An exception occurred while reading from the pipe.");
    }

    pclose(pipe);

    // Trim the new line at the end
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());

    if (result.empty()) {
        return DEFAULT_APN;
    }

    std::cout << "APN Found: " << result << std::endl; // Optional, remove if not needed
    return result;
}

std::string CellularManager::getModemIpType(int modemIndex) {
    std::string cmd = "mmcli --modem=" + std::to_string(modemIndex) + " | grep -E '\\|\\s+initial bearer ip type:' | awk '{print $NF}'";
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    try {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw std::runtime_error("An exception occurred while reading from pipe");
    }

    pclose(pipe);

    // Trim the new line at the end
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());

    if (result.empty()) {
        return DEFAULT_IPTYPE;
    }

    std::cout << "IP Found: " << result << std::endl;
    return result;
}

void CellularManager::setupSignalChecking(int modemIndex) {
    // Setup the signal checking
    std::string setupCmd = "mmcli --modem=" + std::to_string(modemIndex) + " --signal-setup=5";
    int ret = std::system(setupCmd.c_str());

    if (ret != 0) {
        std::cerr << "Failed to set up signal checking for modem index " << modemIndex << std::endl;
    } else {
        std::cout << "Successfully set up signal checking for modem index " << modemIndex << std::endl;
    }
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
    
    if (!modems.empty()) {
        std::cout << "Available modem indexes: ";
        for (const auto& modemIndex : modems) {
            std::cout << modemIndex << ' ';
        }
        std::cout << std::endl;
    } else {
        std::cout << "No available modems." << std::endl;
    }

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
    if (result.find("disabled") != std::string::npos) {
        connectionStatus = State::DISABLED;
    } else if (result.find("searching") != std::string::npos) {
        connectionStatus = State::SEARCHING;
    } else if (result.find("registered") != std::string::npos) {
        connectionStatus = State::REGISTERED;
    } else if (result.find("connected") != std::string::npos) {
        connectionStatus = State::CONNECTED;
    } else {
        connectionStatus =  State::UNKNOWN;
    }

    std::cout << "Modem status:" <<  connectionStatus << std::endl;
    return connectionStatus;
}

void CellularManager::enableModem(int modemIndex) {
    std::string enableCmd = "mmcli --modem=" + std::to_string(modemIndex) + " --enable";
    int result = std::system(enableCmd.c_str());
    if (result == 0) {
        std::cout << "Modem enabled successfully." << std::endl;
    } else {
        std::cerr << "Failed to enable modem." << std::endl;
    }
}

bool CellularManager::connectModem(int modemIndex) {
    std::string apn = getModemApn(modemIndex);
    std::string ipType = getModemIpType(modemIndex);
    
    std::string connectCmd = "mmcli --modem=" + std::to_string(modemIndex) +
                             " --simple-connect='apn=" + apn + ",ip-type=" + ipType + "'";
    
    int result = std::system(connectCmd.c_str());
    if (result == 0) {
        std::cout << "Modem connected successfully." << std::endl;
        return true;
    } else {
        std::cerr << "Failed to connect modem." << std::endl;
        return false;
    }
}

int CellularManager::getModemSignalStrength(int modemIndex) {
    std::string signalCommand = "mmcli --modem=" + std::to_string(modemIndex) + " --signal-get";
    FILE* fp = popen(signalCommand.c_str(), "r");
    
    if (fp == nullptr) {
        std::cerr << "Failed to run command" << std::endl;
        return -1;  // Indicating an error
    }
    
    char buffer[128];
    std::string signalInfo = "";
    while (fgets(buffer, sizeof(buffer)-1, fp) != nullptr) {
        signalInfo += buffer;
    }
    
    pclose(fp);
    
    // Parsing the output to find the signal strength
    std::string keyword = "rssi:";
    auto pos = signalInfo.find(keyword);
    if (pos == std::string::npos) {
        std::cout << "Could not find RSSI information" << std::endl;
        return -1;
    }
    
    std::stringstream ss(signalInfo.substr(pos + keyword.size()));
    int rssi;
    ss >> rssi;

    // Print the RSSI value
    std::cout << "Modem signal strength: " << rssi << " dBm" << std::endl;

    return rssi;
}

void CellularManager::resetHw() {
    //stub
}

int CellularManager::getMinRSSILevel() {
    return minRSSILevel;
}

int CellularManager::getMaxConnectTime() {
    return maxConnectTime;
}

void CellularManager::parseCommandLine(int argc, char *argv[]) {
    int opt;
    static struct option long_options[] = {
        {"connect", required_argument, 0, 'c'},
        {"minRSSI", required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "c:r:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                maxConnectTime = std::stoi(optarg);
                std::cout << "Max connect time set to: " << maxConnectTime << std::endl;
                break;
            case 'r':
                minRSSILevel = std::stoi(optarg);
                std::cout << "Minimum RSSI Level set to: " << minRSSILevel << std::endl;
                break;
            default:
                std::cerr << "Invalid option" << std::endl;
                break;
        }
    }
}

