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

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dbus.h"
#include "effect.h"

namespace modpro {

static effect::size_type next_effect_id = 0;

effect::effect(const std::string dbus_path_in, std::shared_ptr<dbus> dbus_broker_in)
// FIXME this needs to be made atomic
: DBus::ObjectAdaptor(dbus_broker_in->connection, dbus_path_in), effect_id(next_effect_id++)
{

}

std::unique_lock<std::mutex> effect::get_lock()
{
    return std::unique_lock<std::mutex>(effect_mutex);
}

}
