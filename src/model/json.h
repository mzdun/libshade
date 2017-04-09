#pragma once

#include <json/json.hpp>
#include <json/serdes.hpp>

#define JSON_STATIC_DECL(name) \
	template <> \
	struct translator<name> : struct_translator { \
		using my_type = name; \
		translator() { my_type::prepare(*this); } \
	}

#define NAMED_PROP(name, prop) \
	add_prop(name, &my_type::prop)

#define PROP(prop) NAMED_PROP(#prop, prop)
#define PRIV_PROP(prop) NAMED_PROP(#prop, prop ## _)

#define OPT_NAMED_PROP(name, prop) \
	add_opt_prop(name, &my_type::prop)

#define OPT_PROP(prop) OPT_NAMED_PROP(#prop, prop)
#define OPT_PRIV_PROP(prop) OPT_NAMED_PROP(#prop, prop ## _)

#define ITEM_NAMED_PROP(name, prop) \
	add_item_prop(name, &my_type::prop)

#define ITEM_PROP(prop) ITEM_NAMED_PROP(#prop, prop)

namespace json {
	template <typename T>
	struct translator<std::shared_ptr<T>> {
		value pack(const void* ctx, ctx_env& env) const
		{
			auto& ptr = *static_cast<const std::shared_ptr<T>*>(ctx);
			if (!ptr)
				return {};

			translator<T> inner;
			return inner.pack(ptr.get(), env);
		}

		bool unpack(const value& v, void* ctx, ctx_env& env) const
		{
			auto& ptr = *static_cast<std::shared_ptr<T>*>(ctx);
			if (v.is<json::NULLPTR>()) {
				ptr.reset();
				return true;
			}

			T val;
			translator<T> inner;
			if (!inner.unpack(v, &val, env))
				return false;

			ptr = std::make_shared<T>(std::move(val));
			return true;
		}
	};
}
