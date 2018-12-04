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

#include <cmath>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>
#include <vector>

#include "effect.h"
#include "jackaudio.h"
#include "ladspa.h"

namespace modpro {

struct audio {
    using sample_type = float;
    using data_type = float;
    using size_type = unsigned long;

    class config {
        YAML::Node root;

        public:
        const std::string path;
        config(const std::string path_in);
        std::vector<std::string> get_plugins();
        YAML::Node get_chains();
        YAML::Node get_routes();
    };

    class processor : public modpro::jackaudio::handlers, public std::enable_shared_from_this<processor> {
        public:
        using effect_type = std::shared_ptr<modpro::effect>;
        using sample_type = modpro::audio::sample_type;
        using size_type = modpro::audio::size_type;

        private:
        // FIXME do these bools need to be atomic?
        audio::config config;
        std::shared_ptr<event::broker> broker;
        bool initialized = false;
        bool activated = false;
        std::map<const std::string, std::vector<std::string>> auto_connect;
        std::shared_ptr<modpro::jackaudio::client> jack;
        std::shared_ptr<modpro::jackaudio::audio_port> input;
        std::shared_ptr<modpro::jackaudio::audio_port> output;
        std::shared_ptr<modpro::ladspa> ladspa;
        std::vector<sample_type *> buffers;
        std::vector<effect_type> effects;
        std::map<std::string, std::shared_ptr<jackaudio::audio_port>> jack_connections;
        // FIXME this needs to be expanded to support N chains instead of 1
        std::map<std::string, audio::processor::effect_type> chain_effects;

        void init_jack();
        void init_dsp();
        void check_auto_connect();
        std::pair<const std::string, const std::string> parse_effect_port_string(const std::string string_in);

        public:
        processor(const std::string conf_path_in, std::shared_ptr<event::broker> broker_in);
        virtual ~processor() { }
        template<typename... Args>
        static std::shared_ptr<processor> make(Args... args)
        {
            auto new_processor = std::make_shared<processor>(args...);
            new_processor->init();
            return new_processor;
        }

        void init();
        void start();
        void set_auto_connect(const std::string source_in, const std::string dest_in);
        virtual void handle_client_register(const std::string client_name_in);
        virtual void handle_client_unregister(const std::string client_name_in);
        virtual void handle_shutdown();
        virtual void handle_process(modpro::jackaudio::nframes_type nframes);
        virtual void handle_sample_rate_change(modpro::jackaudio::nframes_type sample_rate_in);
        virtual void handle_buffer_size_change(modpro::jackaudio::nframes_type buffer_size_in);
        effect_type make_effect(const std::string name_in);
        sample_type * make_buffer();
    };

    static sample_type * make_buffer(const size_type size_in);

    class chain {
        std::map<std::string, audio::processor::effect_type> effects;

        public:
        void add_effect(const std::string name_in, audio::processor::effect_type effect_in);
    };

};

}
