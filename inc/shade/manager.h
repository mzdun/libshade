#pragma once

#include "shade/discovery.h"
#include "shade/cache.h"
#include "shade/io/http.h"

namespace json { struct value; }

namespace shade {
	namespace hue {
		struct light;
		struct group;
	}

	namespace io {
		class connection;
	}

	namespace listener {
		struct manager;
	}

	class manager {
	public:
		manager(const std::string& name, listener::manager* listener, io::network* net, io::http* browser);

		const auto& current_host() const { return view_.current_host(); }
		bool ready() const { return discovery_.ready(); }
		const cache& view() const { return view_; }
		void store_cache();
		void search();
		void connect(const std::shared_ptr<model::bridge>&);
	private:
		listener::manager* listener_;
		io::network* net_;
		cache view_;
		discovery discovery_{ net_ };
		std::unordered_map<std::string, std::unique_ptr<io::timeout>> timeouts_;

		bool reconnect(json::value doc, const std::shared_ptr<model::bridge>& bridge);

		void get_config(const io::connection& conn);

		void connect(const std::shared_ptr<model::bridge>&, std::chrono::nanoseconds sofar);
		void getuser(const std::shared_ptr<model::bridge>& bridge, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then);

		void get_lights(const std::shared_ptr<model::bridge>& bridge);
		void get_groups(const std::shared_ptr<model::bridge>& bridge, std::unordered_map<std::string, hue::light> lights);
	};
}
