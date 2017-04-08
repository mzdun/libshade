#pragma once

#include "shade/discovery.h"
#include "shade/storage.h"
#include "shade/http.h"

namespace json { struct value; }

namespace shade {
	namespace api {
		struct light;
		struct group;
	}
	class connection;

	class manager {
	public:
		struct client {
			virtual ~client() = default;
			virtual void on_previous(std::unordered_map<std::string, bridge_info> const&) = 0;
			virtual void on_bridge(std::string const& bridgeid, bool known) = 0;
			virtual void on_bridge_named(std::string const& bridgeid) = 0;
			virtual void on_connecting(std::string const& bridgeid) = 0;
			virtual void on_connected(std::string const& bridgeid) = 0;
			virtual void on_connect_timeout(std::string const& bridgeid) = 0;
			virtual void on_refresh(std::string const& bridgeid) = 0;
		};

		manager(const std::string& name, client* client, network* net, http* browser);

		const auto& current_client() const { return storage_.current_client(); }
		bool ready() const { return discovery_.ready(); }
		void search();
		const bridge_info& bridge(const std::string& bridgeid) const;
		const std::unordered_map<std::string, bridge_info>& bridges() const { return storage_.bridges(); }
	private:
		client* client_;
		network* net_;
		http* browser_;
		storage storage_;
		discovery discovery_{ net_ };
		std::unordered_map<std::string, std::unique_ptr<timeout>> timeouts_;

		bool reconnect(json::value doc, const connection& conn);

		void get_config(const connection& conn);
		void create_user(const connection& conn);
		void create_user(const connection& conn, std::chrono::nanoseconds sofar);
		void get_user(const connection& conn, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then);
		void get_lights(const connection& conn);
		void get_groups(const connection& conn, std::unordered_map<std::string, api::light> lights);
		void add_config(const std::string& bridgeid, std::unordered_map<std::string, api::light> lights, std::unordered_map<std::string, api::group> groups);
	};
}
