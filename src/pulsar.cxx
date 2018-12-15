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

#include <cstdlib>
#include <exception>
#include <iostream>

#include "pulsar.h"

namespace pulsar {

edge::edge(const size_type &buffer_size_in)
: buffer_size(buffer_size_in)
{
    buffer = static_cast<data_type *>(std::calloc(buffer_size_in, sizeof(data_type)));

    if (buffer == nullptr) {
        throw std::runtime_error("could not allocate buffer for edge");
    }
}

edge::~edge()
{
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
}

void edge::set_ready__l()
{
    ready = true;
}

void edge::clear_ready__l()
{
    ready = false;
}

const bool edge::is_ready__l()
{
    if (input == nullptr) {
        return true;
    }

    return ready;
}

void edge::set_input__l(pulsar::node * input_in)
{
    input = input_in;
}

void edge::set_output__l(pulsar::node * output_in)
{
    output = output_in;
}

data_type * edge::get_pointer__l()
{
    return buffer;
}

node::~node()
{

}

void node::set_input_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in)
{
    if (input_edges.count(name_in) != 0) {
        throw std::runtime_error("attempt to double connect to input " + name_in);
    }

    auto edge_lock = edge_in->get_lock();
    input_edges[name_in] = edge_in;
    edge_in->set_output__l(this);
}

void node::set_output_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in)
{
    if (output_edges.count(name_in) != 0) {
        throw std::runtime_error("attempt to double connect to output " + name_in);
    }

    auto edge_lock = edge_in->get_lock();
    output_edges[name_in] = edge_in;
    edge_in->set_input__l(this);
}

effect::~effect()
{

}

void effect::activate()
{
    auto lock = get_lock();
    return handle_activate__l();
}

void effect::run(const size_type &num_samples_in)
{
    auto lock = get_lock();

    for(auto input_name : get_inputs()) {
        if (input_edges.count(input_name) == 0) {
            throw std::runtime_error("attempt to run node with an unconnected input");
        }
    }

    for(auto output_name : get_outputs()) {
        if (output_edges.count(output_name) == 0) {
            throw std::runtime_error("attempt to run node with an unconnected output");
        }
    }

    for(auto edge : input_edges) {
        auto edge_lock = edge.second->get_lock();
        if (! edge.second->is_ready__l()) {
            throw std::runtime_error("attempt to run node with an edge that was not ready");
        }
    }

    handle_run__l(num_samples_in);

    for(auto edge : output_edges) {
        auto edge_lock = edge.second->get_lock();
        edge.second->set_ready__l();
    }
}

const pulsar::data_type effect::peek(const std::string &name_in)
{
    auto lock = get_lock();
    return handle_peek__l(name_in);
}

void effect::poke(const std::string &name_in, const pulsar::data_type &value_in)
{
    auto lock = get_lock();
    handle_poke__l(name_in, value_in);
}

const pulsar::data_type effect::knudge(const std::string &name_in, const pulsar::data_type &value_in)
{
    auto lock = get_lock();
    auto value = handle_peek__l(name_in);

    value += value_in;
    handle_poke__l(name_in, value);

    return value;
}

const pulsar::data_type effect::get_default(const std::string &name_in)
{
    auto lock = get_lock();
    return handle_get_default__l(name_in);
}

const std::vector<std::string> effect::get_inputs()
{
    auto lock = get_lock();
    return handle_get_inputs__l();
}

const std::vector<std::string> effect::get_outputs()
{
    auto lock = get_lock();
    return handle_get_outputs__l();
}

}
