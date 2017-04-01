/*
 * Copyright (C) 2014 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __JSON_SERDES_HPP__
#define __JSON_SERDES_HPP__

#include <stdint.h>

namespace json
{
	template <typename T>
	struct translator;

	template <typename T>
	inline value pack(const T& ctx) {
		translator<T> p;
		return p.pack(&ctx);
	}

	template <typename T>
	inline T unpack(const value& v) {
		translator<T> p;
		T ctx;
		if (!p.unpack(v, &ctx))
			return T{};
		return ctx;
	}

	struct base_translator {
		virtual ~base_translator() {}
		virtual value pack(const void* ctx) const = 0;
		virtual bool unpack(const value& v, void* ctx) const = 0;
	};

	struct named_translator : base_translator {
		virtual const std::string& name() const = 0;
		virtual bool valid(const void*) const { return true; }
		virtual bool optional() const { return false; }
		virtual void clean(void* ctx) const = 0;
	};

	template <typename T>
	struct simple_translator : base_translator{
		value pack(const void* ctx) const override {
			return *static_cast<const T*>(ctx);
		}
		bool unpack(const value& v, void* ctx) const override {
			static const type expected = type_to_value<T>::value;
			T& ref = *static_cast<T*>(ctx);
			ref = T(v.as<expected>());
			return true;
		}
	};

	template <typename C>
	struct container_translator : base_translator
	{
		value pack(const void* ctx) const override {
			vector out;
			const C& container = *static_cast<const C*>(ctx);
			for (auto&& item : container)
				out.add(json::pack(item));
			return out;
		}

		bool unpack(const value& v, void* ctx) const override {
			auto in = get<VECTOR>(v);
			C& out = *static_cast<C*>(ctx);
			out.clear();

			using value_t = typename C::value_type;

			translator<value_t> sub;
			for (auto&& item : in) {
				value_t new_ctx;
				if (!sub.unpack(item, &new_ctx))
					return false;

				out.push_back(new_ctx);
			}

			return true;
		}
	};

	template <typename T>
	struct translator<std::map<std::string, T>> : base_translator
	{
		value pack(const void* ctx) const override {
			map out;
			using C = std::map<std::string, T>;
			const C& container = *static_cast<const C*>(ctx);
			for (auto&& item : container)
				out.add(item.first, json::pack(item.second));
			return out;
		}

		bool unpack(const value& v, void* ctx) const override {
			auto in = get<MAP>(v);
			using C = std::map<std::string, T>;
			C& out = *static_cast<C*>(ctx);
			out.clear();

			using value_t = T;
			translator<value_t> sub;
			for (auto&& item : in) {
				value_t& new_ctx = out[item.first];
				if (!sub.unpack(item.second, &new_ctx))
					return false;
			}

			return true;
		}
	};

#define SIMPLE_TRANSLATOR(type) \
	template <> \
	struct translator<type>: simple_translator<type>{}

#define CONTAINER_TRANSLATOR(C) \
	template <typename T> \
	struct translator<C<T>> : container_translator<C<T>>{}

	SIMPLE_TRANSLATOR(std::nullptr_t);
	SIMPLE_TRANSLATOR(bool);
	SIMPLE_TRANSLATOR(std::string);
	SIMPLE_TRANSLATOR(int8_t);
	SIMPLE_TRANSLATOR(int16_t);
	SIMPLE_TRANSLATOR(int32_t);
	SIMPLE_TRANSLATOR(int64_t);
	SIMPLE_TRANSLATOR(float);
	SIMPLE_TRANSLATOR(double);
	CONTAINER_TRANSLATOR(std::vector);
	CONTAINER_TRANSLATOR(std::list);

	template <typename T, typename P>
	struct member_translator : named_translator
	{
		std::string m_name;
		P T::* m_prop;

		member_translator(const std::string& name, P T::* prop) : m_name(name), m_prop(prop) {}
		const std::string& name() const override { return m_name; }
		value pack(const void* ctx) const override {
			auto ptr = static_cast<const T*>(ctx);
			return json::pack(ptr->*m_prop);
		}

		bool unpack(const value& v, void* ctx) const override {
			auto ptr = static_cast<T*>(ctx);
			translator<P> p;
			return p.unpack(v, &(ptr->*m_prop));
		}

		void clean(void* ctx) const override {
			auto ptr = static_cast<T*>(ctx);
			ptr->*m_prop = P();
		}
	};

	template <typename T, typename P>
	struct member_opt_translator : member_translator<T, P>
	{
		member_opt_translator(const std::string& name, P T::* prop) : member_translator<T, P>(name, prop) {}

		bool valid(const void* ctx) const override {
			auto __prop = member_translator<T, P>::m_prop;
			auto ptr = static_cast<const T*>(ctx);
			auto prop = ptr->*__prop;
			return prop != P();
		}
		bool optional() const override { return true; }

		void clean(void* ctx) const override {
			auto __prop = member_translator<T, P>::m_prop;
			auto ptr = static_cast<T*>(ctx);
			ptr->*__prop = P();
		}
	};

	namespace impl
	{
		template <typename C>
		const typename C::value_type& front(const C& container) {
			return container.front();
		}

		template <typename T>
		const T& front(const std::map<std::string, T>& container) {
			return container.begin()->second;
		}

		template <typename C>
		typename C::value_type& front(C& container) {
			container.push_back(typename C::value_type{});
			return container.front();
		}

		template <typename T>
		T& front(std::map<std::string, T>& container) {
			return container[""];
		}

		template <typename C> struct item_expected : std::integral_constant<type, VECTOR> { using value_type = typename C::value_type; };
		template <typename T> struct item_expected<std::map<std::string, T>> : std::integral_constant<type, MAP>{ using value_type = T; };

	}

	template <typename T, typename P>
	struct member_item_translator : named_translator
	{
		std::string m_name;
		P T::* m_prop;

		member_item_translator(const std::string& name, P T::* prop) : m_name(name), m_prop(prop) {}
		const std::string& name() const override { return m_name; }
		value pack(const void* ctx) const override {
			auto ptr = static_cast<const T*>(ctx);
			const auto& prop = ptr->*m_prop;
			if (prop.size() == 1) {
				return json::pack(json::impl::front(prop));
			}
			return json::pack(prop);
		}

		bool unpack(const value& v, void* ctx) const override {
			auto ptr = static_cast<const T*>(ctx);
			P& prop = const_cast<P&>(ptr->*m_prop);
			prop.clear();

			if (v.is<impl::item_expected<P>::value>()) {
				translator<P> p;
				return p.unpack(v, &prop);
			}

			using item_t = typename impl::item_expected<P>::value_type;
			translator<item_t> p;
			item_t& new_ctx = impl::front(prop);
			return p.unpack(v, &new_ctx);
		}

		bool valid(const void* ctx) const override {
			auto ptr = static_cast<const T*>(ctx);
			auto prop = ptr->*m_prop;
			return !prop.empty();
		}

		void clean(void* ctx) const override {
			auto ptr = static_cast<T*>(ctx);
			ptr->*m_prop = P();
		}
	};

	template <typename T>
	struct struct_translator: base_translator
	{
		using props_t = std::vector<std::unique_ptr<named_translator>>;
		props_t m_props;

		template <typename P>
		void add_prop(const std::string& name, P T::* prop) {
			m_props.emplace_back(std::move(std::make_unique<member_translator<T, P>>(name, prop)));
		}

		template <typename P>
		void add_opt_prop(const std::string& name, P T::* prop) {
			m_props.emplace_back(std::move(std::make_unique<member_opt_translator<T, P>>(name, prop)));
		}

		template <typename P>
		void add_item_prop(const std::string& name, P T::* prop) {
			m_props.emplace_back(std::move(std::make_unique<member_item_translator<T, P>>(name, prop)));
		}

		value pack(const void* ctx) const override {
			map obj;
			for (auto&& prop : m_props) {
				if (!prop->valid(ctx))
					continue;

				obj.add(prop->name(), prop->pack(ctx));
			}
			return obj;
		}

		bool unpack(const value& v, void* ctx) const override {
			if (!v.is<MAP>())
				return false;

			auto obj = get<MAP>(v);

			for (auto&& prop : m_props) {
				auto it = obj.find(prop->name());

				if (it == obj.end()) {
					if (!prop->optional())
						return false;
					prop->clean(ctx);
					continue;
				}

				if (!prop->unpack(it->second, ctx))
					return false;
			}

			return true;
		}
	};

#define JSON_STRUCT(name) \
	template <> \
	struct translator<name> : struct_translator<name>{ \
		using sser = struct_translator<name>; \
		using my_type = name; \
		translator(); \
	}; \
	inline translator<name>::translator()

#define JSON_NAMED_PROP(name, prop) \
	sser::add_prop(name, &my_type::prop)

#define JSON_PROP(prop) JSON_NAMED_PROP(#prop, prop)

#define JSON_OPT_NAMED_PROP(name, prop) \
	sser::add_opt_prop(name, &my_type::prop)

#define JSON_OPT_PROP(prop) JSON_OPT_NAMED_PROP(#prop, prop)

#define JSON_ITEM_NAMED_PROP(name, prop) \
	sser::add_item_prop(name, &my_type::prop)

#define JSON_ITEM_PROP(prop) JSON_ITEM_NAMED_PROP(#prop, prop)
}

#endif // __JSON_SERDES_HPP__
