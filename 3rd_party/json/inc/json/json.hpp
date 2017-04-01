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

#ifndef __JSON_JSON_HPP__
#define __JSON_JSON_HPP__

#include <string>
#include <map>
#include <list>
#include <vector>
#include <memory>
#include <iostream>

namespace json
{
	class bad_cast : public std::exception {
		const char* msg = "bad JSON cast";
	public:

		bad_cast() = default;
		bad_cast(char const* const msg) : msg(msg) {}

		char const* what() const noexcept override { return msg; }
	};

	struct value;
	struct vector;
	struct map;

	void json_string(std::ostream&, const std::string&);

	enum type {
		NULLPTR,
		BOOL,
		NUMBER,
		INTEGER,
		FLOAT,
		STRING,
		VECTOR,
		MAP
	};

	template <type value_type>
	struct value_traits;

	template <typename T>
	struct type_to_value;

	template <> struct type_to_value<std::nullptr_t>: std::integral_constant<type, NULLPTR>{};
	template <> struct type_to_value<bool>: std::integral_constant<type, BOOL>{};
	template <> struct type_to_value<int8_t >: std::integral_constant<type, INTEGER>{};
	template <> struct type_to_value<int16_t>: std::integral_constant<type, INTEGER>{};
	template <> struct type_to_value<int32_t>: std::integral_constant<type, INTEGER>{};
	template <> struct type_to_value<int64_t>: std::integral_constant<type, INTEGER>{};
	template <> struct type_to_value<float>: std::integral_constant<type, FLOAT>{};
	template <> struct type_to_value<double>: std::integral_constant<type, FLOAT>{};
	template <> struct type_to_value<std::string>: std::integral_constant<type, STRING>{};
	template <> struct type_to_value<vector>: std::integral_constant<type, VECTOR>{};
	template <> struct type_to_value<map>: std::integral_constant<type, MAP>{};

	template <type value_type>
	using value_t = typename value_traits<value_type>::type;

	template <type value_type>
	value_t<value_type> get(const value& v) { return value_traits<value_type>::__get(v); }

	template <typename T>
	value_t<type_to_value<T>::value> get(const value& v) { return value_traits<type_to_value<T>::value>::__get(v); }

	struct value {
		struct options {
			struct {
				const char* namesep;
				const char* listsep;
				const char* indentStr;
				int indent;
				bool doIndent;
			} val;

			static options dense() {
				return{ { ":", ",", " ", 0, false } };
			}

			static options indented(int indent = 2) {
				return{ { ": ", ",", " ", indent, true } };
			}
			static options indented(const char* indentStr, int indent = 1) {
				return{ { ": ", ",", indentStr, indent, true } };
			}
		};

		struct backend_base {
			virtual void to_string(std::ostream&, const options&, int) const = 0;
			virtual void to_html(std::ostream&, const options&, int) const = 0;
			virtual type get_type() const = 0;
			template <type value_type>
			bool is() const { return get_type() == value_type; }

			virtual const std::string& as_string() const {
				static const std::string s;
				return s;
			}
			virtual int64_t as_int() const { return 0; }
			virtual double as_double() const { return 0.0; }
			virtual bool as_bool() const { return false; }
		};
		using backend_ptr = std::shared_ptr<backend_base>;

		template <type value_type>
		struct backend : backend_base {
			type get_type() const override { return value_type; }
		};

		value() {} // null
		value(std::nullptr_t) {} // null
		value(bool value) { use<logical>(value); }
		value(int8_t value) { use<integer>(value); }
		value(int16_t value) { use<integer>(value); }
		value(int32_t value) { use<integer>(value); }
		value(int64_t value) { use<integer>(value); }
		value(float value) { use<floating>(value); }
		value(double value) { use<floating>(value); }
		value(const char* value) { use<string>(value); }
		value(const std::string& value) { use<string>(value); }

		explicit operator bool() const {
			return !is<NULLPTR>();
		}

		type get_type() const {
			if (m_back)
				return m_back->get_type();
			return NULLPTR;
		}
		template <type value_type>
		bool is() const {
			if (m_back)
				return m_back->is<value_type>();
			return value_type == NULLPTR;
		}

		template <typename T>
		bool is() const {
			return is<type_to_value<T>::value>();
		}

		bool as_bool() const {
			if (m_back)
				return m_back->as_bool();
			return false;
		}
		const std::string& as_string() const {
			if (m_back)
				return m_back->as_string();
			static const std::string s;
			return s;
		}
		int64_t as_int() const {
			if (m_back)
				return m_back->as_int();
			return 0;
		}

		double as_double() const {
			if (m_back)
				return m_back->as_double();
			return 0.0;
		}

		template <type value_type>
		value_t<value_type> as() const {
			return get<value_type>(*this);
		}

		template <typename T>
		value_t<type_to_value<T>::value> as() const {
			return as<type_to_value<T>::value>();
		}

		std::string to_string(const options& = options::dense()) const;
		std::string to_html(const options& = options::dense()) const;

		void to_string(std::ostream& o, const options& opts, int offset) const {
			if (!m_back)
				o << "null";
			else
				m_back->to_string(o, opts, offset);
		}

		void to_html(std::ostream& o, const options& opts, int offset) const {
			if (!m_back)
				o << "<span class='cpp-keyword'>null</span>";
			else
				m_back->to_html(o, opts, offset);
		}

	protected:
		template <typename T, typename... Args>
		void use(Args&&... args) {
			m_back = std::make_shared<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		T* back() { return static_cast<T*>(m_back.get()); }
		template <typename T>
		const T* back() const { return static_cast<const T*>(m_back.get()); }
	private:
		backend_ptr m_back;

		struct logical: backend<BOOL> {
			bool value;

			logical(bool value) : value(value) {}
			void to_string(std::ostream& o, const options&, int) const override { o << (value ? "true" : "false"); }
			void to_html(std::ostream& o, const options&, int) const override { o << "<span class='cpp-keyword'>" << (value ? "true" : "false") << "</span>"; }
			bool as_bool() const override { return value; }
		};

		struct integer : backend<INTEGER> {
			int64_t value;

			integer(int64_t value) : value(value) {}
			void to_string(std::ostream& o, const options&, int) const override { o << std::to_string(value); }
			void to_html(std::ostream& o, const options&, int) const override { o << "<span class='cpp-number'>" << std::to_string(value) << "</span>"; }
			int64_t as_int() const override { return value; }
		};

		struct floating : backend<FLOAT> {
			double value;

			floating(double value) : value(value) {}
			void to_string(std::ostream& o, const options&, int) const override { o << std::to_string(value); }
			void to_html(std::ostream& o, const options&, int) const override { o << "<span class='cpp-number'>" << std::to_string(value) << "</span>"; }
			double as_double() const override { return value; }
		};

		struct string : backend<STRING> {
			std::string value;

			string(std::string value) : value(value) {}
			void to_string(std::ostream& o, const options&, int) const override { json_string(o, value); }
			void to_html(std::ostream& o, const options&, int) const override
			{
				o << "<span class='cpp-string'>";
				json_string(o, value);
				o << "</span>";
			}
			const std::string& as_string() const override { return value; }
		};
	};

	template <>
	inline bool value::is<NUMBER>() const {
		return is<INTEGER>() || is<FLOAT>();
	}


	value from_string(const std::string&);
	value from_string(const char* data, size_t length);

	struct vector : value {
		using container_t = std::vector<value>;
		using iterator = container_t::iterator;
		using const_iterator = container_t::const_iterator;
		using reference = container_t::reference;
		using const_reference = container_t::const_reference;

		vector() { use<backend>(); }
		explicit vector(const value& oth) : value(oth) {
			if (!is<VECTOR>())
				throw bad_cast("JSON value is not a vector");
		}

		vector& add(const value& v) { values().push_back(v); return *this; }
		size_t size() const { return values().size(); }
		iterator begin() { return values().begin(); }
		iterator end() { return values().end(); }
		const_iterator begin() const { return values().begin(); }
		const_iterator end() const { return values().end(); }

		template <type value_type>
		value_t<value_type> as(size_t ndx) const {
			return get<value_type>(at(ndx));
		}

		reference at(size_t ndx) { return values().at(ndx); }
		reference operator[](size_t ndx) { return values()[ndx]; }
		const_reference at(size_t ndx) const { return values().at(ndx); }
		const_reference operator[](size_t ndx) const { return values()[ndx]; }

	private:
		struct backend : value::backend<VECTOR> {
			container_t values;
			void to_string(std::ostream&, const options&, int) const override;
			void to_html(std::ostream&, const options&, int) const override;
		};

		container_t& values() { return back<backend>()->values; }
		const container_t& values() const { return back<backend>()->values; }
	};

	template <typename T>
	vector& operator << (vector& v, const T& value) {
		return v.add(value);
	}

	struct map : value {
		using container_t = std::map<std::string, value>;
		using iterator = container_t::iterator;
		using const_iterator = container_t::const_iterator;
		using mapped_type = container_t::mapped_type;

		map() { use<backend>(); }
		explicit map(const value& oth) : value(oth) {
			if (!is<MAP>())
				throw bad_cast("JSON value is not a map");
		}
		map(const map&) = default;
		map&operator=(const map&) = default;
		map(map&&) = default;
		map&operator=(map&&) = default;

		map& add(const std::string& k, const value& v);

		size_t size() const { return values().size(); }
		iterator begin() { return values().begin(); }
		iterator end() { return values().end(); }
		const_iterator begin() const { return values().begin(); }
		const_iterator end() const { return values().end(); }
		iterator find(const std::string& key) { return values().find(key); }
		const_iterator find(const std::string& key) const { return values().find(key); }
		void erase(const_iterator it) { values().erase(it); }

		template <type value_type>
		value_t<value_type> as(const std::string& key) const {
			return get<value_type>(at(key));
		}

		mapped_type& at(const std::string& key) { return values().at(key); }
		const mapped_type& at(const std::string& key) const { return values().at(key); }
		mapped_type& operator[](const std::string& key) { return values()[key]; }
		//const mapped_type& operator[](const std::string& key) const { return values()[key]; }
	private:
		struct backend : value::backend<MAP> {
			container_t values;
			void to_string(std::ostream&, const options&, int) const override;
			void to_html(std::ostream&, const options&, int) const override;
		};

		container_t& values() { return back<backend>()->values; }
		const container_t& values() const { return back<backend>()->values; }
	};

	template <typename K, typename V>
	map& operator << (map& m, const std::pair<K, V>& value) {
		return m.add(value.first, value.second);
	}

	template <>
	struct value_traits<NULLPTR> {
		using type = std::nullptr_t;
		static inline type __get(const value&)
		{
			return nullptr;
		}
	};

	template <>
	struct value_traits<BOOL> {
		using type = bool;
		static inline type __get(const value& v)
		{
			return v.as_bool();
		}
	};

	template <>
	struct value_traits<INTEGER> {
		using type = int64_t;
		static inline type __get(const value& v)
		{
			return v.as_int();
		}
	};

	template <>
	struct value_traits<FLOAT> {
		using type = double;
		static inline type __get(const value& v)
		{
			return v.as_double();
		}
	};

	template <>
	struct value_traits<STRING> {
		using type = const std::string&;
		static inline type __get(const value& v) {
			return v.as_string();
		}
	};

	template <>
	struct value_traits<VECTOR> {
		using type = vector;
		static inline type __get(const value& v)
		{
			if (!v.is<VECTOR>())
				return type();
			return type{ v };
		}
	};

	template <>
	struct value_traits<MAP> {
		using type = map;
		static inline type __get(const value& v)
		{
			if (!v.is<MAP>())
				return type();
			return type{ v };
		}
	};
}

#endif // __JSON_JSON_HPP__
