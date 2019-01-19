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
    vector<pulsar::node *> ready_nodes;

    cout << "main(): starting" << endl;

    auto domain = make_shared<pulsar::domain>(SAMPLE_RATE, BUFFER_SIZE);

    auto root = make_shared<pulsar::root>(domain);
    auto buffer = domain->make_buffer();
    root->set_output_buffer("output", buffer);

    auto plugin = make_shared<pulsar::ladspa::file>("/usr/lib/ladspa/delay_1898.so");
    auto instance = plugin->make_instance(domain, "delay_n");
    cout << "main(): done with variables" << endl;

    instance->activate();
    cout << "main(): done with activate()" << endl;

    instance->connect("Input", root->make_output_edge("output"));
    cout << "main(): done connecting" << endl;

    ready_nodes.push_back(root.get());

    while(ready_nodes.size() > 0) {
        auto ready = ready_nodes.back();
        ready_nodes.pop_back();
        cout << "running node from ready list" << endl;
        ready->run(BUFFER_SIZE);

        for (auto now_ready : ready->get_ready_children()) {
            cout << "something was ready" << endl;
            ready_nodes.push_back(now_ready);
        }
    }

    cout << "done" << endl;
}
