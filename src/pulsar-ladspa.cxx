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
#include <cmath>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>

#include "pulsar-ladspa.h"

#define DESCRIPTOR_SYMBOL "ladspa_descriptor"

namespace pulsar {

namespace ladspa {

file::file(const std::string &path_in)
: path(path_in)
{
    handle = dlopen(path_in.c_str(), RTLD_NOW);
    if (handle == nullptr) {
        throw std::runtime_error("could not dlopen(" + path + ")");
    }

    descriptor_fn = (LADSPA_Descriptor_Function) dlsym(handle, DESCRIPTOR_SYMBOL);
    if (descriptor_fn == nullptr) {
        throw std::runtime_error("could not get descriptor function for " + path);
    }

    for (auto i : get_descriptors()) {
        label_to_descriptor[i->Label] = i;
    }
}

file::~file()
{
    if (handle != nullptr) {
        if (dlclose(handle)) {
            throw std::runtime_error("could not dlclose() ladspa plugin handle");
        }

        handle = nullptr;
        descriptor_fn = nullptr;
    }
}

const descriptor_type * file::get_descriptor(const std::string &label_in)
{
    if (label_to_descriptor.count(label_in) == 0) {
        throw std::runtime_error("unknown label " + label_in + " in file " + path);
    }

    return label_to_descriptor[label_in];
}

const std::vector<const pulsar::ladspa::descriptor_type *> file::get_descriptors()
{
    std::vector<const descriptor_type *> descriptors;
    const LADSPA_Descriptor * p;

    for(long i = 0; (p = descriptor_fn(i)) != nullptr; i++) {
        descriptors.push_back(p);
    }

    return descriptors;
}

const std::vector<std::string> file::get_labels()
{
    std::vector<std::string> labels(label_to_descriptor.size());

    for(auto i : label_to_descriptor) {
        labels.push_back(i.first);
    }

    return labels;
}

std::shared_ptr<instance> file::make_instance(std::shared_ptr<domain> domain_in, const std::string &label_in)
{
    return std::make_shared<instance>(domain_in, this->shared_from_this(), label_in);
}

instance::instance(std::shared_ptr<pulsar::domain> domain_in, std::shared_ptr<ladspa::file> file_in, const std::string &label_in)
: pulsar::effect(domain_in), sample_rate(domain->sample_rate), file(file_in), label(label_in)
{
    descriptor = file->get_descriptor(label_in);
    handle = descriptor->instantiate(descriptor, sample_rate);

    if (handle == nullptr) {
        throw std::runtime_error("could not instantiate ladspa " + file->path + ": " + label);
    }

    control_buffers.reserve(descriptor->PortCount);
    output_buffers.assign(descriptor->PortCount, nullptr);

    for(id_type i = 0; i < descriptor->PortCount; i++) {
        auto port_descriptor = descriptor->PortDescriptors[i];
        if (LADSPA_IS_PORT_CONTROL(port_descriptor)) {
            control_buffers[i] = get_default(descriptor->PortNames[i]);
            descriptor->connect_port(handle, i, &control_buffers[i]);
        } else if (LADSPA_IS_PORT_AUDIO(port_descriptor)) {
            data_type * p;

            if (LADSPA_IS_PORT_INPUT(port_descriptor)) {
                p = nullptr;
            } else {
                p = output_buffers[i] = static_cast<data_type *>(std::calloc(domain->buffer_size, sizeof(data_type)));
                if (p == nullptr) {
                    throw std::runtime_error("could not calloc()");
                }
            }

            descriptor->connect_port(handle, i, p);
        }
    }
}

instance::~instance()
{
    if (handle != nullptr) {
        if (descriptor->deactivate != nullptr) {
            descriptor->deactivate(handle);
        }

        descriptor->cleanup(handle);
    }

    for (auto p : output_buffers) {
        if (p != nullptr) {
            free(p);
        }
    }
}

void instance::handle_activate__l()
{
    if (descriptor->activate != nullptr) {
        descriptor->activate(handle);
    }
}

// this method does not require locking
const id_type instance::get_port_num(const std::string &name_in)
{
    for(id_type i = 0; i < descriptor->PortCount; i++) {
        if (name_in == descriptor->PortNames[i]) {
            return i;
        }
    }

    throw std::runtime_error("could not find port named " + name_in);
}

void instance::connect(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in)
{
    std::cout << "in connect()" << std::endl;

    auto our_lock = get_lock();
    std::cout << "got our_lock" << std::endl;

    // auto edge_lock = edge_in->get_lock();
    // std::cout << "got edge_lock" << std::endl;

    auto port_num = get_port_num(name_in);
    auto port_descriptor = descriptor->PortDescriptors[port_num];
    std::cout << "past auto port_descriptor" << std::endl;

    if (! LADSPA_IS_PORT_AUDIO(port_descriptor)) {
        throw std::runtime_error("attempt to connect to a non-audio ladspa port");
    }

    if (LADSPA_IS_PORT_INPUT(port_descriptor)) {
        assert(edge_in->output_node != nullptr);

        auto output_buffer = edge_in->output_node->get_output_buffer(name_in);
        descriptor->connect_port(handle, port_num, output_buffer);
        set_input_edge__l(name_in, edge_in);
    } else if (LADSPA_IS_PORT_OUTPUT(port_descriptor)) {
        throw std::runtime_error("use make_output_edge() instead of connect() with an audio output");
    } else {
        throw std::runtime_error("ladspa port was not INPUT or OUTPUT");
    }

    std::cout << "done with instance::connect()" << std::endl;
}

void instance::handle_run__l(const pulsar::size_type &num_samples_in)
{
    descriptor->run(handle, num_samples_in);
}

const pulsar::data_type instance::handle_peek__l(const std::string &name_in)
{
    auto port_num = get_port_num(name_in);
    return control_buffers[port_num];
}

void instance::handle_poke__l(const std::string &name_in, const pulsar::data_type &value_in)
{
    auto port_num = get_port_num(name_in);
    auto port_descriptor = get_port_num(name_in);

    if (! LADSPA_IS_PORT_CONTROL(port_descriptor)) {
        throw std::runtime_error("attempt to poke a ladspa port that was not a control port");
    } else if (! LADSPA_IS_PORT_INPUT(port_descriptor)) {
        throw std::runtime_error("attempt to poke an output port");
    }

    control_buffers[port_num] = value_in;
}

const pulsar::data_type instance::handle_get_default__l(const std::string &name_in)
{
    auto port_num = get_port_num(name_in);
    auto port_hints = descriptor->PortRangeHints[port_num];
    auto hint_descriptor = port_hints.HintDescriptor;

    if (! LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {
        return 0;
    } else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
        return 0;
    } else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {
        return 1;
    } else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {
        return 100;
    } else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
        return 440;
    } else if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
        return port_hints.LowerBound;
    } else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.75 + log(port_hints.UpperBound) * 0.25);
        } else {
            return port_hints.LowerBound * 0.75 + port_hints.UpperBound * 0.25;
        }
    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.5 + log(port_hints.UpperBound) * 0.5);
        } else {
            return (port_hints.LowerBound * 0.5 + port_hints.UpperBound * 0.5);
        }
    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            return exp(log(port_hints.LowerBound) * 0.25 + log(port_hints.UpperBound) * 0.75);
        } else {
            return port_hints.LowerBound * 0.25 + port_hints.UpperBound * 0.75;
        }
    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
        return port_hints.UpperBound;
    }

    throw std::logic_error("could not find hint for ladspa plugin " + file->path + " " + label);
}

const std::vector<std::string> instance::get_inputs__l()
{
    std::vector<std::string> names;

    for(id_type i = 0; i < descriptor->PortCount; i++) {
        auto port_descriptor = descriptor->PortDescriptors[i];
        if (LADSPA_IS_PORT_AUDIO(port_descriptor) && LADSPA_IS_PORT_INPUT(port_descriptor)) {
            names.push_back(descriptor->PortNames[i]);
        }
    }

    return names;
}

const std::vector<std::string> instance::get_outputs__l()
{
    std::vector<std::string> names;

    for(id_type i = 0; i < descriptor->PortCount; i++) {
        auto port_descriptor = descriptor->PortDescriptors[i];
        if (LADSPA_IS_PORT_AUDIO(port_descriptor) && LADSPA_IS_PORT_OUTPUT(port_descriptor)) {
            names.push_back(descriptor->PortNames[i]);
        }
    }

    return names;
}

data_type * instance::get_output_buffer__l(const std::string &name_in)
{
    auto port_num = get_port_num(name_in);
    auto buffer = output_buffers[port_num];

    if (buffer == nullptr) {
        throw std::runtime_error("no buffer was available for port name " + name_in);
    }

    return buffer;
}

} // namespace ladspa

} // namespace pulsar
