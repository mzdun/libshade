#pragma once
#include <unordered_map>
#include <functional>

namespace shade {
	struct bridge_info {
		std::string base;
		std::string username;
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
		bool bridge_located(const std::string& id, const std::string& base);
	};
}
