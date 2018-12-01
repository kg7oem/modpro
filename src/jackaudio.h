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

#include <memory>
#include <mutex>

extern "C" {
#include <jack/jack.h>
}

namespace modpro {

struct jackaudio {
    using audio_sample_type = jack_default_audio_sample_t;
    using client_type = jack_client_t;
    using nframes_type = jack_nframes_t;
    using options_type = jack_options_t;
    using port_type = jack_port_t;

    class audio_port;
    class port;

    struct handlers {
        virtual void handle_shutdown() = 0;
        virtual void handle_process(nframes_type nframes_in) = 0;
        virtual void handle_client_register(const std::string client_name_in) = 0;
        virtual void handle_client_unregister(const std::string client_name_in) = 0;
        virtual void handle_sample_rate_change(nframes_type rate_in) = 0;
        virtual void handle_buffer_size_change(nframes_type buffer_size_in) = 0;
    };

    class client : public std::enable_shared_from_this<client> {
        friend port;

        std::mutex jack_mutex;
        std::shared_ptr<handlers> handler;
        jack_client_t * client_p = nullptr;

        void shutdown();
        port_type * register_port(const std::string name_in, const char * port_type_in, unsigned long flags_in, unsigned long buffer_size_in);

        public:
        nframes_type sample_rate = 0;
        nframes_type buffer_size = 0;
        const std::string name;
        const options_type options = JackNoStartServer;

        client(const std::string name_in, std::shared_ptr<handlers> handler_in) : handler(handler_in), name(name_in) { }
        virtual ~client();
        template<typename... Args>
        static std::shared_ptr<client> make(Args... args)
        {
            return std::make_shared<client>(args...);
        }
        std::unique_lock<std::mutex> get_lock();
        void open();
        void activate();
        nframes_type get_sample_rate();
        nframes_type get_buffer_size();
        std::shared_ptr<audio_port> add_audio_input(const std::string name_in);
        std::shared_ptr<audio_port> add_audio_output(const std::string name_in);
    };

    class port  {
        std::shared_ptr<jackaudio::client> client;

        protected:
        port(std::shared_ptr<jackaudio::client> client_in, port_type * port_p_in) : client(client_in), port_p(port_p_in) { }
        port_type * port_p = nullptr;

        public:
        ~port();
        virtual nframes_type get_buffer_bytes(const nframes_type buffer_size_in) = 0;
    };

    struct audio_port : public port, std::enable_shared_from_this<audio_port> {
        audio_port(std::shared_ptr<jackaudio::client> client_in, port_type * port_p_in)
        : port(client_in, port_p_in) { }
        template<typename... Args>
        static std::shared_ptr<audio_port> make(Args... args)
        {
            return std::make_shared<audio_port>(args...);
        }
        audio_sample_type * get_buffer(const nframes_type nframes_in);
        virtual nframes_type get_buffer_bytes(const nframes_type buffer_size_in) override;
        void copy_into(audio_sample_type * dest_in, nframes_type nframes_in);
        void copy_into(std::shared_ptr<audio_port> dest_in, nframes_type nframes_in);
        void copy_from(audio_sample_type * dest_in, nframes_type nframes_in);
        void copy_from(std::shared_ptr<audio_port> dest_in, nframes_type nframes_in);
    };
};

}
