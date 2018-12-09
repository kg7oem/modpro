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
