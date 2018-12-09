#pragma once

#include <dbus-c++/dbus.h>
#include <memory>
#include <thread>

#include "dbus-adaptor.h"

#define MODPRO_DBUS_NAME "hamradio.modpro"

namespace modpro {

struct dbus : public std::enable_shared_from_this<dbus> {
    const std::string bus_name;

    private:
    std::thread * dispatcher_thread = nullptr;

    public:
    DBus::Connection connection = DBus::Connection::SessionBus();
    dbus();
    ~dbus();
    template<typename... Args>
    static std::shared_ptr<dbus> make(Args... args)
    {
        auto new_dbus = std::make_shared<dbus>(args...);
        new_dbus->init();
        return new_dbus;
    }
    void init();
    void start();
    void stop();
};

}
