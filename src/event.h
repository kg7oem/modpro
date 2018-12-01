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

#include <condition_variable>
#include <list>
#include <mutex>

namespace modpro {

struct event {
    enum name {
        audio_started, audio_stopped, audio_processed, audio_client_change
    };

    struct broker {
        using mutex_type = std::mutex;
        using lock_type = std::unique_lock<mutex_type>;

        private:
        mutex_type pending_events_mutex;
        std::condition_variable pending_events_cv;
        std::list<event::name> pending_events;

        lock_type get_pending_events_lock();

        public:
        const event::name get_event();
        void send_event(const event::name event_name_in);
    };
};

}
