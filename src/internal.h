#include <shade/manager.h>
#include <shade/hue_data.h>
#include <json.hpp>

using namespace std::literals;

namespace json {
	JSON_STRUCT(shade::hue::config) {
		JSON_PROP(name);
		JSON_PROP(mac);
		JSON_PROP(modelid);
	}

	JSON_STRUCT(shade::hue::group_state) {
		JSON_PROP(all_on);
		JSON_PROP(any_on);
	};

	JSON_STRUCT(shade::hue::light_state) {
		JSON_PROP(on);
		JSON_PROP(bri);
		JSON_PROP(hue);
		JSON_PROP(sat);
		JSON_PROP(ct);
		JSON_PROP(xy);
		JSON_PROP(colormode);
	};

	JSON_STRUCT(shade::hue::group) {
		JSON_PROP(name);
		JSON_PROP(type);
		JSON_OPT_NAMED_PROP("class", klass);
		JSON_PROP(lights);
		JSON_PROP(state);
		JSON_PROP(action);
	};

	JSON_STRUCT(shade::hue::light) {
		JSON_PROP(name);
		JSON_PROP(modelid);
		JSON_PROP(uniqueid);
		JSON_PROP(state);
	};

	JSON_STRUCT(shade::hue::error_type) {
		JSON_PROP(type);
		JSON_PROP(address);
		JSON_PROP(description);
	};
}

namespace shade {

	namespace io {
		template <typename Handler>
		class http_client : public http::listener {
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
			}

			void on_data(const char* data, size_t length) override
			{
				if (length == 0) {
					auto value = json::from_string(data_.data(), data_.size());
					if (value.is<json::NULLPTR>()) {
						printf("%s\n", std::string{ data_.data(), data_.size() }.c_str());
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

	static inline bool get_error(hue::error_type& error, json::value doc)
	{
		std::vector<std::unordered_map<std::string, hue::error_type>> ctx;
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

	static inline bool get_error(hue::errors& code, json::value doc)
	{
		hue::error_type err;
		if (!get_error(err, doc))
			return false;
		code = (hue::errors)err.type;
		return true;
	}
}
