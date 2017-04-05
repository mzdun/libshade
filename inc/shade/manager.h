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

		manager(client* client, network* net, http* browser);

		bool ready() const { return discovery_.ready(); }
		void search();
		const bridge_info& bridge(const std::string& bridgeid) const;
		const std::unordered_map<std::string, bridge_info>& bridges() const { return storage_.bridges(); }

		bool is_selected(const std::string& id, const std::string& dev) const { return storage_.is_selected(id, dev); }
		void switch_selection(const std::string& id, const std::string& dev) { storage_.switch_selection(id, dev); }
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
		void get_lights(const connection& conn);
		void get_groups(const connection& conn, std::unordered_map<std::string, api::light> lights);
		void add_config(const std::string& bridgeid, std::unordered_map<std::string, api::light> lights, std::unordered_map<std::string, api::group> groups);
	};
}
