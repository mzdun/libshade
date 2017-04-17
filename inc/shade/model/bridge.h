#pragma once
#include <shade/model/host.h>
#include <shade/model/light.h>
#include <shade/model/group.h>
#include <shade/io/connection.h>
#include <shade/io/http.h>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace json {
	struct struct_translator;
	struct value;
}

namespace shade { namespace hue {
	struct light;
	struct group;
} }

namespace shade { namespace listener {
	struct bridge;
} }

namespace shade { namespace io {
	struct http;
} }

namespace shade { namespace model {
	struct hw_info {
		std::string base;
		std::string name;
		std::string mac;
		std::string modelid;
	};

#define MEM_EQ(name) (lhs.name == rhs.name)

	inline bool operator == (hw_info const& lhs, hw_info const& rhs) {
		return MEM_EQ(base)
			&& MEM_EQ(name)
			&& MEM_EQ(mac)
			&& MEM_EQ(modelid);
	}

	inline bool operator != (hw_info const& lhs, hw_info const& rhs) {
		return !(lhs == rhs);
	}

#undef MEM_EQ

	class bridge : public std::enable_shared_from_this<bridge> {
		bool seen_ = false;
		std::string id_;
		hw_info hw_;
		vector_shared<light> lights_;
		vector_shared<group> groups_;
		std::shared_ptr<model::host> current_;
		vector_shared<model::host> hosts_;
		io::http* browser_ = nullptr;

	public:
		bridge() = default;
		bridge(const std::string& id, io::http* browser);
		bridge(const bridge&) = delete;
		bridge& operator=(const bridge&) = delete;
		bridge(bridge&&) = default;
		bridge& operator=(bridge&&) = delete;
		void from_storage(const std::string& id, io::http* browser);

		static void prepare(json::struct_translator&);

		void set_host(const std::string&);
		model::host& host() const { return *current_; }

		std::string id() const { return id_; }
		bool seen() const { return seen_; }
		void seen(bool v) { seen_ = v; }
		const hw_info& hw() const { return hw_; }
		void hw(hw_info v) { hw_ = std::move(v); }
		void set_base(std::string base) { hw_.base = std::move(base); }
		const vector_shared<light>& lights() const { return lights_; }
		void lights(vector_shared<light> v) { lights_ = std::move(v); }
		const vector_shared<group>& groups() const { return groups_; }
		void groups(vector_shared<group> v) { groups_ = std::move(v); }

		io::connection logged(io::http* browser) const { return unlogged(browser).logged(host().username()); }
		io::connection unlogged(io::http* browser) const { return { browser, hw_.base, id_ }; }

		bool bridge_lights(
			std::unordered_map<std::string, hue::light> lights,
			std::unordered_map<std::string, hue::group> groups,
			listener::bridge* listener);

		void seen(std::string name, std::string mac, std::string modelid)
		{
			seen_ = true;
			hw_.name = std::move(name);
			hw_.mac = std::move(mac);
			hw_.modelid = std::move(modelid);
		}

	private:
		void connect(std::chrono::nanoseconds sofar);
		void getuser(int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then);

		bool update_lights(std::unordered_map<std::string, hue::light> lights, listener::bridge* listener);
		bool update_groups(std::unordered_map<std::string, hue::group> groups, listener::bridge* listener);
	};
} }
