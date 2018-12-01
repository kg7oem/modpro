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

#include "effect.h"
#include "audio.h"

namespace modpro {
audio::sample_type * audio::make_buffer(const audio::size_type size_in)
{
    assert(size_in > 0);
    auto new_buffer = calloc(sizeof(audio::sample_type), size_in);

    if (new_buffer == nullptr) {
        throw std::runtime_error("could not create new buffer because malloc failed");
    }

    return static_cast<audio::sample_type *>(new_buffer);
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
    jack = modpro::jackaudio::client::make("ModPro", this->shared_from_this());
    jack->open();

    input = jack->add_audio_input("in_1");
    output = jack->add_audio_output("out_1");

    std::cout << "Jack is initialized" << std::endl;
    std::cout << "  sample rate = " << jack->get_sample_rate() << std::endl;
    std::cout << "  max buffer size = " << jack->get_buffer_size() << std::endl;
    std::cout << std::endl;
}

void audio::processor::init_dsp()
{
    ladspa = modpro::ladspa::make();
    ladspa->open("/usr/lib/ladspa/amp_1181.so");
    ladspa->open("/usr/lib/ladspa/ZamComp-ladspa.so");
    ladspa->open("/usr/lib/ladspa/ZamGate-ladspa.so");
    ladspa->open("/usr/lib/ladspa/delay_1898.so");

    gain_effect = make_effect("Simple amplifier");
    gain_effect->set_control("Amps gain (dB)", 0);

    delay_effect = make_effect("Simple delay line, linear interpolation");
    delay_effect->set_control("Delay Time (s)", .150);
    delay_effect->set_control("Max Delay (s)", .2);

    gate_effect = make_effect("ZamGate");
    gate_effect->set_control("Sidechain", 1);
    gate_effect->set_control("Threshold", -65);
    gate_effect->set_control("Attack", 25);
    gate_effect->set_control("Release", 25);
    gate_effect->set_control("Max gate close", -INFINITY);

    sample_type * buf_p;

    buf_p = make_buffer();
    gain_effect->connect("Output", buf_p);
    delay_effect->connect("Input", buf_p);
    gate_effect->connect("Sidechain Input", buf_p);

    buf_p = make_buffer();
    delay_effect->connect("Output", buf_p);
    gate_effect->connect("Audio Input 1", buf_p);

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

    for (auto i : effects) {
        std::cout << "  Activating effect " << i->get_name() << std::endl;
        i->activate();
    }

    std::cout << std::endl;

    std::cout << "  Activating JACK" << std::endl;
    std::cout << std::endl;

    activated = true;

    jack_lock.unlock();
    jack->activate();

    broker->send_event(event::name::audio_started);
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_shutdown()
{
    broker->send_event(event::name::audio_stopped);
}

void audio::processor::handle_client_register(const std::string client_name_in)
{
    broker->send_event(event::name::audio_client_change);
}

void audio::processor::handle_client_unregister(const std::string client_name_in)
{
    broker->send_event(event::name::audio_client_change);
}

// inside jack audio thread - jack is already locked
void audio::processor::handle_process(modpro::jackaudio::nframes_type nframes)
{
    assert(initialized);
    assert(activated);

    gain_effect->connect("Input", input->get_buffer(nframes));
    gate_effect->connect("Audio Output 1", output->get_buffer(nframes));

    for(auto i : effects) {
        i->run(nframes);
    }

    // JACK does not gurantee buffers wont change between calls to the
    // process handler
    gain_effect->disconnect("Input");
    gate_effect->disconnect("Audio Output 1");

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
    effects.push_back(new_effect);
    return new_effect;
}

audio::sample_type * audio::processor::make_buffer()
{
    auto new_buffer = modpro::audio::make_buffer(jack->get_buffer_size());
    buffers.push_back(new_buffer);
    return new_buffer;
}

}
