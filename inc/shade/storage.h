#pragma once
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <shade/model/bridge.h>

namespace shade {
	class storage {
		std::string clientid;
		std::unordered_map<std::string, model::bridge> known_bridges;
		static constexpr char confname[] = ".shade.cfg";
		static std::string build_filename();
		static auto& filename() {
			static auto name = build_filename();
			return name;
		}
	public:
		storage(const std::string& client) : clientid{ client } {}
		const std::string& current_client() const { return clientid; }
		void load();
		void store();
		auto const& bridges() const { return known_bridges; }
		auto find(const std::string& id) const { return known_bridges.find(id); }
		auto begin() const { return known_bridges.begin(); }
		auto end() const { return known_bridges.end(); }

		bool bridge_located(const std::string& id, const std::string& base);
		void bridge_named(const std::string& id, const std::string& name, const std::string& mac, const std::string& modelid);
		void bridge_connected(const std::string& id, const std::string& username);
		void bridge_config(const std::string& id,
			vector_shared<model::light> lights,
			vector_shared<model::group> groups);
	};
}
