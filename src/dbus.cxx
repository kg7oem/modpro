// Copyright (C) 2018  Tyler Riddle <cardboardaardvark@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
