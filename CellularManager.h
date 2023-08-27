#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include <string>
#include <vector>
#include <memory> 
#include <functional>

class CellularManager {
public:
    enum State { DISABLED = 0, SEARCHING, REGISTERED, CONNECTED, UNKNOWN };
    static const std::string DEFAULT_APN;
    static const std::string DEFAULT_IPTYPE;

    using UnsolicitedCallback = std::function<void(const std::string& message)>;

    CellularManager();
    ~CellularManager();

    void parseCommandLine(int argc, char *argv[]);
    void setupSignalChecking(int modemIndex);
    std::string getModemApn(int modemIndex);
    std::string getModemIpType(int modemIndex);
    std::vector<int> getAvailableModems();
    bool connectModem(int modemIndex);
    void disconnectModem(const std::string& modemIdentifier);
    void enableModem(int modemIndex);
    State getState(int modemIndex);
    int getModemSignalStrength(int modemIndex);
    void resetHw();
    void assignIp(int modemIndex);
    std::string executeCommand(const std::string& cmd); 
    int getMinRSSILevel();
    int getMaxConnectTime();
private:
    int minRSSILevel = -120; //basically means connect without min level
    int maxConnectTime = 300;
    UnsolicitedCallback unsolicitedCallback;
    State connectionStatus;
};

#endif
