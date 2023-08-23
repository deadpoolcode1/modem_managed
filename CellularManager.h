#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include <string>
#include <vector>
#include <memory> 
#include <functional>
#include <dbus-c++/dbus.h>

class CellularManager {
public:
    using UnsolicitedCallback = std::function<void(const std::string& message)>;

    static DBus::BusDispatcher dispatcher;

    CellularManager();
    ~CellularManager();
    
    std::vector<std::string> getAvailableModems() const;
    bool connectModem(const std::string& modemIdentifier, const std::string& apn, const std::string& username, const std::string& password);
    void disconnectModem(const std::string& modemIdentifier);
    
    bool isConnectionValidForCriticalData() const;
    void maintainConnection();
    void logIssue(const std::string& issue);
    
    int getModemSignalStrength(const std::string& modemIdentifier) const;
    int getModemBER(const std::string& modemIdentifier) const;
    
    void registerUnsolicitedListener(const UnsolicitedCallback& callback);
    void unregisterUnsolicitedListener();
    
    void handleUnsolicitedIndication(const std::string& message);

private:
    DBus::Connection conn;
    UnsolicitedCallback unsolicitedCallback;
};

#endif
