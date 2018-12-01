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

#include "audio.h"
#include "event.h"

using namespace std;
using namespace modpro;

void handle_audio_started()
{
    cout << "Audio system has started" << endl;
}

void handle_audio_stopped(bool * should_run_p_in)
{
    *should_run_p_in = false;
    cout << "Audio system has stopped" << endl;
}

void process_audio()
{
    bool should_run = true;
    auto broker = make_shared<event::broker>();
    auto processor = audio::processor::make(broker);
    processor->start();

    while(should_run) {
        auto event = broker->get_event();

        switch (event) {
            case event::name::audio_started: handle_audio_started(); break;
            case event::name::audio_stopped: handle_audio_stopped(&should_run); break;
            case event::name::audio_processed: break;
            case event::name::audio_client_change: break;
        }
    }
}

int main(void)
{
    cout << "Starting" << endl;
    cout << endl;

    process_audio();

    cout << "Done" << endl;
    return 0;
}