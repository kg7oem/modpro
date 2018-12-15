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

#include <mutex>

namespace modpro {

struct util {

    class lockfactory {
        std::mutex mutex;

        public:
        std::unique_lock<std::mutex> get() {
            return std::unique_lock<std::mutex>(mutex);
        }
    };

    class lockable {
        util::lockfactory lockf;

        public:
        std::unique_lock<std::mutex> get_lock() {
            return lockf.get();
        }
    };

};

}
