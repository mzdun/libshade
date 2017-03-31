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

#include <tangle/msg/http_parser.h>
#include <algorithm>

namespace tangle { namespace msg {
	namespace {
		template <typename It>
		It parse_number(It cur, It end, int& val)
		{
			val = 0;
			while (cur != end) {
				switch (*cur) {
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					val *= 10;
					val += *cur++ - '0';
					break;
				default:
					return cur;
				}
			}
			return cur;
		}

#define TEST_CHAR(c) if (cur == end || *cur != (c)) return false; ++cur;
		template <typename It>
		bool parse_proto(It cur, It end, http_version& var)
		{
			TEST_CHAR('H');
			TEST_CHAR('T');
			TEST_CHAR('T');
			TEST_CHAR('P');
			TEST_CHAR('/');

			auto next = parse_number(cur, end, var.http_major);
			if (next == cur) return false;
			cur = next;

			TEST_CHAR('.');

			next = parse_number(cur, end, var.http_minor);
			return next != cur && next == end;
		}
#undef TEST_CHAR
	}

	std::pair<size_t, parsing> http_request::first_line(const char* data, size_t length)
	{
		// Method SP Request-URI SP HTTP-Verson CRLF
		auto cur = data;
		auto end = data + length;
		size_t len = 1;

		if (m_resource.empty() || m_resource.back() != '\r') {
			auto it = std::find(cur, end, '\r');
			if (it == end || it == (end - 1)) {
				m_resource.append(data, length);
				return { length, parsing::reading };
			}
			if (it[1] != '\n')
				return { (it - data), parsing::error };
			m_resource.append(data, it + 1);
			len += it - data + 1;
		} else if (!length)
			return { 0, parsing::reading };
		else if (*data != '\n')
			return { 0, parsing::error };

		m_resource.pop_back();

		auto proto_pos = m_resource.find_last_of(' ');
		if (proto_pos == std::string::npos)
			return { len, parsing::error };
		auto method_pos = m_resource.find_first_of(' ');
		if (proto_pos == method_pos)
			return { len, parsing::error };

		http_version ver;
		if (!parse_proto(
			std::next(m_resource.begin(), proto_pos + 1),
			m_resource.end(), ver)) {
			return { len, parsing::error };
		}

		set_proto(ver.http_major, ver.http_minor);

		m_method = m_resource.substr(0, method_pos);

		while (method_pos < proto_pos && m_resource[method_pos] == ' ')
			++method_pos;
		while (method_pos < proto_pos && m_resource[proto_pos - 1] == ' ')
			--proto_pos;

		if (method_pos == proto_pos)
			return { len, parsing::error };

		m_resource = m_resource.substr(method_pos, proto_pos - method_pos);
		return { len, parsing::separator };
	}

	std::pair<size_t, parsing> http_response::first_line(const char* data, size_t length)
	{
		// HTTP-Version SP Status-Code SP Reason-Phrase CRLF
		auto cur = data;
		auto end = data + length;
		size_t len = 1;

		if (m_reason.empty() || m_reason.back() != '\r') {
			auto it = std::find(cur, end, '\r');
			if (it == end || it == (end - 1)) {
				m_reason.append(data, length);
				return { length, parsing::reading };
			}
			if (it[1] != '\n')
				return { (it - data), parsing::error };
			m_reason.append(data, it + 1);
			len += it - data + 1;
		} else if (!length)
			return { 0, parsing::reading };
		else if (*data != '\n')
			return { 0, parsing::error };

		m_reason.pop_back();

		auto status_pos = m_reason.find_first_of(' ');
		if (status_pos == std::string::npos)
			return { len, parsing::error };

		http_version ver;
		if (!parse_proto(
			m_reason.begin(),
			std::next(m_reason.begin(), status_pos),
			ver)) {
			return { len, parsing::error };
		}

		set_proto(ver.http_major, ver.http_minor);

		{
			auto end = m_reason.end();
			auto cur = m_reason.begin();
			std::advance(cur, status_pos);
			while (cur != end && *cur == ' ')
				++cur;
			auto msg = parse_number(cur, end, m_status);
			if (msg == cur || msg == end || *msg != ' ' || m_status < 100)
				return { len, parsing::error };

			while (msg != end && *msg == ' ')
				++msg;
			m_reason = m_reason.substr(std::distance(m_reason.begin(), msg));
		}

		return { len, parsing::separator };
	}
}}
