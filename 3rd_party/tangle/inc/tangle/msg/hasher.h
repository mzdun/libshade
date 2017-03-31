/*
 * Copyright (C) 2016 midnightBITS
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

#pragma once
#include <string>
#include <tangle/cstring.h>

namespace tangle { namespace msg {
	class hasher {
	public:
		hasher();
		size_t value() const { return m_value; }
		hasher& append(const void* buffer, size_t length);
		static size_t hash(const std::string& s)
		{
			return hash(s.data(), s.length());
		}
		static size_t hash(const cstring& s)
		{
			return hash(s.data(), s.length());
		}
		static size_t hash(const char*, size_t);
	private:
		size_t m_value;
	};

	class combined_string {
	public:
		combined_string()
		{
			new (&m_cstr) cstring { };
		}

		combined_string(const cstring& key)
		{
			new (&m_cstr) cstring { key };
		}

		combined_string(const char* key)
		{
			new (&m_cstr) cstring { key };
		}

		combined_string(const std::string& key) : m_use_cstr { false }
		{
			new (&m_str) std::string { key };
		}

		combined_string(std::string&& key) : m_use_cstr { false }
		{
			new (&m_str) std::string { std::move(key) };
		}

		combined_string(const combined_string& rhs) : m_use_cstr { rhs.m_use_cstr }
		{
			if (m_use_cstr)
				new (&m_cstr) cstring { rhs.m_cstr };
			else
				new (&m_str) std::string { rhs.m_str };
		}

		combined_string& operator=(const combined_string& rhs)
		{
			if (this == &rhs)
				return *this;

			if (m_use_cstr)
				m_cstr.~cstring();
			else
				m_str.~basic_string();

			m_use_cstr = rhs.m_use_cstr;
			if (m_use_cstr)
				new (&m_cstr) cstring { rhs.m_cstr };
			else
				new (&m_str) std::string { rhs.m_str };

			return *this;
		}

		~combined_string()
		{
			if (m_use_cstr)
				m_cstr.~cstring();
			else
				m_str.~basic_string();
		}

		const char* c_str() const
		{
			return m_use_cstr ? m_cstr.c_str() : m_str.c_str();
		}

		size_t length() const
		{
			return m_use_cstr ? m_cstr.length() : m_str.length();
		}

		size_t hash() const
		{
			return m_use_cstr ? hasher::hash(m_cstr) : hasher::hash(m_str);
		}

		std::string str() const { return { c_str(), length() }; }
		operator cstring() const
		{
			return { c_str(), length() };
		}

		bool operator == (const combined_string& rhs) const
		{
			return (cstring)*this == rhs;
		}

		friend inline std::ostream& operator<<(std::ostream& o, const combined_string& cs)
		{
			return o << (cstring)cs;
		}

	private:
		union {
			std::string m_str;
			cstring m_cstr;
		};
		bool m_use_cstr = true;
	};

}};

namespace std {
	template <>
	struct hash<tangle::cstring> : unary_function<tangle::cstring, size_t> {
		size_t operator()(const tangle::cstring& key) const
		{
			return tangle::msg::hasher::hash(key);
		}
	};
	template <>
	struct hash<tangle::msg::combined_string> : std::unary_function<tangle::msg::combined_string, size_t> {
		size_t operator()(const tangle::msg::combined_string& key) const
		{
			return key.hash();
		}
	};

};