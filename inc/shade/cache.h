#pragma once
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <shade/model/bridge.h>
#include <shade/io/http.h>
#include <shade/hue_data.h>
#include <shade/listener.h>

namespace shade {
	using bridges_t = std::unordered_map<std::string, std::shared_ptr<model::bridge>>;
	class cache {
		io::http* browser_;
		std::string clientid;
		bridges_t known_bridges;
	public:
		cache(const std::string& client, io::http* browser) : browser_{ browser }, clientid{ client } {}
		io::http* browser() const { return browser_; }
		const std::string& current_host() const { return clientid; }
		auto const& bridges() const { return known_bridges; }
		void bridges(const bridges_t& v) { known_bridges = v; }
		void bridges(bridges_t&& v) { known_bridges = std::move(v); }
		auto find(const std::string& id) const { return known_bridges.find(id); }
		auto begin() const { return known_bridges.begin(); }
		auto end() const { return known_bridges.end(); }
		std::shared_ptr<model::bridge> get(const std::string& id) const
		{
			auto it = find(id);
			if (it != end())
				return it->second;
			return {};
		}

		void bridge_located(const std::string& id, const std::string& base, listener::storage* storage);
		void bridge_named(const std::string& id, std::string name, std::string mac, std::string modelid, listener::storage* storage);
		void bridge_connected(const std::shared_ptr<model::bridge>& bridge, const std::string& username, listener::storage* storage);
		void bridge_lights(const std::shared_ptr<model::bridge>& bridge,
			std::unordered_map<std::string, hue::light> lights,
			std::unordered_map<std::string, hue::group> groups,
			listener::storage* storage);
	};
}
