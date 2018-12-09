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

#include <exception>

#include "chain.h"

namespace modpro {

chain::chain(const std::string name_in, std::shared_ptr<dbus> dbus_broker_in)
:  DBus::ObjectAdaptor(dbus_broker_in->connection, make_dbus_path(name_in)), name(name_in)
{

}

const std::string chain::make_dbus_path(const std::string name_in)
{
    auto buf = std::string(MODPRO_DBUS_CHAIN_PREFIX);
    buf += "/";
    buf += name_in;
    return buf;
}

void chain::activate()
{
    for(auto i : effect_instances) {
        i.second->activate();
    }
}

void chain::run(const effect::size_type sample_count_in)
{
    for(auto i : run_list) {
        i->run(sample_count_in);
    }
}

void chain::add_effect(const std::string name_in, std::shared_ptr<effect> effect_in)
{
    if (effect_instances.count(name_in) != 0) {
        throw std::runtime_error("attempt to add duplicate effect name: " + name_in);
    }

    effect_instances[name_in] = effect_in;
    run_list.push_back(effect_in);
}

std::shared_ptr<effect> chain::get_effect(const std::string name_in)
{
    if (effect_instances.count(name_in) == 0) {
        throw std::runtime_error("effect name is not known: " + name_in);
    }

    return effect_instances[name_in];
}

void chain::add_route(std::string port_name_in, std::shared_ptr<jackaudio::audio_port> port_in)
{
    jack_connections.push_back(std::make_pair(port_name_in, port_in));
}

const std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> chain::get_routes()
{
    return jack_connections;
}

}
