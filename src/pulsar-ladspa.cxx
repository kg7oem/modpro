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

#include <cmath>
#include <cstdlib>
#include <dlfcn.h>

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

std::shared_ptr<instance> file::make_instance(const std::string &label_in, const ladspa::size_type &sample_rate_in)
{
    return std::make_shared<instance>(this->shared_from_this(), label_in, sample_rate_in);
}

instance::instance(std::shared_ptr<ladspa::file> file_in, const std::string &label_in, const ladspa::size_type &sample_rate_in)
: sample_rate(sample_rate_in), file(file_in), label(label_in)
{
    descriptor = file->get_descriptor(label_in);
    handle = descriptor->instantiate(descriptor, sample_rate_in);

    if (handle == nullptr) {
        throw std::runtime_error("could not instantiate ladspa " + file->path + ": " + label);
    }

    control_buffers.reserve(descriptor->PortCount);

    for(id_type i = 0; i < descriptor->PortCount; i++) {
        auto port_descriptor = descriptor->PortDescriptors[i];
        if (LADSPA_IS_PORT_CONTROL(port_descriptor)) {
            control_buffers[i] = get_default(descriptor->PortNames[i]);
            descriptor->connect_port(handle, i, &control_buffers[i]);
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
    auto our_lock = get_lock();
    auto edge_lock = edge_in->get_lock();

    auto port_num = get_port_num(name_in);
    auto port_descriptor = descriptor->PortDescriptors[port_num];

    if (! LADSPA_IS_PORT_AUDIO(port_descriptor)) {
        throw std::runtime_error("attempt to connect to a non-audio ladspa port");
    }

    descriptor->connect_port(handle, port_num, edge_in->get_pointer__l());

    if (LADSPA_IS_PORT_INPUT(port_descriptor)) {
        set_input_edge__l(name_in, edge_in);
    } else if (LADSPA_IS_PORT_OUTPUT(port_descriptor)) {
        set_output_edge__l(name_in, edge_in);
    } else {
        throw std::runtime_error("ladspa port was not INPUT or OUTPUT");
    }
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

    throw std::logic_error("could not find hit for ladspa plugin " + file->path + " " + label);
}

} // namespace ladspa

} // namespace pulsar
