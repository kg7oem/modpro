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

#include <map>
#include <memory>
#include <string>

#include "dbus.h"
#include "effect.h"
#include "jackaudio.h"

#define MODPRO_DBUS_CHAIN_PREFIX "/modpro/Chain"

namespace modpro {

struct chain : public hamradio::modpro::chain_adaptor, public DBus::IntrospectableAdaptor, public DBus::ObjectAdaptor, public std::enable_shared_from_this<chain> {
    const std::string name;
    std::map<std::string, std::shared_ptr<effect>> effect_instances;
    std::vector<std::shared_ptr<effect>> run_list;
    std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> jack_connections;
    std::shared_ptr<dbus> dbus_broker;

    public:
    chain(const std::string name_in, std::shared_ptr<dbus> dbus_broker_in);
    static const std::string make_dbus_path(const std::string name_in);
    void activate();
    void run(const effect::size_type sample_count_in);
    void add_effect(const std::string name_in, std::shared_ptr<effect> effect_in);
    std::shared_ptr<effect> get_effect(const std::string name_in);
    void add_route(std::string port_name_in, std::shared_ptr<jackaudio::audio_port> port_in);
    const std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> get_routes();
};

}
