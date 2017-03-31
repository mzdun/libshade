#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "shade/udp.h"
#include "shade/tcp.h"

namespace shade {
	struct bridge_info {
		std::string location;
		std::string host;
	};

	class discovery {
		bool conntected_ = false;
		std::unique_ptr<udp> udp_socket_;
		std::unique_ptr<tcp> tcp_socket_;
		std::unique_ptr<read_handler> current_search_;

		std::unordered_map<std::string, bridge_info> known_bridges_;

	public:
		using onbridge = std::function<void(const std::string& id, const std::string& base)>;
		discovery(std::unique_ptr<udp> udp, std::unique_ptr<tcp> tcp);
		discovery(const discovery&) = delete;
		discovery(discovery&&);
		discovery& operator=(const discovery&) = delete;
		discovery& operator=(discovery&&);
		~discovery();

		bool search(onbridge callback);

		bool seen(const std::string& id, const std::string& location);
	};
}