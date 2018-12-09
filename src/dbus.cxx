#include <cassert>
#include <iostream>

#include "dbus.h"

namespace modpro {

static DBus::BusDispatcher * init_dispatcher()
{
    auto new_dispatcher = new DBus::BusDispatcher();
    // this is annoying
    DBus::default_dispatcher = new_dispatcher;
    return new_dispatcher;
}

static auto dbus_dispatcher = init_dispatcher();

dbus::dbus() : bus_name(MODPRO_DBUS_NAME)
{

}

dbus::~dbus()
{
    if (dispatcher_thread != nullptr) {
        dbus_dispatcher->leave();
        std::cout << "joining with DBUS dispatcher thread" << std::endl;
        dispatcher_thread->join();
        std::cout << "done joining with dispatcher thread" << std::endl;
        delete dispatcher_thread;
        dispatcher_thread = nullptr;
    }
}

void dbus::init()
{
    connection.request_name(bus_name.c_str());
}

void dbus::start()
{
    assert(dispatcher_thread == nullptr);
    dispatcher_thread = new std::thread([]() -> void { dbus_dispatcher->enter(); });
}

}
