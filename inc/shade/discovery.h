#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "shade/io/network.h"

namespace shade {
	class discovery {
		bool connected_ = false;
		io::network* net_;
		std::unique_ptr<io::udp> udp_socket_;
		std::unique_ptr<io::read_handler> current_search_;

		struct bridge_info {
			std::string location;
			std::string base;
		};
		std::unordered_map<std::string, bridge_info> known_bridges_;

	public:
		using onbridge = std::function<void(const std::string& id, const std::string& base)>;
		using ondone = std::function<void()>;
		discovery(io::network* net);
		discovery(const discovery&) = delete;
		discovery(discovery&&);
		discovery& operator=(const discovery&) = delete;
		discovery& operator=(discovery&&);
		~discovery();

		bool ready() const { return connected_; }
		bool search(onbridge callback, ondone done);

		bool seen(const std::string& id, const std::string& location);
		bool base_known(const std::string& id, const std::string& base);
	};
}
