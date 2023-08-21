#ifndef CELLULAR_MANAGER_H
#define CELLULAR_MANAGER_H

#include </usr/include/dbus-c++-1/dbus-c++/dbus.h>
#include <string>
#include <vector>
#include <memory> 
#include <functional>

using UnsolicitedCallback = std::function<void(const std::string& message)>;

class CellularManager {
public:
    CellularManager();
    ~CellularManager();

    std::vector<std::string> getAvailableModems() const;
    bool connectModem(const std::string& modemIdentifier, const std::string& apn = "", const std::string& username = "", const std::string& password = "");
    void disconnectModem(const std::string& modemIdentifier);
    bool isConnectionValidForCriticalData() const;
    void maintainConnection();
    void logIssue(const std::string& issue);
    void registerUnsolicitedListener(const UnsolicitedCallback& callback);
    void unregisterUnsolicitedListener();

private:
    DBus::Connection conn;
    UnsolicitedCallback unsolicitedCallback;
    int getModemSignalStrength(const std::string& modemIdentifier) const;
    int getModemBER(const std::string& modemIdentifier) const;
    void handleUnsolicitedIndication(const std::string& message);
};

#endif
