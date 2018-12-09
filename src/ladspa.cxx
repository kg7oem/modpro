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
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <utility>

#include "event.h"
#include "ladspa.h"

#define DESCRIPTOR_SYMBOL "ladspa_descriptor"

namespace modpro {

ladspa::file * ladspa::open(const std::string path_in)
{
    if (loaded_files.count(path_in)) {
        throw std::runtime_error("attempt to open file twice: " + path_in);
    }

    std::cout << "Loading plugin: " << path_in << std::endl;

    auto new_file = new ladspa::file(path_in);
    loaded_files[path_in] = new_file;

    for(auto i : new_file->get_types()) {
        auto imported_id = i.second->get_id();
        auto imported_name = i.second->get_name();

        if (loaded_types.count(imported_id) != 0) {
            throw std::runtime_error("attempt to register duplicated id: " + imported_id);
        }

        if (name_to_id.count(imported_name) != 0) {
            throw std::runtime_error("attempt to register duplicate name: " + imported_name);
        }

        loaded_types.insert(std::make_pair(imported_id, i.second));
        name_to_id.insert(std::make_pair(imported_name, imported_id));

        std::cout << "  " << i.second->get_id() << " " << i.second->get_name() << std::endl;
        for(auto j : i.second->get_ports()) {
            std::cout << "    Port # " << j->number << " " << j->get_name() << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;

    return new_file;
}

ladspa::type * ladspa::get_type(const id_type id_in)
{
    if (loaded_types.count(id_in) == 0) {
        throw std::runtime_error("could not get type for ladspa ID: " + std::to_string(id_in));
    }

    return loaded_types[id_in];
}

std::shared_ptr<ladspa::instance> ladspa::instantiate(const id_type id_in, const size_type sample_rate_in, const std::string dbus_name_in, std::shared_ptr<dbus> dbus_broker_in)
{
    return get_type(id_in)->instantiate(sample_rate_in, dbus_name_in, dbus_broker_in);
}

std::shared_ptr<ladspa::instance> ladspa::instantiate(const std::string name_in, const size_type sample_rate_in, const std::string dbus_path_in, std::shared_ptr<dbus> dbus_broker_in)
{
    if (name_to_id.count(name_in) == 0) {
        throw std::runtime_error("could not find plugin by name: " + name_in);
    }

    return instantiate(name_to_id[name_in], sample_rate_in, dbus_path_in, dbus_broker_in);
}

ladspa::file::file(const std::string path_in) : path(path_in)
{
    handle = dlopen(path_in.c_str(), RTLD_NOW);
    if (handle == NULL) {
        throw std::runtime_error("could not dlopen(" + path + ")");
    }

    descriptor_fn = (LADSPA_Descriptor_Function) dlsym(handle, DESCRIPTOR_SYMBOL);
    if (descriptor_fn == NULL) {
        throw std::runtime_error("could not get descriptor function for " + path);
    }

    const LADSPA_Descriptor * p;
    for(long i = 0; (p = descriptor_fn(i)) != NULL; i++) {
        auto new_type = new ladspa::type(p, this);
        types[new_type->get_id()] = new_type;
    }
}

const std::map<ladspa::id_type, ladspa::type *> ladspa::file::get_types()
{
    return types;
}

ladspa::type::type(const LADSPA_Descriptor * descriptor_in, ladspa::file * file_in)
: descriptor(descriptor_in), file(file_in)
{
    ladspa::id_type port_count = get_port_count();

    for(ladspa::id_type i = 0; i < port_count; i++) {
        auto new_port = new ladspa::port(i, this);
        ports.push_back(new_port);
        port_name_to_id[new_port->get_name()] = new_port->number;
    }
}

ladspa::id_type ladspa::type::get_id()
{
    return descriptor->UniqueID;
}

const std::string ladspa::type::get_name()
{
    return descriptor->Name;
}

const ladspa::id_type ladspa::type::get_port_count()
{
    return descriptor->PortCount;
}

const std::vector<ladspa::port *> ladspa::type::get_ports()
{
    return ports;
}

ladspa::port * ladspa::type::get_port(const id_type number_in)
{
    return ports[number_in];
}

ladspa::port * ladspa::type::get_port(const std::string port_name_in)
{
    return get_port(port_name_to_id[port_name_in]);
}

std::shared_ptr<ladspa::instance> ladspa::type::instantiate(const ladspa::size_type sample_rate_in, const std::string dbus_name_in, std::shared_ptr<dbus> dbus_broker_in)
{
    auto new_handle = descriptor->instantiate(descriptor, sample_rate_in);
    return std::make_shared<ladspa::instance>(new_handle, this, dbus_name_in, dbus_broker_in);
}

ladspa::port::port(const id_type number_in, ladspa::type * type_in)
: number(number_in), type(type_in)
{

}

const std::string ladspa::port::get_name()
{
    return type->descriptor->PortNames[number];
}

const int ladspa::port::get_descriptor()
{
    return type->descriptor->PortDescriptors[number];
}

bool ladspa::port::is_control()
{
    return LADSPA_IS_PORT_CONTROL(get_descriptor());
}

bool ladspa::port::is_audio()
{
    return LADSPA_IS_PORT_AUDIO(get_descriptor());
}

bool ladspa::port::is_input()
{
    return LADSPA_IS_PORT_INPUT(get_descriptor());
}

bool ladspa::port::is_output()
{
    return LADSPA_IS_PORT_OUTPUT(get_descriptor());
}

ladspa::instance::instance(const LADSPA_Handle handle_in, ladspa::type * type_in, const std::string dbus_path_in, std::shared_ptr<dbus> dbus_broker_in)
: effect(dbus_path_in, dbus_broker_in), handle(handle_in), type(type_in)
{
    control_buffers = std::vector<data_type>(type->get_port_count());

    for (auto i : type->get_ports()) {
        if (i->is_control()) {
            connect(i->number, &control_buffers[i->number]);
        } else if (i->is_audio()) {
            disconnect(i->number);
        } else {
            throw std::runtime_error("port was not audio or control");
        }
    }
}

ladspa::data_type ladspa::instance::get_control(const ladspa::id_type id_in)
{
    auto lock = get_lock();

    assert(type->get_port(id_in)->is_control());

    return control_buffers[id_in];
}

ladspa::data_type ladspa::instance::get_control(const std::string name_in)
{
    if (type->port_name_to_id.count(name_in) == 0) {
        throw std::runtime_error("unknown port name: " + name_in);
    }

    return get_control(type->port_name_to_id[name_in]);
}

double ladspa::instance::read(const std::string & name_in)
{
    if (type->port_name_to_id.count(name_in) == 0) {
        throw DBus::Error("hamradio.modpro.errors.ControlNameUnknown", "unknown control name");
    }

    std::cout << "get control request: " << name_in << std::endl;
    return get_control(name_in);
}

std::map<std::string, double> ladspa::instance::read_all()
{
    std::map<std::string, double> retval;

    for (auto i : get_ports()) {
        if (! i->is_control()) {
            continue;
        }

        retval[i->get_name()] = get_control(i->get_name());
    }

    return retval;
}

void ladspa::instance::write(const std::string & name_in, const double & value_in)
{
    if (type->port_name_to_id.count(name_in) == 0) {
        throw DBus::Error("hamradio.modpro.errors.ControlNameUnknown", "unknown control name");
    }

    std::cout << "set control request: " << name_in << " = " << value_in << std::endl;
    return set_control(name_in, value_in);
}

double ladspa::instance::knudge(const std::string & name_in, const double & value_in)
{
    if (type->port_name_to_id.count(name_in) == 0) {
        throw DBus::Error("hamradio.modpro.errors.ControlNameUnknown", "unknown control name");
    }

    auto new_value = get_control(name_in) + value_in;
    set_control(name_in, new_value);
    return new_value;
}

void ladspa::instance::set_control(const ladspa::id_type id_in, ladspa::data_type value_in)
{
    auto lock = get_lock();

    assert(type->get_port(id_in)->is_control());
    assert(type->get_port(id_in)->is_input());

    control_buffers[id_in] = value_in;
}

void ladspa::instance::set_control(const std::string name_in, ladspa::data_type value_in)
{
    set_control(type->port_name_to_id[name_in], value_in);
}

const std::string ladspa::instance::get_name()
{
    return type->get_name();
}

const std::string ladspa::instance::get_label()
{
    return label;
}

ladspa::type * ladspa::instance::get_type()
{
    return type;
}

std::vector<ladspa::port *> ladspa::instance::get_ports()
{
    return type->get_ports();
}

std::vector<std::string> ladspa::instance::get_control_names()
{
    auto ports = get_ports();
    std::vector<std::string> retval(ports.size());

    for(auto i : ports) {
        if (! i->is_control()) {
            continue;
        }

        retval.push_back(i->get_name());
    }

    return retval;
}

ladspa::port * ladspa::instance::get_port(const std::string port_name_in)
{
    return type->get_port(port_name_in);
}

void ladspa::instance::connect(const ladspa::id_type portnum_in, ladspa::data_type * buffer_in)
{
    type->descriptor->connect_port(handle, portnum_in, buffer_in);
    port_is_connected[portnum_in] = true;
}

void ladspa::instance::connect(const ladspa::port * port_in, ladspa::data_type * buffer_in)
{
    connect(port_in->number, buffer_in);
}

void ladspa::instance::connect(const std::string name_in, data_type * buffer_in)
{
    if (type->port_name_to_id.count(name_in) == 0) {
        throw std::runtime_error("there is no known port named " + name_in);
    }

    connect(type->port_name_to_id[name_in], buffer_in);
}

void ladspa::instance::disconnect(const ladspa::id_type portnum_in)
{
    port_is_connected[portnum_in] = false;
    return connect(portnum_in, nullptr);
}

void ladspa::instance::disconnect(const ladspa::port * port_in)
{
    disconnect(port_in->number);
}

void ladspa::instance::disconnect(const std::string name_in)
{
    disconnect(type->port_name_to_id[name_in]);
}

void ladspa::instance::activate()
{
    for( auto i : type->get_ports()) {
        // FIXME this isn't working?
        if (! port_is_connected[i->number]) {
            throw std::runtime_error("port is not connected to anything; #" + i->number);
        }
    }

    if (type->descriptor->activate) {
        type->descriptor->activate(handle);
    }
}

void ladspa::instance::run(ladspa::size_type num_samples_in)
{
    auto lock = get_lock();
    type->descriptor->run(handle, num_samples_in);
}

}
