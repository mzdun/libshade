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
#include <vector>
#include <tuple>
#include <unordered_map>
#include <tangle/msg/base_parser.h>

namespace tangle { namespace msg {
	struct http_version {
		int http_major;
		int http_minor;
		http_version(int http_major = 0, int http_minor = 0)
			: http_major(http_major), http_minor(http_minor)
		{
		}
	};

	template <typename Final>
	class http_parser_base {
	public:
		using dict_t = base_parser::dict_t;
		std::pair<size_t, parsing> append(const char* data, size_t length);
		const http_version& proto() const { return m_proto; }
		auto dict() { return m_fields.dict(); }
	protected:
		void set_proto(int http_major, int http_minor)
		{
			m_proto = { http_major, http_minor };
		}
	private:
		base_parser m_fields;
		http_version m_proto;
		bool m_needs_first_line = true;
	};

	template <typename Final>
	inline std::pair<size_t, parsing> http_parser_base<Final>::append(const char* data, size_t length)
	{
		if (m_needs_first_line) {
			auto& thiz = static_cast<Final&>(*this);
			auto ret = thiz.first_line(data, length);

			if (std::get<parsing>(ret) != parsing::separator)
				return ret;

			m_needs_first_line = false;
			auto offset = std::get<size_t>(ret);
			ret = m_fields.append(data + offset, length - offset);
			std::get<size_t>(ret) += offset;
			return ret;
		}
		return m_fields.append(data, length);
	}

	class http_request : public http_parser_base<http_request> {
		friend class http_parser_base<http_request>;
		std::pair<size_t, parsing> first_line(const char* data, size_t length);
	public:
		const std::string& method() const { return m_method; }
		const std::string& resource() const { return m_resource; }
	private:
		std::string m_method;
		std::string m_resource;
	};

	class http_response : public http_parser_base<http_response> {
		friend class http_parser_base<http_response>;
		std::pair<size_t, parsing> first_line(const char* data, size_t length);
	public:
		int status() const { return m_status; }
		const std::string& reason() const { return m_reason; }
	private:
		int m_status = 0;
		std::string m_reason;
	};
}};
