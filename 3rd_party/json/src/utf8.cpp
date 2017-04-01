/*
 * Copyright (C) 2015 midnightBITS
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

#include "utf8.hpp"
#include <iterator>

namespace utf
{
	using UTF8 = uint8_t;
	using UTF16 = uint16_t;
	using UTF32 = uint32_t;

	/*
	* Index into the table below with the first byte of a UTF-8 sequence to
	* get the number of trailing bytes that are supposed to follow it.
	* Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
	* left as-is for anyone who may want to do such conversion, which was
	* allowed in earlier algorithms.
	*/
	static const char trailingBytesForUTF8[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};

	/*
	* Magic values subtracted from a buffer value during UTF8 conversion.
	* This table contains as many values as there might be trailing bytes
	* in a UTF-8 sequence.
	*/
	static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL };

	/*
	* Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
	* into the first byte, depending on how many bytes follow.  There are
	* as many entries in this table as there are UTF-8 sequence types.
	* (I.e., one byte sequence, two byte... etc.). Remember that sequencs
	* for *legal* UTF-8 will be 4 or fewer bytes total.
	*/
	static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	enum {
		UNI_SUR_HIGH_START = 0xD800,
		UNI_SUR_HIGH_END = 0xDBFF,
		UNI_SUR_LOW_START = 0xDC00,
		UNI_SUR_LOW_END = 0xDFFF,
	};

	enum {
		UNI_MAX_BMP = 0x0000FFFF,
		UNI_MAX_UTF16 = 0x0010FFFF,
	};

	static const int halfShift = 10; /* used for shifting by 10 bits */

	static const UTF32 halfBase = 0x0010000UL;
	static const UTF32 halfMask = 0x3FFUL;

	template <typename T>
	static bool isLegalUTF8(T source, int length)
	{
		UTF8 a;
		auto srcptr = source + length;
		switch (length) {
		default: return false;
			/* Everything else falls through when "true"... */
		case 4: if ((a = ((uint8_t)*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 3: if ((a = ((uint8_t)*--srcptr)) < 0x80 || a > 0xBF) return false;
		case 2: if ((a = ((uint8_t)*--srcptr)) < 0x80 || a > 0xBF) return false;

			switch ((uint8_t)*source) {
				/* no fall-through in this inner switch */
			case 0xE0: if (a < 0xA0) return false; break;
			case 0xED: if (a > 0x9F) return false; break;
			case 0xF0: if (a < 0x90) return false; break;
			case 0xF4: if (a > 0x8F) return false; break;
			default:   if (a < 0x80) return false;
			}

		case 1: if ((uint8_t) *source >= 0x80 && (uint8_t) *source < 0xC2) return false;
		}
		if ((uint8_t) *source > 0xF4) return false;
		return true;
	}

	std::u16string widen(const std::string& src)
	{
		std::u16string out;
		auto source = src.begin();
		auto sourceEnd = src.end();
		auto target = std::back_inserter(out);

		while (source < sourceEnd) {
			UTF32 ch = 0;
			unsigned short extraBytesToRead = trailingBytesForUTF8[(uint8_t)*source];
			if (extraBytesToRead >= sourceEnd - source) {
				break;
			}
			/* Do this check whether lenient or strict */
			if (!isLegalUTF8(source, extraBytesToRead + 1)) {
				break;
			}
			/*
			* The cases all fall through. See "Note A" below.
			*/
			switch (extraBytesToRead) {
			case 5: ch += (uint8_t)*source++; ch <<= 6; /* remember, illegal UTF-8 */
			case 4: ch += (uint8_t)*source++; ch <<= 6; /* remember, illegal UTF-8 */
			case 3: ch += (uint8_t)*source++; ch <<= 6;
			case 2: ch += (uint8_t)*source++; ch <<= 6;
			case 1: ch += (uint8_t)*source++; ch <<= 6;
			case 0: ch += (uint8_t)*source++;
			}
			ch -= offsetsFromUTF8[extraBytesToRead];

			if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
									 /* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
					*target++ = '?';
				}
				else {
					*target++ = (UTF16) ch; /* normal case */
				}
			}
			else if (ch > UNI_MAX_UTF16) {
				*target++ = '?';
			}
			else {
				ch -= halfBase;
				*target++ = (UTF16) ((ch >> halfShift) + UNI_SUR_HIGH_START);
				*target++ = (UTF16) ((ch & halfMask) + UNI_SUR_LOW_START);
			}
		}

		return out;
	}

	std::string narrowed(const std::u16string& src) {
		std::string out;
		auto source = src.begin();
		auto sourceEnd = src.end();
		auto target = std::back_inserter(out);

		while (source < sourceEnd) {
			UTF32 ch;
			unsigned short bytesToWrite = 0;
			const UTF32 byteMask = 0xBF;
			const UTF32 byteMark = 0x80;
			auto oldSource = source; /* In case we have to back up because of target overflow. */
			ch = *source++;
			/* If we have a surrogate pair, convert to UTF32 first. */
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
				/* If the 16 bits following the high surrogate are in the source buffer... */
				if (source < sourceEnd) {
					UTF32 ch2 = *source;
					/* If it's a low surrogate, convert to UTF32. */
					if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
						ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
							+ (ch2 - UNI_SUR_LOW_START) + halfBase;
						++source;
					}
				}
				else { /* We don't have the 16 bits following the high surrogate. */
					--source; /* return to the high surrogate */
					break;
				}
			}

			/* Figure out how many bytes the result will require */
			if (ch < (UTF32) 0x80) {
				bytesToWrite = 1;
			}
			else if (ch < (UTF32) 0x800) {
				bytesToWrite = 2;
			}
			else if (ch < (UTF32) 0x10000) {
				bytesToWrite = 3;
			}
			else if (ch < (UTF32) 0x110000) {
				bytesToWrite = 4;
			}
			else {
				bytesToWrite = 3;
				ch = '?';
			}

			UTF8 mid[4];
			UTF8* midp = mid + sizeof(mid);
			switch (bytesToWrite) { /* note: everything falls through. */
			case 4: *--midp = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
			case 3: *--midp = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
			case 2: *--midp = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
			case 1: *--midp = (UTF8) (ch | firstByteMark[bytesToWrite]);
			}
			for (int i = 0; i < bytesToWrite; ++i)
				*target++ = *midp++;
		}
		return out;
	}

	const char* next_char(const char* src)
	{
		if (!src)
			return src;

		auto trailing = trailingBytesForUTF8[(uint8_t)*src];
		return src + 1 + trailing;
	}
}
