#include <shade/heartbeat.h>
#include <shade/listener.h>
#include <shade/storage.h>
#include <shade/io/connection.h>
#include <json.hpp>
#include "internal.h"
#include <algorithm>

using namespace std::literals;

namespace shade {
	heartbeat::heartbeat(cache* view, listener::manager* listener, io::network* net, const std::shared_ptr<model::bridge>& bridge)
		: view_{ view }
		, listener_{ listener }
		, net_{ net }
		, bridge_{ bridge }
	{
	}

	void heartbeat::stop()
	{
		timeout_.reset();
	}

	bool heartbeat::reconnect(json::value doc)
	{
		hue::errors error;
		if (get_error(error, doc)) {
			if (error == hue::errors::unauthorised_user) {
				auto listener = listener_->connection_listener(bridge_);
				if (listener)
					listener->onlost(bridge_);
				return true;
			}
		}
		return false;
	}

	void heartbeat::tick() {
		class storage_listener : public listener::storage {
			bool dirty_ = false;
			cache* parent_;
		public:
			storage_listener(cache* parent) : parent_{ parent } {}
			~storage_listener() {
				if (dirty_)
					shade::storage::store(*parent_);
			}

			void mark_dirty() { dirty_ = true; }
		};

		timeout_ = net_->timeout(1s, [=] { tick(); });

		auto self = shared_from_this();
		bridge_->logged(view_->browser()).get("/lights", io::make_client([self, this](int status, json::value doc) {
			std::unordered_map<std::string, hue::light> lights;
			if (unpack_json(lights, doc)) {
				bridge_->logged(view_->browser()).get("/groups", io::make_client([self, this, lights = std::move(lights)](int status, json::value doc) {
					std::unordered_map<std::string, hue::group> groups;
					if (unpack_json(groups, doc)) {
						storage_listener listener{ view_ };
						view_->bridge_lights(bridge_, std::move(lights), std::move(groups), &listener, listener_->bridge_listener(bridge_));
						return;
					}

					if (reconnect(doc))
						return;

					printf("GETTING GROUPS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
				}));
				return;
			}

			if (reconnect(doc))
				return;

			printf("GETTING LIGHTS: %d\n%s\n", status, doc.to_string(json::value::options::indented()).c_str());
		}));
	}
}
