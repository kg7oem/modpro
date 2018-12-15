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

#include <iostream>
#include <list>

#include "pulsar.h"
#include "pulsar-ladspa.h"
// #include "audio.h"
// #include "event.h"

using namespace std;
// using namespace modpro;

// void handle_audio_started()
// {
//     cout << "Audio system has started" << endl;
// }

// void handle_audio_stopped(bool * should_run_p_in)
// {
//     *should_run_p_in = false;
//     cout << "Audio system has stopped" << endl;
// }

// void handle_audio_client_changed(shared_ptr<audio::processor> processor_in)
// {
//     processor_in->check_auto_connect();
// }

// void process_audio(const char * conf_path)
// {
//     bool should_run = true;

//     auto dbus_broker = dbus::make();
//     dbus_broker->start();

//     auto event_broker = make_shared<event::broker>();
//     auto processor = audio::processor::make(conf_path, event_broker, dbus_broker);

//     processor->start();

//     while(should_run) {
//         auto event = event_broker->get_event();

//         switch (event) {
//             case event::name::audio_started: handle_audio_started(); break;
//             case event::name::audio_stopped: handle_audio_stopped(&should_run); break;
//             case event::name::audio_processed: break;
//             case event::name::audio_client_change: handle_audio_client_changed(processor); break;
//         }
//     }
// }

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 512

int main(int argc, const char *argv[])
{
    auto plugin = make_shared<pulsar::ladspa::file>("/usr/lib/ladspa/delay_1898.so");

    for(auto i : plugin->get_descriptors()) {
        cout << "label: " << i->Label;
        cout << "; name: " << i->Name;
        cout << endl;
    }

    auto input_edge = make_shared<pulsar::edge>(BUFFER_SIZE);
    auto output_edge = make_shared<pulsar::edge>(BUFFER_SIZE);
    auto instance = plugin->make_instance("delay_n", SAMPLE_RATE);
    instance->connect("Input", input_edge);
    instance->connect("Output", output_edge);

    instance->activate();
    instance->run(BUFFER_SIZE);
}
