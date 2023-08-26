#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include <string>
#include <vector>
#include <memory> 
#include <functional>
#include <dbus-c++/dbus.h>

class CellularManager {
public:
    enum State { DISABLED = 0, SEARCHING, REGISTERED, CONNECTED, UNKNOWN };
    static const std::string DEFAULT_APN;
    static const std::string DEFAULT_IPTYPE;

    using UnsolicitedCallback = std::function<void(const std::string& message)>;
    static DBus::BusDispatcher dispatcher;

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
    bool isConnectionValidForCriticalData() const;
    void maintainConnection();
    void logIssue(const std::string& issue);
    State getState(int modemIndex);
    State getConnectionStatus(State connectionStatus);
    int getModemSignalStrength(int modemIndex);
    int getModemBER(const std::string& modemIdentifier) const;
    
    void registerUnsolicitedListener(const UnsolicitedCallback& callback);
    void unregisterUnsolicitedListener();
    
    void handleUnsolicitedIndication(const std::string& message);
    int getMinRSSILevel();
private:
    int minRSSILevel = -120; //basically means connect without min level
    UnsolicitedCallback unsolicitedCallback;
    State connectionStatus;
};

#endif
