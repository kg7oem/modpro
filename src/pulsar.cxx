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

}

