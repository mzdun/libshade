#pragma once

#include "shade/discovery.h"
#include "shade/storage.h"
#include "shade/io/http.h"

namespace json { struct value; }

namespace shade {
	namespace api {
		struct light;
		struct group;
	}
	namespace io {
		class connection;
	}

	class manager {
	public:
		struct listener {
			virtual ~listener() = default;
			virtual void on_previous(std::unordered_map<std::string, model::bridge> const&) = 0;
			virtual void on_bridge(std::string const& bridgeid) = 0;
			virtual void on_connecting(std::string const& bridgeid) = 0;
			virtual void on_connected(std::string const& bridgeid) = 0;
			virtual void on_connect_timeout(std::string const& bridgeid) = 0;
			virtual void on_refresh(std::string const& bridgeid) = 0;
		};

		manager(const std::string& name, listener* listener, io::network* net, io::http* browser);

		const auto& current_client() const { return storage_.current_client(); }
		bool ready() const { return discovery_.ready(); }
		void search();
		const model::bridge& bridge(const std::string& bridgeid) const;
		const std::unordered_map<std::string, model::bridge>& bridges() const { return storage_.bridges(); }
	private:
		listener* listener_;
		io::network* net_;
		io::http* browser_;
		storage storage_;
		discovery discovery_{ net_ };
		std::unordered_map<std::string, std::unique_ptr<io::timeout>> timeouts_;

		bool reconnect(json::value doc, const io::connection& conn);

		void get_config(const io::connection& conn);
		void create_user(const io::connection& conn);
		void create_user(const io::connection& conn, std::chrono::nanoseconds sofar);
		void get_user(const io::connection& conn, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then);
		void get_lights(const io::connection& conn);
		void get_groups(const io::connection& conn, std::unordered_map<std::string, api::light> lights);
		void add_config(const std::string& bridgeid, std::unordered_map<std::string, api::light> lights, std::unordered_map<std::string, api::group> groups);
	};
}
