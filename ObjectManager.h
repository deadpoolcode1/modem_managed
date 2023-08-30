#include <sdbus-c++/sdbus-c++.h>
#include <iostream>

class ObjectManagerProxy final : public sdbus::ProxyInterfaces< sdbus::ObjectManager_proxy >
{
public:
    ObjectManagerProxy(sdbus::IConnection& connection, const std::string& destination, std::string path)
    : ProxyInterfaces(connection, destination, std::move(path))
    , m_connection(connection)
    , m_destination(destination)
    {
        registerProxy();
    }

    ~ObjectManagerProxy()
    {
        unregisterProxy();
    }

private:
    void onInterfacesAdded( const sdbus::ObjectPath& objectPath
            , const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfacesAndProperties) override
    {
        std::cout << objectPath << " added:\t";
        for (const auto& [interface, _] : interfacesAndProperties) {
            std::cout << interface << " ";
        }
        std::cout << std::endl;
    }

    void onInterfacesRemoved( const sdbus::ObjectPath& objectPath
            , const std::vector<std::string>& interfaces) override
    {
        std::cout << objectPath << " removed:\t";
        for (const auto& interface : interfaces) {
            std::cout << interface << " ";
        }
        std::cout << std::endl;
    }

    sdbus::IConnection& m_connection;
    std::string m_destination;
};