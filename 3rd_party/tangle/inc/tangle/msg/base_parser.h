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
#include <tangle/msg/hasher.h>

namespace tangle { namespace msg {
	enum class parsing {
		reading,
		separator,
		error
	};

	class base_parser {
	public:
		class span {
		public:
			constexpr span() = default;
			constexpr span(size_t offset, size_t length)
				: m_offset(offset)
				, m_length(length)
			{
			}

			constexpr size_t offset() const { return m_offset; }
			constexpr size_t length() const { return m_length; }
		private:
			size_t m_offset = 0;
			size_t m_length = 0;
			size_t m_hash = 0;
		};

		using dict_t = std::unordered_map<combined_string, std::vector<std::string>>;

		std::pair<size_t, parsing> append(const char* data, size_t length);

		auto dict()
		{
			return std::move(m_dict);
		}
	private:
		std::vector<char> m_contents;
		std::vector<std::tuple<span, span>> m_field_list;
		dict_t m_dict;
		size_t m_last_line_end = 0;

		bool rearrange();
		cstring get(span s)
		{
			return { m_contents.data() + s.offset(), s.length() };
		}
	};
}};
