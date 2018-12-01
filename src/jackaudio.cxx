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
#include <string.h>

#include "jackaudio.h"

namespace modpro {

jackaudio::client::~client()
{
    shutdown();
}

std::unique_lock<std::mutex> jackaudio::client::get_lock()
{
    return std::unique_lock<std::mutex>(jack_mutex);
}

static int wrap_nframes_cb(jack_nframes_t nframes, void * arg)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<int(jack_nframes_t)> *>(arg);
    auto cb = *p;
    cb(nframes);
    return 0;
}

static void wrap_void_cb(void * arg)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<void(void)> *>(arg);
    auto cb = *p;
    cb();
}

static void wrap_string_int_cb(const char * string_in, const int register_in, void * arg)
{
    // FIXME what is the syntax to cast/dereference this well?
    auto p = static_cast<std::function<void(const std::string, const int)> *>(arg);
    auto cb = *p;
    cb(string_in, register_in);
}

void jackaudio::client::open()
{
    client_p = jack_client_open(name.c_str(), options, 0);

    if (client_p == nullptr) {
        throw std::runtime_error("could not open jack server");
    }

    sample_rate = jack_get_sample_rate(client_p);
    buffer_size = jack_get_buffer_size(client_p);

    // FIXME leaks memory because the std::function never gets delete called
    if (jack_set_process_callback(
        client_p,
        wrap_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            auto lock = get_lock();
            handler->handle_process(nframes_in);
    }))))
    {
        throw std::runtime_error("could not set jack process callback");
    }

    // FIXME leaks memory because the std::function never gets delete called
    if(jack_set_sample_rate_callback(
        client_p,
        wrap_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            auto lock = get_lock();
            if (sample_rate != nframes_in) {
                sample_rate = nframes_in;
                handler->handle_sample_rate_change(nframes_in);
            }
    }))))
    {
        throw std::runtime_error("could not set jack sample rate callback");
    }

    // FIXME leaks memory because the std::function never gets delete called
    if(jack_set_buffer_size_callback(
        client_p,
        wrap_nframes_cb,
        static_cast<void *>(new std::function<void(jack_nframes_t)>([this](jack_nframes_t nframes_in) -> void {
            auto lock = get_lock();
            if (buffer_size != nframes_in) {
                buffer_size = nframes_in;
                this->handler->handle_buffer_size_change(nframes_in);
            }
    }))))
    {
        throw std::runtime_error("could not set jack buffer size callback");
    }

    // FIXME leaks memory because the std::function never gets delete called
    // does not have a return value
    jack_on_shutdown(
        client_p,
        wrap_void_cb,
        static_cast<void *>(new std::function<void(void)>([this]() -> void {
            auto lock = get_lock();
            this->handler->handle_shutdown();
    })));

    // FIXME leaks memory because the std::function never gets delete called
    if(jack_set_client_registration_callback(
        client_p,
        wrap_string_int_cb,
        static_cast<void *>(new std::function<void(const std::string client_name_in, const int register_in)>([this](const std::string client_name_in, const int register_in) -> void {
            auto lock = get_lock();
            if (register_in) {
                this->handler->handle_client_register(client_name_in);
            } else {
                this->handler->handle_client_unregister(client_name_in);
            }
    }))))
    {
        throw std::runtime_error("could not set jack buffer size callback");
    }
}

void jackaudio::client::shutdown()
{
    if (client_p != nullptr) {
        if (jack_deactivate(client_p)) {
            throw std::runtime_error("could not deactivate jack client");
        }

        if (jack_client_close(client_p)) {
            throw std::runtime_error("could not close jack client");
        }

        client_p = nullptr;
    }
}

void jackaudio::client::activate()
{
    assert(client_p != nullptr);

    if (jack_activate(client_p)) {
        throw std::runtime_error("could not activate jack client");
    }
}

jackaudio::nframes_type jackaudio::client::get_sample_rate()
{
    assert(client_p != nullptr);
    return sample_rate;
}

jackaudio::nframes_type jackaudio::client::get_buffer_size()
{
    assert(client_p != nullptr);
    return buffer_size;
}

jackaudio::port_type * jackaudio::client::register_port(const std::string name_in, const char * port_type_in, const unsigned long flags_in, unsigned long buffer_size_in = 0)
{
    assert(client_p != nullptr);

    auto new_jack_port = jack_port_register(client_p, name_in.c_str(), port_type_in, flags_in, buffer_size_in);

    if (new_jack_port == nullptr) {
        throw std::runtime_error("could not create jack audio input port");
    }

    return new_jack_port;
}

std::shared_ptr<jackaudio::audio_port> jackaudio::client::add_audio_input(const std::string name_in)
{
    auto new_port = register_port(name_in, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    return jackaudio::audio_port::make(this->shared_from_this(), new_port);
}

std::shared_ptr<jackaudio::audio_port> jackaudio::client::add_audio_output(const std::string name_in)
{
    auto new_port = register_port(name_in, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    return jackaudio::audio_port::make(this->shared_from_this(), new_port);
}

jackaudio::port::~port()
{
    assert(port_p != nullptr);

    if (jack_port_unregister(client->client_p, port_p)) {
        throw std::runtime_error("could not unregister jack port");
    }
}

jackaudio::audio_sample_type * jackaudio::audio_port::get_buffer(const nframes_type nframes_in)
{
    assert(port_p != nullptr);
    return static_cast<jackaudio::audio_sample_type *>(jack_port_get_buffer(port_p, nframes_in));
}

jackaudio::nframes_type jackaudio::audio_port::get_buffer_bytes(const nframes_type buffer_size_in)
{
    return sizeof(audio_sample_type) * buffer_size_in;
}

void jackaudio::audio_port::copy_into(jackaudio::audio_sample_type * dest_in, nframes_type nframes_in)
{
    memcpy(dest_in, this->get_buffer(nframes_in), this->get_buffer_bytes(nframes_in));
}
void jackaudio::audio_port::copy_into(std::shared_ptr<audio_port> dest_in, nframes_type nframes_in)
{
    copy_into(dest_in->get_buffer(nframes_in), nframes_in);
}

void jackaudio::audio_port::copy_from(jackaudio::audio_sample_type * source_in, nframes_type nframes_in)
{
    memcpy(this->get_buffer(nframes_in), source_in, this->get_buffer_bytes(nframes_in));
}
void jackaudio::audio_port::copy_from(std::shared_ptr<audio_port> source_in, nframes_type nframes_in)
{
    copy_from(source_in->get_buffer(nframes_in), nframes_in);
}

}
