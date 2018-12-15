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

#include <ladspa.h>
#include <memory>
#include <string>
#include <vector>

#include "pulsar.h"

namespace pulsar {

namespace ladspa {

    using data_type = LADSPA_Data;
    using descriptor_type = _LADSPA_Descriptor;
    using handle_type = LADSPA_Handle;
    using id_type = unsigned long;
    using size_type = unsigned long;

    class instance;

    class file : public std::enable_shared_from_this<file> {
        void *handle = nullptr;
        LADSPA_Descriptor_Function descriptor_fn = nullptr;
        std::map<std::string, const descriptor_type *> label_to_descriptor;

        public:
        const std::string path;
        const std::vector<const descriptor_type *> get_descriptors();
        const descriptor_type * get_descriptor(const std::string &name_in);
        const std::vector<std::string> get_labels();
        file(const std::string &path_in);
        virtual ~file();
        std::shared_ptr<instance> make_instance(const std::string &label_in, const ladspa::size_type &sample_rate_in);
    };

    class instance : public pulsar::effect, public std::enable_shared_from_this<instance> {
        handle_type handle = nullptr;
        const descriptor_type * descriptor = nullptr;
        std::vector<ladspa::data_type> control_buffers;
        const ladspa::id_type get_port_num(const std::string &name_in);
        virtual void handle_run__l(const pulsar::size_type &num_samples_in) override;
        virtual const pulsar::data_type handle_peek__l(const std::string &name_in) override;
        virtual void handle_poke__l(const std::string &name_in, const pulsar::data_type &value_in) override;

        public:
        const ladspa::size_type &sample_rate;
        const std::shared_ptr<ladspa::file> file;
        const std::string label;
        instance(std::shared_ptr<ladspa::file> file_in, const std::string &label_in, const ladspa::size_type &sample_rate_in);
        virtual ~instance();
        virtual void connect(const std::string &name_in, std::shared_ptr<pulsar::edge>) override;
    };

    }

}
