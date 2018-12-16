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

edge::edge(pulsar::node * output_node_in, const std::string &output_name_in)
: output_node(output_node_in), output_name(output_name_in)
{

}

edge::~edge()
{

}

node::node(std::shared_ptr<pulsar::domain> domain_in)
: domain(domain_in)
{

}

node::~node()
{

}

bool node::is_ready()
{
    auto lock = get_lock();
    return is_ready__l();
}

void node::set_input_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in)
{
    if (input_edges.count(name_in) != 0) {
        throw std::runtime_error("attempt to double connect to input " + name_in);
    }

    input_edges[name_in] = edge_in;
    edge_in->input_node = this;
    edge_in->input_name = name_in;
}

void node::set_output_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in)
{
    if (output_edges.count(name_in) != 0) {
        throw std::runtime_error("attempt to double connect to output " + name_in);
    }

    output_edges[name_in] = edge_in;
    edge_in->output_node = this;
    edge_in->output_name = name_in;
}

data_type * node::get_input_buffer(const std::string &name_in)
{
    auto lock = get_lock();
    return get_input_buffer(name_in);
}

void node::set_input_buffer(const std::string &name_in, data_type * buffer_in)
{
    auto lock = get_lock();
    set_input_buffer__l(name_in, buffer_in);
}

data_type * node::get_output_buffer(const std::string &name_in)
{
    auto lock = get_lock();
    return get_output_buffer__l(name_in);
}

void node::set_output_buffer(const std::string &name_in, data_type * buffer_in)
{
    auto lock = get_lock();
    return set_output_buffer__l(name_in, buffer_in);
}

std::shared_ptr<edge> node::make_output_edge(const std::string &name_in)
{
    auto new_edge = std::make_shared<edge>(this, name_in);
    created_output_edges.push_back(new_edge);
    return new_edge;
}

root::root(std::shared_ptr<pulsar::domain> domain_in)
: node(domain_in)
{

}

root::~root()
{

}

bool root::is_ready__l()
{
    return buffer != nullptr;
}

data_type * root::get_input_buffer__l(const std::string &name_in)
{
    throw std::runtime_error("root nodes do not have input buffers");
}

void root::set_input_buffer__l(const std::string &name_in, data_type * buffer_in)
{
    throw std::runtime_error("can not set input buffer for a root node");
}

void root::set_output_buffer__l(const std::string &name_in, data_type * buffer_in)
{
    buffer = buffer_in;

    for (auto i : created_output_edges) {
        i->input_node->set_input_buffer(i->input_name, buffer);
    }
}

data_type * root::get_output_buffer__l(const std::string &name_in)
{
    // if (name_in != "output") {
    //     throw std::runtime_error("root nodes only have 1 output named 'output'");
    // }

    return buffer;
}

void root::connect(const std::string &name_in, std::shared_ptr<pulsar::edge>)
{
    throw std::runtime_error("root nodes can not be connected to");
}

domain::domain(const size_type &sample_rate_in, const size_type &buffer_size_in)
: sample_rate(sample_rate_in), buffer_size(buffer_size_in)
{

}

effect::effect(std::shared_ptr<pulsar::domain> domain_in)
: node(domain_in)
{

}

effect::~effect()
{

}

data_type * domain::make_buffer()
{
    auto buffer = static_cast<data_type *>(calloc(buffer_size, sizeof(data_type)));
    if (buffer == nullptr) {
        throw std::runtime_error("could not allocate buffer");
    }
    return buffer;
}

void effect::activate()
{
    auto lock = get_lock();
    return handle_activate__l();
}

bool effect::is_ready__l()
{
    return ready;
}

void effect::run(const size_type &num_samples_in)
{
    auto lock = get_lock();

    for(auto input_name : get_inputs__l()) {
        if (input_edges.count(input_name) == 0) {
            throw std::runtime_error("attempt to run node with an unconnected input");
        }
    }

    // for(auto output_name : get_outputs__l()) {
    //     if (output_edges.count(output_name) == 0) {
    //         throw std::runtime_error("attempt to run node with an unconnected output");
    //     }
    // }

    for(auto edge : input_edges) {
        if (! edge.second->output_node->is_ready()) {
            throw std::runtime_error("attempt to run node with a parent that was not ready");
        }
    }

    handle_run__l(num_samples_in);
    ready = true;
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
    return get_inputs__l();
}

const std::vector<std::string> effect::get_outputs()
{
    auto lock = get_lock();
    return get_outputs__l();
}

}
