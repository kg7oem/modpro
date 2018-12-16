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

class domain;
class effect;
class node;

struct edge : public std::enable_shared_from_this<edge> {
    pulsar::node * input_node;
    std::string input_name;
    pulsar::node * output_node;
    std::string output_name;
    edge(pulsar::node * output_node_in, const std::string &input_name_in);
    ~edge();
};

class node : protected modpro::util::lockable {
    friend pulsar::effect;

    std::map<std::string, std::shared_ptr<pulsar::edge>> input_edges;
    std::map<std::string, std::shared_ptr<pulsar::edge>> output_edges;

    protected:
    std::vector<std::shared_ptr<edge>> created_output_edges;
    std::shared_ptr<pulsar::domain> domain;
    void set_input_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in);
    void set_output_edge__l(const std::string &name_in, std::shared_ptr<pulsar::edge> edge_in);
    virtual data_type * get_input_buffer__l(const std::string &name_in) = 0;
    virtual void set_input_buffer__l(const std::string &name_in, data_type * buffer_in) = 0;
    virtual data_type * get_output_buffer__l(const std::string &name_in) = 0;
    virtual void set_output_buffer__l(const std::string &name_in, data_type * buffer_in) = 0;
    virtual bool is_ready__l() = 0;

    public:
    node(std::shared_ptr<pulsar::domain> domain_in);
    virtual ~node();
    virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) = 0;
    // virtual void disconnect(const std::string &name_in) = 0;
    bool is_ready();
    std::shared_ptr<edge> make_output_edge(const std::string &name_in);
    void set_input_buffer(const std::string &name_in, data_type * buffer_in);
    data_type * get_input_buffer(const std::string &name_in);
    void set_output_buffer(const std::string &name_in, data_type * buffer_in);
    data_type * get_output_buffer(const std::string &name_in);
};

class root : public pulsar::node, public std::enable_shared_from_this<root> {
    data_type * buffer;

    protected:
    virtual bool is_ready__l() override;
    virtual data_type * get_input_buffer__l(const std::string &name_in) override;
    virtual void set_input_buffer__l(const std::string &name_in, data_type * buffer_in) override;
    virtual data_type * get_output_buffer__l(const std::string &name_in) override;
    void set_output_buffer__l(const std::string &name_in, data_type * buffer_in);
    virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) override;

    public:
    root(std::shared_ptr<pulsar::domain> domain_in);
    virtual ~root();
};

class domain : public std::enable_shared_from_this<domain> {
    private:
    std::vector<edge> edges;

    public:
    const size_type sample_rate;
    const size_type buffer_size;
    domain(const size_type &sample_rate_in, const size_type &buffer_size_in);
    data_type * make_buffer();
};

class effect : public node {
    protected:
    bool ready = false;
    virtual void handle_run__l(const pulsar::size_type &num_samples_in) = 0;
    virtual const pulsar::data_type handle_peek__l(const std::string &name_in) = 0;
    virtual void handle_poke__l(const std::string &name_in, const pulsar::data_type &value_in) = 0;
    virtual const pulsar::data_type handle_get_default__l(const std::string &name_in) = 0;
    virtual const std::vector<std::string> get_inputs__l() = 0;
    virtual const std::vector<std::string> get_outputs__l() = 0;
    virtual void set_input_buffer__l(const std::string &name_in, data_type * buffer_in) = 0;
    virtual data_type * get_input_buffer__l(const std::string &name_in) = 0;
    virtual void set_output_buffer__l(const std::string &name_in, data_type * buffer_in) = 0;
    virtual data_type * get_output_buffer__l(const std::string &name_in) = 0;
    virtual void handle_activate__l() = 0;
    virtual bool is_ready__l() override;

    public:
    effect(std::shared_ptr<pulsar::domain> domain_in);
    virtual ~effect();
    virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) = 0;
    void activate();
    void run(const pulsar::size_type &num_samples_in);
    const pulsar::data_type peek(const std::string &name_in);
    void poke(const std::string &name_in, const pulsar::data_type &value_in);
    const pulsar::data_type knudge(const std::string &name_in, const pulsar::data_type &value_in);
    const pulsar::data_type get_default(const std::string &name_in);
    const std::vector<std::string> get_inputs();
    const std::vector<std::string> get_outputs();
};

}
