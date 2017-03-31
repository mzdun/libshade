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

#include <tangle/msg/base_parser.h>
#include <algorithm>
#include <cctype>

namespace tangle { namespace msg {
	namespace {
		template <typename V>
		auto find(V& contents, const char* sub)
		{
			auto cur = std::begin(contents);
			auto end = std::end(contents);

			if (!sub || !*sub)
				return end;

			auto len = strlen(sub);
			do {
				cur = std::find(cur, end, *sub);
				if (cur == end) break;
				if (size_t(end - cur) < len) break;
				if (!memcmp(&*cur, sub, len))
					return cur;
				++cur;
			} while (true);
			return end;
		}

		inline size_t report_read(size_t prev, size_t position)
		{
			return (position > prev) ? prev - position : 0;
		}
	}

	std::string printable(std::string&& s)
	{
		for (auto& c : s)
			c = isprint((uint8_t)c) ? c : '.';
		return s;
	}

	std::pair<size_t, parsing> base_parser::append(const char* data, size_t length)
	{
		if (!length)
			return { 0, parsing::reading };

		auto prev = m_contents.size();
		m_contents.insert(m_contents.end(), data, data + length);

		auto begin = std::begin(m_contents);
		auto cur = std::next(begin, m_last_line_end);
		auto end = std::end(m_contents);

		while (cur != end) {
			auto it = std::find(cur, end, '\r');
			if (it == end) break;
			if (std::next(it) == end) break;
			if (*std::next(it) != '\n') // mid-line \r? - check with RFC if ignore, or error
				return { report_read(prev, std::distance(begin, it)), parsing::error };

			if (it == cur) { // empty line
				if (!rearrange())
					return { report_read(prev, std::distance(begin, it)), parsing::error };
				return { report_read(prev, std::distance(begin, it)), parsing::separator };
			}

			std::advance(it, 2);
			if (isspace((uint8_t)*cur)) {
				if (m_field_list.empty())
					return { report_read(prev, std::distance(begin, it)), parsing::error };

				m_last_line_end = std::distance(begin, it);
				auto& fld = std::get<1>(m_field_list.back());
				fld = span(fld.offset(), m_last_line_end - fld.offset());
			} else {
				auto colon = std::find(cur, it, ':');
				if (colon == it) // no colon in field's first line
					return { report_read(prev, std::distance(begin, it)), parsing::error };

				m_last_line_end = std::distance(begin, it);
				m_field_list.emplace_back(
					span(std::distance(begin, cur), std::distance(cur, colon)),
					span(std::distance(begin, colon) + 1, std::distance(colon, it) - 1)
				);
			}

			cur = it;
		}
		return { length, parsing::reading };
	}

	std::string produce(cstring cs)
	{
		auto ptr = cs.c_str();
		auto len = cs.length();
		auto end = ptr + len;

		while (ptr != end && std::isspace((uint8_t)*ptr)) {
			++ptr;
			--len;
		}
		while (ptr != end && std::isspace((uint8_t)end[-1])) {
			--end;
			--len;
		}
		cs = { ptr, len };

		bool in_cont = false;
		for (auto cur = ptr; cur != end; ++cur) {
			uint8_t uc = *cur;
			if (in_cont) {
				in_cont = !!std::isspace(uc);
				if (in_cont)
					--len;
				continue;
			}
			if (uc == '\r') {
				in_cont = true;
				// while (ptr != cur && std::isspace((uint8_t)cur[-1])) {
				// 	--cur;
				// 	--len;
				// }
				continue;
			}
		}

		std::string out;
		out.reserve(len);

		in_cont = false;
		for (auto cur = ptr; cur != end; ++cur) {
			uint8_t uc = *cur;
			if (in_cont) {
				in_cont = !!std::isspace(uc);
				if (in_cont)
					--len;
				else
					out.push_back(uc);
				continue;
			}
			if (uc == '\r') {
				in_cont = true;
				while (ptr != cur && std::isspace((uint8_t)cur[-1])) {
					--cur;
					out.pop_back();
				}
				out.push_back(' ');
				continue;
			}
			out.push_back(uc);
		}

		out.shrink_to_fit();
		return out;
	}

	std::string lower(std::string&& in)
	{
		for (auto& c : in)
			c = std::tolower((uint8_t)c);
		return in;
	}

	bool base_parser::rearrange()
	{
		m_dict.clear();

		for (auto& pair : m_field_list){
			auto key = lower(produce(get(std::get<0>(pair))));
			auto value = produce(get(std::get<1>(pair)));
			m_dict[std::move(key)].emplace_back(std::move(value));
		}

		m_field_list.clear();
		m_contents.clear();
		m_contents.shrink_to_fit();
		return true;
	}
}};
