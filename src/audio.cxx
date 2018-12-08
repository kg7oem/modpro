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
#include <functional>
#include <stdexcept>

#include "audio.h"

namespace modpro {

audio::config::config(const std::string path_in)
: path(path_in)
{
    root = YAML::LoadFile(path);
}

std::vector<std::string> audio::config::get_plugins()
{
    std::vector<std::string> retval;

    if (! root["plugins"]) {
        throw std::runtime_error("config file had no plugins section");
    } else if (! root["plugins"].IsSequence()) {
        throw std::runtime_error("config file plugins was not a list");
    }

    for(auto i : root["plugins"]) {
        retval.push_back(i.as<std::string>());
    }

    return retval;
}

YAML::Node audio::config::get_chains()
{
    if (! root["chains"]) {
        throw std::runtime_error("config file did not have chains section");
    }

    auto chains_node = root["chains"];

    if (! chains_node.IsMap()) {
        throw std::runtime_error("chains node in config file was not a map");
    }

    return chains_node;
}

YAML::Node audio::config::get_routes()
{
    if (! root["routes"]) {
        throw std::runtime_error("config file did not have a routes section");
    }

    return root["routes"];
}

audio::sample_type * audio::make_buffer(const audio::size_type size_in)
{
    assert(size_in > 0);
    auto new_buffer = calloc(sizeof(audio::sample_type), size_in);

    if (new_buffer == nullptr) {
        throw std::runtime_error("could not create new buffer because malloc failed");
    }

    return static_cast<audio::sample_type *>(new_buffer);
}

audio::processor::processor(const std::string conf_file_path_in, std::shared_ptr<event::broker> broker_in)
: config(conf_file_path_in), broker(broker_in)
{

}

void audio::processor::init()
{
    assert(! initialized);
    assert(! activated);

    init_jack();
    init_dsp();

    initialized = true;
}

// outside jack audio thread
void audio::processor::init_jack()
{
    std::cout << "Initializing JACK audio" << std::endl;

    jack = modpro::jackaudio::client::make("ModPro", this->shared_from_this());
    jack->open();

    for (auto i : config.get_routes()) {
        auto source = i[0].as<std::string>();
        auto dest = i[1].as<std::string>();
        std::cout << "  setting auto connect " << source << " -> " << dest << std::endl;
        set_auto_connect(source, dest);
    }

    std::cout << "Jack is initialized" << std::endl;
    std::cout << "  sample rate = " << jack->get_sample_rate() << std::endl;
    std::cout << "  max buffer size = " << jack->get_buffer_size() << std::endl;
    std::cout << std::endl;
}

void audio::processor::init_dsp()
{
    ladspa = modpro::ladspa::make();
    for (auto i : config.get_plugins()) {
        ladspa->open(i);
    }

    for (auto i : config.get_chains()) {
        auto chain_name = i.first.as<std::string>();
        auto chain_node = i.second;
        auto inputs_node = chain_node["inputs"];
        auto outputs_node = chain_node["outputs"];
        auto effects_node = chain_node["effects"];
        int port_num;

        if (chains.count(chain_name) != 0) {
            throw std::runtime_error("attempt to register duplicate chain name: " + chain_name);
        }

        std::cout << "Creating new chain: " << chain_name << std::endl;
        auto new_chain = std::make_shared<modpro::chain>(chain_name);
        chains[chain_name] = new_chain;

        for (auto j : effects_node) {
            auto effect_name = j["name"].as<std::string>();
            auto effect_type_name = j["type"].as<std::string>();

            std::cout << "  creating new effect: " << effect_name << " = " << effect_type_name << std::endl;
            auto effect = make_effect(effect_type_name);

            for (auto k : j["controls"]) {
                auto control_name = k.first.as<std::string>();
                auto control_value = k.second.as<audio::data_type>();
                std::cout << "    setting control: " << control_name << " = " << control_value << std::endl;
                effect->set_control(control_name, control_value);
            }

            new_chain->add_effect(effect_name, effect);
        }

        port_num = 0;
        for (auto k : chain_node["inputs"]) {
            port_num++;
            auto port_name = chain_name + "_in_" + std::to_string(port_num);
            std::cout << "  creating JACK input port: " << port_name << std::endl;
            auto new_jack_port = jack->add_audio_input(port_name);
            new_chain->add_route(k.as<std::string>(), new_jack_port);
        }

        port_num = 0;
        for (auto k : chain_node["outputs"]) {
            port_num++;
            auto port_name = chain_name + "_out_" + std::to_string(port_num);
            std::cout << "  creating JACK output port: " << port_name << std::endl;
            auto new_jack_port = jack->add_audio_output(port_name);
            new_chain->add_route(k.as<std::string>(), new_jack_port);
        }

        for (auto j : effects_node) {
            auto effect_name = j["name"].as<std::string>();
            auto src_effect = new_chain->get_effect(effect_name);

            for (auto k : j["wires"]) {
                auto buf = make_buffer();
                auto src_port_name = k.first.as<std::string>();

                src_effect->connect(src_port_name, buf);

                for (auto l : k.second) {
                    auto dest = parse_effect_port_string(l.as<std::string>());
                    std::cout << "  wiring " << effect_name << "." << src_port_name << " to " << dest.first << "." << dest.second << std::endl;
                    auto dest_effect = new_chain->get_effect(dest.first);
                    dest_effect->connect(dest.second, buf);
                }
            }
        }

        std::cout << std::endl;
    }

    std::cout << "DSP is initialized" << std::endl;
    std::cout << std::endl;
}

// outside of jack audio thread
void audio::processor::start()
{
    std::cout << "Starting audio processing" << std::endl;
    auto jack_lock = jack->get_lock();
    assert(initialized);
    assert(! activated);

    for (auto i : chains) {
        std::cout << "  Activating chain " << i.first << std::endl;
        i.second->activate();
    }

    activated = true;
    jack_lock.unlock();

    jack->activate();
    check_auto_connect();

    broker->send_event(event::name::audio_started);
}

void audio::processor::set_auto_connect(const std::string source_in, const std::string dest_in)
{
    auto_connect[source_in].push_back(dest_in);
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_shutdown()
{
    broker->send_event(event::name::audio_stopped);
}

void audio::processor::check_auto_connect()
{
    for(auto client_port : jack->get_known_port_names()) {
        if (auto_connect.count(client_port) > 0) {
            for (auto destination : auto_connect[client_port] ) {
                std::cout << "Auto connect: " << client_port << " -> " << destination << std::endl;
                auto result = jack->connect_port(client_port, destination);
                if (result != 0 && result != EEXIST) {
                    std::cout << "Error trying to connect ports: " << client_port << " -> " << destination << std::endl;
                }
            }
        }
    }
}

void audio::processor::handle_client_register(const std::string client_name_in)
{
    // broker->send_event(event::name::audio_client_change);
}

void audio::processor::handle_client_unregister(const std::string client_name_in)
{
    // broker->send_event(event::name::audio_client_change);
}

void audio::processor::handle_port_register(const uint32_t port_id_in)
{
    broker->send_event(event::name::audio_client_change);
}

void audio::processor::handle_port_unregister(const uint32_t port_id_in)
{
    // broker->send_event(event::name::audio_client_change);
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_process(modpro::jackaudio::nframes_type nframes)
{
    assert(initialized);
    assert(activated);

    for(auto chain_entry : chains) {
        auto chain = chain_entry.second;
        for(auto i : chain->get_routes()) {
            auto effect_port = parse_effect_port_string(i.first);
            auto effect = chain->get_effect(effect_port.first);
            effect->connect(effect_port.second, i.second->get_buffer(nframes));
        }

        chain->run(nframes);

        // JACK does not gurantee buffers wont change between calls to the
        // process handler
        for(auto i : chain->get_routes()) {
            auto effect_port = parse_effect_port_string(i.first);
            auto effect = chain->get_effect(effect_port.first);
            effect->disconnect(effect_port.second);
        }
    }

    broker->send_event(event::name::audio_processed);
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_sample_rate_change(modpro::jackaudio::nframes_type sample_rate_in)
{
    throw std::runtime_error("unable to handle sample rate change");
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_buffer_size_change(modpro::jackaudio::nframes_type buffer_size_in)
{
    throw std::runtime_error("unable to change maximum buffer size");
}

audio::processor::effect_type audio::processor::make_effect(const std::string name_in)
{
    auto new_effect = ladspa->instantiate(name_in, jack->get_sample_rate());
    return new_effect;
}

audio::sample_type * audio::processor::make_buffer()
{
    auto new_buffer = modpro::audio::make_buffer(jack->get_buffer_size());
    buffers.push_back(new_buffer);
    return new_buffer;
}

std::pair<const std::string, const std::string> audio::processor::parse_effect_port_string(const std::string string_in)
{
    auto dot_pos = string_in.find(".");

    if (dot_pos == std::string::npos) {
        throw std::runtime_error("unable to find string in " + string_in);
    }

    auto effect_name = string_in.substr(0, dot_pos);
    auto port_name = string_in.substr(dot_pos + 1, string_in.size());
    return std::make_pair(effect_name, port_name);
}

}
