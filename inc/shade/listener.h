#pragma once
#include <memory>

namespace shade {
	namespace model {
		class bridge;
		class light_source;
	}

	class cache;

	namespace listener {
		struct storage {
			virtual ~storage() = default;
			virtual void mark_dirty() = 0;
		};

		struct connection {
			virtual ~connection() = default;
			virtual void onconnecting(const std::shared_ptr<model::bridge>&) = 0;
			virtual void onconnected(const std::shared_ptr<model::bridge>&) = 0;
			virtual void onfailed(const std::shared_ptr<model::bridge>&) = 0;
			virtual void onlost(const std::shared_ptr<model::bridge>&) = 0;
		};

		struct bridge {
			virtual ~bridge() = default;
			virtual void update_start(const std::shared_ptr<model::bridge>&) = 0;
			virtual void source_added(const std::shared_ptr<model::light_source>&) = 0;
			virtual void source_removed(const std::shared_ptr<model::light_source>&) = 0;
			virtual void source_changed(const std::shared_ptr<model::light_source>&) = 0;
			virtual void update_end(const std::shared_ptr<model::bridge>&) = 0;
		};

		struct manager {
			virtual ~manager() = default;
			virtual void onload(const shade::cache&) = 0;
			virtual void onbridge(const std::shared_ptr<model::bridge>&) = 0;
			virtual connection* connection_listener(const std::shared_ptr<model::bridge>&) { return nullptr; }
			virtual bridge* bridge_listener(const std::shared_ptr<model::bridge>&) { return nullptr; }
		};
	}
}
