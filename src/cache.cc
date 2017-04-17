#include <shade/cache.h>
#include "model/json.h"
#include <algorithm>
#include <cstdio>
#include <memory>

namespace json {
	JSON_STATIC_DECL(shade::model::bridge);
}

namespace shade {
	void cache::bridge_located(const std::string& id, const std::string& base, listener::storage* storage) {
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			auto bridge = std::make_shared<model::bridge>(id, browser_);
			bridge->set_host(clientid);
			bridge->set_base(std::move(base));
			known_bridges[id] = std::move(bridge);
			storage->mark_dirty();
		} else {
			if (it->second->hw().base != base) {
				it->second->set_base(std::move(base));
				storage->mark_dirty();
			}
		}
	}

	void cache::bridge_named(const std::string& id,
		std::string name, std::string mac,
		std::string modelid, listener::storage* storage)
	{
		auto it = known_bridges.find(id);
		if (it == known_bridges.end()) {
			auto bridge = std::make_shared<model::bridge>(id, browser_);
			bridge->set_host(clientid);
			bridge->seen(std::move(name), std::move(mac), std::move(modelid));
			known_bridges[id] = std::move(bridge);
			storage->mark_dirty();
		} else {
			auto& bridge = it->second;
			bridge->seen(true);
			if (bridge->hw().name != name
				|| bridge->hw().mac != mac
				|| bridge->hw().modelid != modelid) {
				bridge->seen(std::move(name), std::move(mac), std::move(modelid));
				storage->mark_dirty();
			}
		}
	}

	void cache::bridge_connected(const std::shared_ptr<model::bridge>& bridge, const std::string& username, listener::storage* storage)
	{
		if (bridge->host().update(username))
			storage->mark_dirty();
	}

	void cache::bridge_lights(const std::shared_ptr<model::bridge>& bridge,
		std::unordered_map<std::string, hue::light> lights,
		std::unordered_map<std::string, hue::group> groups,
		listener::storage* storage, listener::bridge* changes)
	{
		if (bridge->bridge_lights(std::move(lights), std::move(groups), changes))
			storage->mark_dirty();
	}

}
