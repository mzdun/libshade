#include "shade/manager.h"
#include "json.hpp"

using namespace std::literals;

namespace shade { namespace api {
	struct config {
		std::string name;
		std::string mac;
		std::string modelid;
	};

	struct state_type {
		bool all_on;
		bool any_on;
	};

	struct action_type {
		bool on;
		int bri;
		int hue;
		int sat;
		int ct;
		std::array<double, 2> xy;
		std::string colormode;
	};

	struct group {
		std::string name;
		std::string type;
		std::string klass;
		std::vector<std::string> lights;
		state_type state;
		action_type action;
	};

	struct light {
		std::string name;
		std::string modelid;
		std::string uniqueid;
		action_type state;
	};

	struct error_type {
		int type;
		std::string address;
		std::string description;
	};

	enum class errors {
		unauthorised_user = 1,
		invalid_json = 2,
		resource_404 = 3,
		method_unsupported = 4,
		missing_param = 5,
		parameter_404 = 6,
		invalid_value = 7,
		param_read_only = 8,
		too_many_items = 11,
		portal_required = 12,
		button_not_pressed = 101,
		internal_error = 901
	};
} }

namespace json {
	JSON_STRUCT(shade::api::config) {
		JSON_PROP(name);
		JSON_PROP(mac);
		JSON_PROP(modelid);
	}

	JSON_STRUCT(shade::api::state_type) {
		JSON_PROP(all_on);
		JSON_PROP(any_on);
	};

	JSON_STRUCT(shade::api::action_type) {
		JSON_PROP(on);
		JSON_PROP(bri);
		JSON_PROP(hue);
		JSON_PROP(sat);
		JSON_PROP(ct);
		JSON_PROP(xy);
		JSON_PROP(colormode);
	};

	JSON_STRUCT(shade::api::group) {
		JSON_PROP(name);
		JSON_PROP(type);
		JSON_OPT_NAMED_PROP("class", klass);
		JSON_PROP(lights);
		JSON_PROP(state);
		JSON_PROP(action);
	};

	JSON_STRUCT(shade::api::light) {
		JSON_PROP(name);
		JSON_PROP(modelid);
		JSON_PROP(uniqueid);
		JSON_PROP(state);
	};

	JSON_STRUCT(shade::api::error_type) {
		JSON_PROP(type);
		JSON_PROP(address);
		JSON_PROP(description);
	};
}

namespace shade {

	template <typename Handler>
	class http_client : public http::client {
		Handler handler_;
		std::unique_ptr<http::handler> load_handler_;
		int status_ = 0;
		std::vector<char> data_;
	public:

		http_client(Handler handler)
			: handler_{ std::move(handler) }
		{
		}

		void set_handler(std::unique_ptr<http::handler> handler) override
		{
			load_handler_ = std::move(handler);
		}

		void on_headers(int status, const tangle::cstring& reason, const http::headers& headers) override
		{
			status_ = status;

#if 0
			std::cout << status << " " << reason << "\n";
			for (const auto& pair : headers) {
				for (const auto& value : pair.second) {
					std::cout << pair.first << ": " << value << "\n";
				}
			}
#endif
		}

		void on_data(const char* data, size_t length) override
		{
			if (length == 0) {
				auto value = json::from_string(data_.data(), data_.size());
				if (value.is<json::NULLPTR>()) {
					printf("%s\n", std::string{data_.data(), data_.size()}.c_str());
				}
				data_.clear();
				handler_(status_, value);
				load_handler_.reset(); // this will start a destroy cascade
				return;                // so do not touch anything and run...
			}
			data_.insert(data_.end(), data, data + length);
		}
	};

	template <typename Handler>
	class http_json_client : public http_client<Handler> {
	public:
		http_json_client(Handler handler)
			: http_client<Handler>{ std::move(handler) }
		{
		}

		tangle::cstring content_type() override { return "text/json"; }
	};

	template <typename Handler>
	auto make_client(Handler handler)
	{
		return std::make_unique<http_client<Handler>>(std::move(handler));
	}

	template <typename Handler>
	auto make_json_client(Handler handler)
	{
		return std::make_unique<http_json_client<Handler>>(std::move(handler));
	}

	template <typename T>
	static inline bool unpack_json(T& ctx, json::value doc) {
		if (!doc.is<json::MAP>())
			return false;
		return json::unpack(ctx, doc);
	}

	template <typename T>
	static inline bool unpack_json(std::vector<T>& ctx, json::value doc) {
		if (!doc.is<json::VECTOR>())
			return false;
		return json::unpack(ctx, doc);
	}

	static inline json::value map(json::value obj, const std::string& key) {
		if (!obj.is<json::MAP>())
			return {};

		json::map m{ obj };
		auto it = m.find(key);
		if (it == m.end())
			return {};
		return it->second;
	}

	template <typename F>
	static inline F foreach(json::value obj, F pred) {
		if (!obj.is<json::VECTOR>())
			return pred;

		for (auto elem : json::vector{ obj }) {
			pred(elem);
		}

		return pred;
	}

	template <typename F>
	static inline F find_first(json::value obj, F pred) {
		if (!obj.is<json::VECTOR>())
			return pred;

		for (auto elem : json::vector{ obj }) {
			if (pred(elem))
				break;
		}

		return pred;
	}

	static inline bool get_error(api::error_type& error, json::value doc)
	{
		std::vector<std::unordered_map<std::string, api::error_type>> ctx;
		if (!unpack_json(ctx, doc))
			return false;
		if (ctx.empty() || ctx.front().empty())
			return false;

		auto it = ctx.front().find("error");
		if (it == ctx.front().end())
			return false;

		error = std::move(it->second);
		return true;
	}

	static inline bool get_error(api::errors& code, json::value doc)
	{
		api::error_type err;
		if (!get_error(err, doc))
			return false;
		code = (api::errors)err.type;
		return true;
	}
}
