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

#include <ladspa.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "dbus.h"
#include "effect.h"

namespace modpro {

struct ladspa : public std::enable_shared_from_this<ladspa> {
    class port;
    class type;

    using data_type = LADSPA_Data;
    using id_type = unsigned long;
    using size_type = unsigned long;

    class file {
        friend ladspa;

        void *handle = nullptr;
        LADSPA_Descriptor_Function descriptor_fn = nullptr;
        std::map<id_type, ladspa::type *> types;
        file(const std::string path_in);

    public:
        const std::string path;
        // FIXME id_type should be const id_type
        const std::map<id_type, type *> get_types();
    };

    class instance : public modpro::effect {
        const std::string label;
        const LADSPA_Handle handle;
        ladspa::type * type;
        std::vector<data_type> control_buffers;
        std::map<const id_type, bool> port_is_connected;

    public:
        instance(const LADSPA_Handle handle_in, ladspa::type * type_in, const std::string dbus_prefix_in, std::shared_ptr<dbus> dbus_broker_in);
        std::vector<ladspa::port *> get_ports();
        std::vector<std::string> get_control_names();
        virtual const std::string get_name() override;
        virtual const std::string get_label() override;
        ladspa::port * get_port(const std::string port_name_in);
        ladspa::type * get_type();
        data_type get_control(const id_type id_in);
        data_type get_control(const std::string name_in);
        virtual double read(const std::string & name_i);
        std::map<std::string, double> read_all();
        virtual void write(const std::string & name_in, const double & value_in);
        virtual double knudge(const std::string & name_in, const double & value_in);
        void set_control(const id_type id_in, ladspa::data_type value_in);
        void set_control(const std::string name_in, ladspa::data_type value_in);
        void connect(const id_type portnum_in, data_type * buffer_in);
        void connect(const port * port_in, data_type * buffer_in);
        void connect(const std::string name_in, data_type * buffer_in);
        void disconnect(const id_type portnum_in);
        void disconnect(const port * port_in);
        void disconnect(const std::string name_in);
        void activate();
        void run(const size_type num_samples_in);
    };

    class type {
        friend instance;
        friend port;

        const LADSPA_Descriptor * descriptor;
        ladspa::file * file;
        std::vector<port *> ports;
        std::map<std::string, id_type> port_name_to_id;

    public:
        type(const LADSPA_Descriptor * descriptor_in, ladspa::file * file_in);
        id_type get_id();
        const ladspa::id_type get_port_count();
        const std::vector<port *> get_ports();
        const std::string get_name();
        port * get_port(const id_type number_in);
        port * get_port(const std::string port_name_in);
        std::shared_ptr<instance> instantiate(const size_type sample_rate_in, const std::string dbus_path_in, std::shared_ptr<dbus> dbus_broker_in);
    };

    struct port {
        const ladspa::id_type number;
        const ladspa::type * type;

        port(const id_type number_in, ladspa::type * type_in);
        const std::string get_name();
        const int get_descriptor();
        bool is_control();
        bool is_audio();
        bool is_input();
        bool is_output();
    };

    private:
    std::map<const std::string, file *> loaded_files;
    std::map<const id_type, type *> loaded_types;
    std::map<const std::string, const id_type> name_to_id;

    public:
    template<typename... Args>
    static std::shared_ptr<ladspa> make(Args... args)
    {
        return std::make_shared<ladspa>(args...);
    }

    file * open(const std::string path_in);
    type * get_type(const id_type id_in);
    std::shared_ptr<instance> instantiate(const id_type id_in, const size_type sample_rate_in, const std::string dbus_name_in, std::shared_ptr<dbus> dbus_broker_in);
    std::shared_ptr<instance> instantiate(const std::string name_in, const size_type sample_rate_in, const std::string dbus_name_in, std::shared_ptr<dbus> dbus_broker_in);
};

}
