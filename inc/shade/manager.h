#pragma once

#include "shade/discovery.h"
#include "shade/storage.h"

namespace shade {
	class manager {
	public:
		struct client {
			virtual ~client() = default;
			virtual void on_previous(std::unordered_map<std::string, bridge_info> const&) = 0;
			virtual void on_bridge(std::string const& bridgeid, bool known) = 0;
			virtual void on_connecting(std::string const& bridgeid) = 0;
			virtual void on_connected(std::string const& bridgeid) = 0;
		};

		manager(client* client, network* net);

		bool ready() const { return discovery_.ready(); }
		void search();
	private:
		client* client_;
		network* net_;
		storage storage_;
		discovery discovery_{ net_ };
	};
}
