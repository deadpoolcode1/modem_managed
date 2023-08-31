#include "CellularManager.h"
#include "ObjectManager.h"
#include <iostream> // For logging
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <regex>
#include <algorithm>
#include <getopt.h>
#include <cstdlib>
#include <dbus-c++/dispatcher.h>

static const char* MODEM_MANAGER_PATH = "org/freedesktop/ModemManager";
static const char* MODEM_MANAGER_SERVICE = "org.freedesktop.ModemManager";
static const char* MODEM_MANAGER_INTERFACE = "org.freedesktop.ModemManager";
const std::string CellularManager::DEFAULT_APN = "rem8.com";
const std::string CellularManager::DEFAULT_IPTYPE = "ipv4";

DBus::BusDispatcher CellularManager::dispatcher;

CellularManager::CellularManager() : conn(DBus::Connection::SystemBus())
{
    DBus::default_dispatcher = &dispatcher;
}

CellularManager::~CellularManager()
{
}

std::string CellularManager::getModemApn(int modemIndex) {
    std::string result = getModemInfo(modemIndex, "apn");
    return result.empty() ? DEFAULT_APN : result;
}

std::string CellularManager::getModemInfo(int modemIndex, const std::string& infoType) {
    return executeCommand("mmcli --modem=" + std::to_string(modemIndex) + " | grep -E '\\|\\s+initial bearer " + infoType + ":' | awk '{print $NF}'");
}

std::string CellularManager::getModemIpType(int modemIndex) {
    std::string result = getModemInfo(modemIndex, "ip type");
    return result.empty() ? DEFAULT_IPTYPE : result;
}

void CellularManager::setupSignalChecking(int modemIndex) {
    int ret = std::system(("mmcli --modem=" + std::to_string(modemIndex) + " --signal-setup=5").c_str());
    std::cout << (ret ? "Failed" : "Successfully") << " set up signal checking for modem index " << modemIndex << std::endl;
}

std::vector<int> CellularManager::getAvailableModems() {
    std::vector<int> modems;

    try {
        auto connection = sdbus::createSystemBusConnection();
        auto managerProxy = std::make_unique<ObjectManagerProxy>(*connection, "org.freedesktop.ModemManager1", "/org/freedesktop/ModemManager1");
        auto managed_objects = managerProxy->GetManagedObjects();
        const std::string prefix = "/org/freedesktop/ModemManager1/Modem/";
        for (const auto& p : managed_objects) {
            if (p.first.starts_with(prefix)) {
                const int id = std::stoi(p.first.substr(prefix.size()));
                modems.push_back(id);
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
    } catch (const DBus::Error& e) {
        std::cerr << "D-Bus error: " << e.what() << std::endl;
    }

    return modems;
}

void CellularManager::scanModems() {
    try {
        auto msg = DBus::CallMessage("org.freedesktop.ModemManager1",
            "/org/freedesktop/ModemManager1", "org.freedesktop.ModemManager1", "ScanDevices");

        auto resp = conn.send_blocking(msg);
        if (!resp.is_error()) {
            std::cout << "successfully requested to scan devices" << std::endl;
        }
    } catch (const DBus::Error& e) {
        std::cerr << "D-Bus error: " << e.what() << std::endl;
    }
}

CellularManager::State CellularManager::getState(int modemIndex) {
    std::map<std::string, sdbus::Variant> result;

    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem.Simple", "GetStatus");
        auto reply = modemProxy->callMethod(method);
        reply >> result;
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to get modem state." << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }

    connectionStatus =  State::UNKNOWN;

    if (result.find("state") != result.end()) {
        const uint32_t state = result["state"].get<uint32_t>();

        // Convert uint to enum
        if (state == 3) {
            connectionStatus = State::DISABLED;
        } else if (state == 7) {
            connectionStatus = State::SEARCHING;
        } else if (state == 8) {
            connectionStatus = State::REGISTERED;
        } else if (state == 11) {
            connectionStatus = State::CONNECTED;
        }
    }

    return connectionStatus;
}

void CellularManager::enableModem(int modemIndex) {
    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem", "Enable");
        method << true;
        auto reply = modemProxy->callMethod(method);

        std::cout << "Modem enabled successfully." << std::endl;
    }
    catch (const sdbus::Error& e) {
        std::cerr << "Failed to enable modem." << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }
}

bool CellularManager::connectModem(int modemIndex) {
    std::string apn = getModemApn(modemIndex);
    std::string ipType = getModemIpType(modemIndex);

    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem.Simple", "Connect");
        std::map<std::string, sdbus::Variant> properties = {
            {"apn", apn},
            {"ip-type", ipType}
        };
        method << properties;
        auto reply = modemProxy->callMethod(method);

        std::cout << "Modem connected successfully." << std::endl;
    }
    catch (const sdbus::Error& e) {
        std::cerr << "Failed to connect modem." << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }
}

void CellularManager::disconnectModem(const std::string& modemIdentifier) {
    const std::string prefix = "/org/freedesktop/ModemManager1/Modem/";
    const int idx = std::stoi(modemIdentifier.substr(prefix.size()));
    disconnectModem(idx);
}

void CellularManager::disconnectModem(int modemIndex) {
    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);

        // Disconnect all modem bearers
        for (const auto& b : listBearers(modemIndex)) {
            auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem.Simple", "Disconnect");
            method << b;
            auto reply = modemProxy->callMethod(method);
        }

        std::cout << "Modem disconnected successfully." << std::endl;
    }
    catch (const sdbus::Error& e) {
        std::cerr << "Failed to disconnect modem." << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }
}

std::vector<sdbus::ObjectPath> CellularManager::listBearers(int modemIndex) {
    std::vector<sdbus::ObjectPath> bearers;

    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem", "ListBearers");
        auto reply = modemProxy->callMethod(method);
        reply >> bearers;

        if (!bearers.empty()) {
            std::cout << "Available bearer pathes: " << std::endl;
            for (const auto& bearer : bearers) {
                std::cout << bearer << std::endl;
            }
            std::cout << std::endl;
        } else {
            std::cout << "No available bearers." << std::endl;
        }
    }
    catch (const sdbus::Error& e) {
        std::cerr << "Failed to list bearers." << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }

    return bearers;
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
        {"help", no_argument, 0, 'h'},
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
            case 'h':
                std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -c, --connect TIME     Set max connect time\n"
                      << "  -r, --minRSSI LEVEL    Set minimum RSSI level\n"
                      << "  -h, --help             Show this help message\n";
            exit(0);
            default:
                std::cerr << "Invalid option" << std::endl;
                break;
        }
    }
}

