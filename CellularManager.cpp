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
const CellularManager::IpFamily CellularManager::DEFAULT_IPTYPE =
    CellularManager::MM_BEARER_IP_FAMILY_IPV4;

using Properties = std::map<std::string, sdbus::Variant>;

DBus::BusDispatcher CellularManager::dispatcher;

CellularManager::CellularManager() : conn(DBus::Connection::SystemBus())
{
    DBus::default_dispatcher = &dispatcher;
}

CellularManager::~CellularManager()
{
}

std::string CellularManager::getModemApn(int modemIndex) {
    sdbus::Variant res = getModemInfo(modemIndex, "apn");
    return res.isEmpty() ? DEFAULT_APN : res.get<std::string>();
}

sdbus::Variant CellularManager::getModemInfo(int modemIndex, const std::string& infoType) {
    sdbus::Variant res;

    try {
        for (auto path : listBearers(modemIndex)) {
            auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", path);
            auto method = modemProxy->createMethodCall("org.freedesktop.DBus.Properties", "GetAll");
            method << "org.freedesktop.ModemManager1.Bearer";
            auto reply = modemProxy->callMethod(method);

            Properties properties;
            reply >> properties;
            const auto bearer_properties = properties["Properties"].get<Properties>();
            if (bearer_properties.find(infoType) != bearer_properties.end()) {
                res = bearer_properties.at(infoType);
                break;
            }
        }
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to get modem info for modem index " << modemIndex << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }

    return res;
}

CellularManager::IpFamily CellularManager::getModemIpType(int modemIndex) {
    sdbus::Variant res = getModemInfo(modemIndex, "ip-type");
    return res.isEmpty() ? DEFAULT_IPTYPE : static_cast<IpFamily>(res.get<uint32_t>());
}

void CellularManager::setupSignalChecking(int modemIndex) {
    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem.Signal", "Setup");
        const uint32_t signal_value = 5;
        method << signal_value;
        auto reply = modemProxy->callMethod(method);
        std::cout << "Successfully set up signal checking for modem index " << modemIndex << std::endl;
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to set up signal checking for modem index " << modemIndex << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }
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
        connectionStatus = static_cast<State>(state);
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
    try {
        std::string apn = getModemApn(modemIndex);
        uint32_t ipType = getModemIpType(modemIndex);
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.ModemManager1.Modem.Simple", "Connect");
        Properties properties = {
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
        return false;
    }

    return true;
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
    double rssi = 0;
    
    try {
        const std::string modem_path = "/org/freedesktop/ModemManager1/Modem/" + std::to_string(modemIndex);
        auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", modem_path);
        auto method = modemProxy->createMethodCall("org.freedesktop.DBus.Properties", "GetAll");
        method << "org.freedesktop.ModemManager1.Modem.Signal";
        auto reply = modemProxy->callMethod(method);
        Properties properties;
        reply >> properties;
        for (const auto& p : properties) {
            if (p.second.containsValueOfType<Properties>()) {
                auto type_propertires = p.second.get<Properties>();
                if (type_propertires.find("rssi") != type_propertires.end()) {
                    rssi = type_propertires.at("rssi");
                    break;
                }
            }
        }
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to getModemSignalStrength for modem index " << modemIndex << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }

    // Print the RSSI value
    std::cout << "Modem signal strength: " << rssi << " dBm" << std::endl;

    return rssi;
}

void CellularManager::assignIp(int modemIndex) {
    Properties ipConfig;
    std::string ipInterface;
    std::string ipAddress;
    std::string ipGateway;
    uint32_t ipMtu;
    std::vector<std::string> dnsArray;

    try {
        for (auto path : listBearers(modemIndex)) {
            auto modemProxy = sdbus::createProxy("org.freedesktop.ModemManager1", path);
            auto method = modemProxy->createMethodCall("org.freedesktop.DBus.Properties", "GetAll");
            method << "org.freedesktop.ModemManager1.Bearer";
            auto reply = modemProxy->callMethod(method);

            Properties properties;
            reply >> properties;

            ipInterface = properties["Interface"].get<std::string>();
            if (properties.find("Ip4Config") != properties.end() && !properties["Ip4Config"].isEmpty()) {
                ipConfig = properties["Ip4Config"].get<Properties>();
                break;
            } else if (properties.find("Ip6Config") != properties.end() && !properties["Ip6Config"].isEmpty()) {
                ipConfig = properties["Ip6Config"].get<Properties>();
                break;
            }
        }

        if (ipConfig.find("adress") != ipConfig.end() && !ipConfig.at("adress").isEmpty()) {
            ipAddress = ipConfig.at("adress").get<std::string>();
        }

        if (ipConfig.find("gateway") != ipConfig.end() && !ipConfig.at("gateway").isEmpty()) {
            ipAddress = ipConfig.at("gateway").get<std::string>();
        }

        if (ipConfig.find("dns1") != ipConfig.end() && !ipConfig.at("dns1").isEmpty()) {
            dnsArray.push_back(ipConfig.at("dns1"));
        }

        if (ipConfig.find("dns2") != ipConfig.end() && !ipConfig.at("dns2").isEmpty()) {
            dnsArray.push_back(ipConfig.at("dns2"));
        }

        if (ipConfig.find("dns3") != ipConfig.end() && !ipConfig.at("dns3").isEmpty()) {
            dnsArray.push_back(ipConfig.at("dns3"));
        }

        if (ipConfig.find("mtu") != ipConfig.end()) {
            ipMtu = ipConfig.at("mtu");
        }
    } catch (const sdbus::Error& e) {
        std::cerr << "Failed to get IP info for modem index " << modemIndex << std::endl;
        std::cerr << "D-Bus error: " << e.getMessage() << std::endl;
    }

    std::system(("ip link set " + ipInterface + " up").c_str());
    std::system(("ip addr add " + ipAddress + "/32 dev " + ipInterface).c_str());
    std::system(("ip link set dev " + ipInterface + " arp off").c_str());
    std::system(("ip link set dev " + ipInterface + " mtu " + std::to_string(ipMtu)).c_str());
    std::system(("ip route add default dev " + ipInterface + " metric 200").c_str());

    for (const auto& dns : dnsArray) {
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

