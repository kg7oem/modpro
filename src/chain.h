#include <map>
#include <memory>
#include <string>

#include "effect.h"
#include "jackaudio.h"

namespace modpro {

struct chain : public std::enable_shared_from_this<chain> {
    const std::string name;
    std::map<std::string, std::shared_ptr<effect>> effect_instances;
    std::vector<std::shared_ptr<effect>> run_list;
    std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> jack_connections;

    public:
    chain(const std::string name_in);
    void activate();
    void run(const effect::size_type sample_count_in);
    void add_effect(const std::string name_in, std::shared_ptr<effect> effect_in);
    std::shared_ptr<effect> get_effect(const std::string name_in);
    void add_route(std::string port_name_in, std::shared_ptr<jackaudio::audio_port> port_in);
    const std::vector<std::pair<std::string, std::shared_ptr<jackaudio::audio_port>>> get_routes();
};

}