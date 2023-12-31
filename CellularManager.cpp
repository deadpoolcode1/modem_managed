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
#include <syslog.h>

static const char* MODEM_MANAGER_PATH = "org/freedesktop/ModemManager";
static const char* MODEM_MANAGER_SERVICE = "org.freedesktop.ModemManager";
static const char* MODEM_MANAGER_INTERFACE = "org.freedesktop.ModemManager";
std::string ipType = "ipv4"; // Default value for ipType
std::string APN = "rem8.com"; // Default value for APN
int maxConnectTime = 90; // Default value for maxConnectTime
int minRSSILevel = -90; // Default value for minRSSILevel

CellularManager::CellularManager() 
{
    openlog("modem", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

CellularManager::~CellularManager()
{
    closelog();
}

std::string CellularManager::getModemApn(int modemIndex) {
    return APN;
}

std::string CellularManager::getModemInfo(int modemIndex, const std::string& infoType) {
    return executeCommand("mmcli --modem=" + std::to_string(modemIndex) + " | grep -E '\\|\\s+initial bearer " + infoType + ":' | awk '{print $NF}'");
}

std::string CellularManager::getModemIpType(int modemIndex) {
    return ipType;
}

void CellularManager::setupSignalChecking(int modemIndex) {
    int ret = std::system(("mmcli --modem=" + std::to_string(modemIndex) + " --signal-setup=5").c_str());
    syslog(LOG_INFO, "Successfully set up signal checking for modem index %d", modemIndex);
    if (ret) {
        syslog(LOG_ERR, "Failed to set up signal checking for modem index %d", modemIndex);
    }
}

std::vector<int> CellularManager::getAvailableModems() {
    std::vector<int> modems;
    
    FILE *fp = popen("mmcli -L", "r");
    if (fp == nullptr) {
        syslog(LOG_ERR, "Failed to run mmcli -L");
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
        syslog(LOG_INFO, "Available modem indexes found."); 
    } else {
        syslog(LOG_WARNING, "No available modems."); 
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
    return connectionStatus;
}

void CellularManager::enableModem(int modemIndex) {
    std::string enableCmd = "mmcli --modem=" + std::to_string(modemIndex) + " --enable";
    int result = std::system(enableCmd.c_str());

    if (result == 0) {
        syslog(LOG_INFO, "Modem enabled successfully."); // INFO level for successful operations
    } else {
        syslog(LOG_ERR, "Failed to enable modem."); // ERR level for failures
    }
}

bool CellularManager::connectModem(int modemIndex) {
    std::string apn = getModemApn(modemIndex);
    std::string ipType = getModemIpType(modemIndex);
    
    std::string connectCmd = "mmcli --modem=" + std::to_string(modemIndex) +
                             " --simple-connect='apn=" + apn + ",ip-type=" + ipType + "'";
    
    int result = std::system(connectCmd.c_str());
    if (result == 0) {
        syslog(LOG_INFO, "Modem connected successfully."); // INFO level for successful operations
    } else {
        syslog(LOG_ERR, "Failed to connect modem."); // ERR level for failures
    }

    return result == 0;
}

int CellularManager::getModemSignalStrength(int modemIndex) {
    std::string signalCommand = "mmcli --modem=" + std::to_string(modemIndex) + " --signal-get";
    FILE* fp = popen(signalCommand.c_str(), "r");
    
    if (fp == nullptr) {
        syslog(LOG_ERR, "Failed to run command");
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
        syslog(LOG_WARNING, "Could not find RSSI information.");
        return -1;
    }
    
    std::stringstream ss(signalInfo.substr(pos + keyword.size()));
    int rssi;
    ss >> rssi;

    // Print the RSSI value
    syslog(LOG_INFO, "Modem signal strength: %d dBm", rssi);

    return rssi;
}

void CellularManager::assignIp(int modemIndex) {
    auto runCmd = [this, modemIndex](const std::string& pattern) {
        std::string cmd = "mmcli --modem=" + std::to_string(modemIndex) + pattern;
        return executeCommand(cmd);
    };

    std::string bearerIndex = runCmd(" | grep -E 'Bearer\\s+\\|\\s+paths:' | awk -F'/' '{print $NF}'");
    std::string ipInterface = runCmd(" --bearer=" + bearerIndex + " | grep -E '\\|\\s+interface:' | awk '{print $NF}'");
    std::string ipAddress   = runCmd(" --bearer=" + bearerIndex + " | grep -E '\\|\\s+address:' | awk '{print $NF}'");
    std::string ipGateway   = runCmd(" --bearer=" + bearerIndex + " | grep -E '\\|\\s+gateway:' | awk '{print $NF}'");
    std::string ipMtu       = runCmd(" --bearer=" + bearerIndex + " | grep -E '\\|\\s+mtu:' | awk '{print $NF}'");
    std::string dnsString   = runCmd(" --bearer=" + bearerIndex + " | grep -E '\\|\\s+dns:'");

    std::vector<std::string> ipDnsArray;
    std::regex re("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
    for (std::sregex_iterator it = std::sregex_iterator(dnsString.begin(), dnsString.end(), re); it != std::sregex_iterator(); ++it) {
        ipDnsArray.push_back(it->str());
    }

    std::system(("ip link set " + ipInterface + " up").c_str());
    std::system(("ip addr add " + ipAddress + "/32 dev " + ipInterface).c_str());
    std::system(("ip link set dev " + ipInterface + " arp off").c_str());
    std::system(("ip link set dev " + ipInterface + " mtu " + ipMtu).c_str());
    std::system(("ip route add default dev " + ipInterface + " metric 200").c_str());

    for (const auto& dns : ipDnsArray) {
        std::system(("sh -c \"echo 'nameserver " + dns + "' >> /etc/resolv.conf\"").c_str());
    }
}


std::string CellularManager::executeCommand(const std::string& cmd) {
    char buffer[128];
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result;
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
        {"ipType", required_argument, 0, 'i'},
        {"APN", required_argument, 0, 'a'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "c:r:i:a:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                maxConnectTime = std::stoi(optarg);
                std::cout << "Max connect time set to: " << maxConnectTime << std::endl;
                break;
            case 'r':
                minRSSILevel = std::stoi(optarg);
                std::cout << "Minimum RSSI Level set to: " << minRSSILevel << std::endl;
                break;
            case 'i':
                ipType = optarg;
                std::cout << "IP type set to: " << ipType << std::endl;
                break;
            case 'a':
                APN = optarg;
                std::cout << "APN set to: " << APN << std::endl;
                break;
            case 'h':
                std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -c, --connect TIME     Set max connect time\n"
                      << "  -r, --minRSSI LEVEL    Set minimum RSSI level\n"
                      << "  -i, --ipType TYPE      Set IP type (IPv4 or IPv6)\n"
                      << "  -a, --APN APN          Set APN\n"
                      << "  -h, --help             Show this help message\n";
                exit(0);
            default:
                std::cerr << "Invalid option" << std::endl;
                break;
        }
    }
}

