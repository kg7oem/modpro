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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "util.h"

namespace pulsar {

using data_type = float;
using size_type = unsigned long;

class node;

class edge : public modpro::util::lockable, public std::enable_shared_from_this<edge> {
    const size_type buffer_size;
    pulsar::node * input;
    pulsar::node * output;
    bool ready = false;
    data_type * buffer = nullptr;

    public:
    edge(const size_type &buffer_size_in);
    ~edge();
    void set_input__l(pulsar::node * input_in);
    pulsar::node * get_input__l();
    void set_output__l(pulsar::node * output_in);
    pulsar::node * get_output__l();
    bool is_ready__l();
    void set_ready__l();
    void clear_ready__l();
    data_type * get_pointer__l();
};

class node : protected modpro::util::lockable {
    std::map<std::string, std::shared_ptr<pulsar::edge>> input_edges;
    std::map<std::string, std::shared_ptr<pulsar::edge>> output_edges;

    protected:
    void set_input_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in);
    void set_output_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in);
    void update_node__l();
    bool is_ready__l();

    public:
    virtual ~node();
    virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) = 0;
    // virtual void disconnect(const std::string &name_in) = 0;
};

class effect : public node {
    protected:
    virtual void handle_run__l(const pulsar::size_type &num_samples_in) = 0;
    virtual const pulsar::data_type handle_peek__l(const std::string &name_in) = 0;
    virtual void handle_poke__l(const std::string &name_in, const pulsar::data_type &value_in) = 0;

    public:
    virtual ~effect();
    virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) = 0;
    void run(const pulsar::size_type &num_samples_in);
    const pulsar::data_type peek(const std::string &name_in);
    void poke(const std::string &name_in, const pulsar::data_type &value_in);
    const pulsar::data_type knudge(const std::string &name_in, const pulsar::data_type &value_in);
};

}
