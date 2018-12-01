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

#include "event.h"

namespace modpro {

event::broker::lock_type event::broker::get_pending_events_lock()
{
    return lock_type(pending_events_mutex);
}

void event::broker::send_event(const event::name event_name_in) {
    auto lock = get_pending_events_lock();
    assert(pending_events.size() < 50);
    pending_events.push_back(event_name_in);
    pending_events_cv.notify_one();
}

const event::name event::broker::get_event()
{
    auto lock = get_pending_events_lock();
    pending_events_cv.wait(lock, [this]{ return pending_events.size() > 0; });

    auto retval = pending_events.front();
    pending_events.pop_front();
    return retval;
}

}
