#pragma once
#include <array>
#include <functional>
#include <unordered_map>
#include <vector>

namespace shade {
	struct light_state {
		bool on;
		int bri;
		int hue;
		int sat;
		int ct;
		std::array<double, 2> xy;
		std::string mode;
	};

	struct group_state : light_state {
		bool any;
	};

	struct info_base {
		std::string id;
		std::string name;
		std::string type; // Group type or Light modelid
	};

	struct light_info : info_base {
		light_state state;
	};

	struct group_info : info_base {
		std::string klass;
		group_state state;
		std::vector<std::string> lights;
	};

#define MEM_EQ(name) (lhs.name == rhs.name)
#define BASE_EQ(base) (static_cast<const base&>(lhs) == rhs)

	inline bool operator == (light_state const& lhs, light_state const& rhs) {
		return MEM_EQ(on)
			&& MEM_EQ(bri)
			&& MEM_EQ(hue)
			&& MEM_EQ(sat)
			&& MEM_EQ(ct)
			&& MEM_EQ(xy[0])
			&& MEM_EQ(xy[1])
			&& MEM_EQ(mode);
	}

	inline bool operator != (light_state const& lhs, light_state const& rhs) {
		return !(lhs == rhs);
	}

	inline bool operator == (group_state const& lhs, group_state const& rhs) {
		return MEM_EQ(any)
			&& BASE_EQ(light_state);
	}

	inline bool operator != (group_state const& lhs, group_state const& rhs) {
		return !(lhs == rhs);
	}

	inline bool operator == (info_base const& lhs, info_base const& rhs) {
		return MEM_EQ(id)
			&& MEM_EQ(name)
			&& MEM_EQ(type);
	}

	inline bool operator != (info_base const& lhs, info_base const& rhs) {
		return !(lhs == rhs);
	}

	inline bool operator == (light_info const& lhs, light_info const& rhs) {
		return MEM_EQ(state)
			&& BASE_EQ(info_base);
	}

	inline bool operator != (light_info const& lhs, light_info const& rhs) {
		return !(lhs == rhs);
	}

	inline bool operator == (group_info const& lhs, group_info const& rhs) {
		return MEM_EQ(state)
			&& MEM_EQ(klass)
			&& MEM_EQ(lights)
			&& BASE_EQ(info_base);
	}

	inline bool operator != (group_info const& lhs, group_info const& rhs) {
		return !(lhs == rhs);
	}

#undef BASE_EQ
#undef MEM_EQ

	struct bridge_info {
		bool seen = false;
		std::string base;
		std::string username;
		std::string name;
		std::string mac;
		std::string modelid;
		std::vector<light_info> lights;
		std::vector<group_info> groups;
	};

	class store_at_exit;
	class storage {
		friend class store_at_exit;
		std::unordered_map<std::string, bridge_info> known_bridges;
		static constexpr char confname[] = ".shade.cfg";
		static std::string build_filename();
		static auto& filename() {
			static auto name = build_filename();
			return name;
		}
		void store();
	public:
		void load();
		auto const& bridges() const { return known_bridges; }
		auto find(const std::string& id) const { return known_bridges.find(id); }
		auto begin() const { return known_bridges.begin(); }
		auto end() const { return known_bridges.end(); }
		bool bridge_located(const std::string& id, const std::string& base);
		void bridge_named(const std::string& id, const std::string& name, const std::string& mac, const std::string& modelid);
		void bridge_connected(const std::string& id, const std::string& username);
		void bridge_config(const std::string& id,
			std::vector<light_info> lights,
			std::vector<group_info> groups);
	};
}
