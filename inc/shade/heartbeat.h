#pragma once

#include <shade/discovery.h>
#include <shade/cache.h>
#include <shade/io/http.h>

namespace shade {
	namespace hue {
		struct light;
		struct group;
	}

	namespace io {
		class connection;
	}

	namespace listener {
		struct bridge;
	}

	class heartbeat : public std::enable_shared_from_this<heartbeat> {
	public:
		heartbeat(cache* view, listener::manager* listener, io::network* net, const std::shared_ptr<model::bridge>& bridge);
		heartbeat() = delete;
		heartbeat(heartbeat&&) = delete;
		heartbeat(const heartbeat&) = delete;
		heartbeat& operator=(heartbeat&&) = delete;
		heartbeat& operator=(const heartbeat&) = delete;

		void stop();
		void start() { tick(); }
	private:
		cache* view_;
		listener::manager* listener_;
		io::network* net_;
		std::shared_ptr<model::bridge> bridge_;
		std::unique_ptr<io::timeout> timeout_;

		bool reconnect(json::value doc);

		void tick();
	};
}
