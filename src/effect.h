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

#include <string>

#include "event.h"

namespace modpro {

struct effect {
    struct notification;

    using sample_type = float;
    using data_type = float;
    using size_type = unsigned long;

    protected:
    void notify(const std::string port_name_in, const effect::data_type value_in);

    public:
    const size_type effect_id;
    virtual const std::string get_name() = 0;
    virtual const std::string get_label() = 0;
    virtual data_type get_control(const std::string name_in) = 0;
    virtual void set_control(const std::string name_in, const sample_type data_type) = 0;
    virtual void connect(const std::string name_in, sample_type * buffer_in) = 0;
    virtual void disconnect(const std::string name_in) = 0;
    virtual void activate() = 0;
    virtual void run(size_type sample_count) = 0;
    effect();
};

}
