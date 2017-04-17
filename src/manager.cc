#include <shade/manager.h>
#include <shade/listener.h>
#include <shade/heartbeat.h>
#include <shade/storage.h>
#include <shade/io/connection.h>
#include <json.hpp>
#include "internal.h"
#include <algorithm>

using namespace std::literals;

namespace shade {
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

	manager::manager(const std::string& name, listener::manager* listener, io::network* net, io::http* browser)
		: listener_{ listener }
		, net_{ net }
		, view_{ name, browser }
	{
		storage::load(view_);
	}

	void manager::search()
	{
		listener_->onload(view_);

		discovery_.search([this](std::string const& id, std::string const& base) {
			storage_listener listener{ &view_ };
			view_.bridge_located(id, base, &listener);
			auto bridge = view_.get(id);
			if (!bridge)
				return;

			get_config(bridge->unlogged(view_.browser()));
		}, [this] {
			// retry all unactivated cached bridges
			for (auto const& bridge : view_) {
				if (bridge.second->seen()) continue;
				get_config(bridge.second->unlogged(view_.browser()));
			}
		});
	}

	void manager::store_cache()
	{
		storage::store(view_);
	}

	void manager::get_config(const io::connection& conn)
	{
		conn.get("/config", io::make_client([=](int status, json::value doc) {
			hue::config cfg;
			if (unpack_json(cfg, doc)) {
				auto bridge = view_.get(conn.id());
				if (!bridge)
					return;
				storage_listener listener{ &view_ };
				view_.bridge_named(
					conn.id(),
					std::move(cfg.name),
					std::move(cfg.mac),
					std::move(cfg.modelid),
					&listener
				);
				listener_->onbridge(bridge);
			}
		}));
	}

	void manager::connect(const std::shared_ptr<model::bridge>& bridge)
	{
		connect(bridge, {});
	}

	std::string machine();
	std::string userdefinition(const std::string& application) {
		auto name = machine();
		if (name.empty())
			name = "unknown";

		return json::map{}.add("devicetype", application + "#" + name).to_string();
	}

	void manager::connect(const std::shared_ptr<model::bridge>& bridge, std::chrono::nanoseconds sofar)
	{
		auto listener = listener_->connection_listener(bridge);
		if (!sofar.count() && listener) {
			listener->onconnecting(bridge);
		}

		if (sofar >= 30s) {
			if (listener)
				listener->onfailed(bridge);
			return;
		}

		auto then = std::chrono::steady_clock::now();
		auto json = userdefinition(view_.current_host());
		bridge->unlogged(view_.browser()).post("", json, io::make_json_client([=](int status, json::value doc) {
			getuser(bridge, status, doc, sofar, then);
		}));
	}

	static inline json::value username(json::value doc)
	{
		json::value out;
		find_first(doc, [&](json::value child) {
			out = map(map(child, "success"), "username");
			return !out.is<json::NULLPTR>();
		});
		return out;
	}

	void manager::getuser(const std::shared_ptr<model::bridge>& bridge, int status, json::value doc, std::chrono::nanoseconds sofar, std::chrono::steady_clock::time_point then)
	{
		auto listener = listener_->connection_listener(bridge);

		if (status / 100 < 4) {
			auto value = username(doc);
			if (value.is<json::STRING>()) {
				auto username = value.as<json::STRING>();
				bridge->host().update(username);
				store_cache();
				if (listener)
					listener->onconnected(bridge);
				return;
			}
		}

		hue::errors error;
		if (get_error(error, doc)) {
			if (error == hue::errors::button_not_pressed) {
				auto& timeout = timeouts_[bridge->id()];
				timeout = net_->timeout(300ms, [=]() {
					timeouts_.erase(bridge->id());
					auto now = std::chrono::steady_clock::now();
					connect(bridge, sofar + (now - then));
				});

				if (timeout)
					return;
			}
		}

		if (listener)
			listener->onfailed(bridge);
	}

	class monitor : public heart_monitor {
		std::shared_ptr<heartbeat> beat_;
	public:
		monitor(std::shared_ptr<heartbeat> beat)
			: beat_{ std::move(beat) }
		{
		}

		~monitor()
		{
			beat_->stop();
		}
	};

	std::shared_ptr<heart_monitor> manager::defib(const std::shared_ptr<model::bridge>& bridge)
	{
		auto beat = std::make_shared<heartbeat>(&view_, listener_, net_, bridge);
		beat->start();
		return std::make_shared<monitor>(std::move(beat));
	}
}
