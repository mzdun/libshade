#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "shade/network.h"

namespace shade {
	class discovery {
		bool connected_ = false;
		network* net_;
		std::unique_ptr<udp> udp_socket_;
		std::unique_ptr<read_handler> current_search_;

		struct bridge_info {
			std::string location;
			std::string base;
		};
		std::unordered_map<std::string, bridge_info> known_bridges_;

	public:
		using onbridge = std::function<void(const std::string& id, const std::string& base)>;
		discovery(network* net);
		discovery(const discovery&) = delete;
		discovery(discovery&&);
		discovery& operator=(const discovery&) = delete;
		discovery& operator=(discovery&&);
		~discovery();

		bool ready() const { return connected_; }
		bool search(onbridge callback);

		bool seen(const std::string& id, const std::string& location);
		bool base_known(const std::string& id, const std::string& base);
	};
}
