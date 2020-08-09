/*
 * Copyright 2012-2016 Moritz Hilscher
 *
 * This file is part of Mapcrafter.
 *
 * Mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "other.h"

#include "../config.h"

#include <cctype>

#ifdef HAVE_ENDIAN_H
# ifdef ENDIAN_H_FREEBSD
#  include <sys/endian.h>
# else
#  include <endian.h>
# endif
#endif

namespace mapcrafter {
namespace util {

bool isBigEndian() {
	int test = 1;
	return *((char*) &test) != 1;
}

int16_t swapEndian16(int16_t x) {
	return ((x & 0xff00) >> 8) |
		   ((x & 0xff) << 8);
}

int32_t swapEndian32(int32_t x) {
	return ((x & 0xff000000) >> 24) |
		   ((x & 0xff0000) >> 8) |
		   ((x & 0xff00) << 8) |
		   ((x & 0xff) << 24);
}

int64_t swapEndian64(int64_t x) {
	return ((x & 0xff00000000000000) >> 56) |
		   ((x & 0xff000000000000) >> 40) |
		   ((x & 0xff0000000000) >> 24) |
		   ((x & 0xff00000000) >> 8) |
		   ((x & 0xff000000) << 8) |
		   ((x & 0xff0000) << 24) |
		   ((x & 0xff00) << 40) |
		   ((x & 0xff) << 56);
}

#ifndef HAVE_ENDIAN_H
static bool IS_BIG_ENDIAN = isBigEndian();
#endif

int16_t bigEndian16(int16_t x) {
#ifdef HAVE_ENDIAN_H
	return htobe16(x);
#else
	if (!IS_BIG_ENDIAN)
		return swapEndian16(x);
	return x;
#endif
}

int32_t bigEndian32(int32_t x) {
#ifdef HAVE_ENDIAN_H
	return htobe32(x);
#else
	if (!IS_BIG_ENDIAN)
		return swapEndian32(x);
	return x;
#endif
}

int64_t bigEndian64(int64_t x) {
#ifdef HAVE_ENDIAN_H
	return htobe64(x);
#else
	if (!IS_BIG_ENDIAN)
		return swapEndian64(x);
	return x;
#endif
}

// nicer bool -> string conversion
template <>
std::string str<bool>(bool value) {
	return value ? "true" : "false";
}

/**
 * Overwrites the string stream solution for string -> string conversion.
 * Why? Converting the string 'This is a test.' would just result in 'This'
 */
template <>
std::string as<std::string>(const std::string& from) {
	return from;
}

/**
 * Same thing with path -> string conversion.
 */
template <>
fs::path as<fs::path>(const std::string& from) {
	return fs::path(from);
}

template <>
bool as<bool>(const std::string& from) {
	if (from == "true" || from == "1")
		return true;
	if (from == "false" || from == "0")
		return false;
	throw std::invalid_argument("Must be one of true/false or 0/1");
}

bool isHexNumber(const std::string& str) {
	for (size_t i = 0; i < str.size(); i++)
		if (!isxdigit(str[i]))
			return false;
	return true;
}

unsigned int parseHexNumber(const std::string& str) {
	std::stringstream ss;
	ss << std::hex << str;

	unsigned int x;
	ss >> x;
	return x;
}

std::string trim(const std::string& str) {
	// removes trailing and leading whitespaces
	std::string trimmed = str;
	size_t end = trimmed.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		trimmed = trimmed.substr(0, end+1);
	size_t start = trimmed.find_first_not_of(" \t\r\n");
	if (start != std::string::npos)
		trimmed = trimmed.substr(start);
	else if (trimmed.find_first_of(" \t\r\n") != std::string::npos)
		// special case if all characters are whitespaces
		trimmed = "";
	return trimmed;
}

// http://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c

std::string escapeJSON(const std::string& str) {
	std::ostringstream ss;
	for (auto it = str.begin(); it != str.end(); ++it) {
	//C++98/03:
	//for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		switch (*it) {
			case '\\': ss << "\\\\"; break;
			case '"': ss << "\\\""; break;
			case '/': ss << "\\/"; break;
			case '\b': ss << "\\b"; break;
			case '\f': ss << "\\f"; break;
			case '\n': ss << "\\n"; break;
			case '\r': ss << "\\r"; break;
			case '\t': ss << "\\t"; break;
			default: ss << *it; break;
		}
	}
	return ss.str();
}

std::string capitalize(const std::string& str) {
	if (str.empty())
		return "";
	std::string capitalized = str.substr(1);
	capitalized.insert(capitalized.begin(), toupper(str[0]));
	return capitalized;
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
	std::string replaced = str;
	size_t start = 0;
	while ((start = replaced.find(from, start)) != std::string::npos) {
		replaced.replace(start, from.length(), to);
		start += to.length();
	}
	return replaced;
}

bool startswith(const std::string& str, const std::string& start) {
	if (start.size() > str.size())
		return false;
	return str.substr(0, start.size()) == start;
}

bool endswith(const std::string& str, const std::string& end) {
	if (end.size() > str.size())
		return false;
	return str.substr(str.size() - end.size(), end.size()) == end;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> splitted;

	std::istringstream ss(str);
	for (std::string token; std::getline(ss, token, delimiter);) {
		splitted.push_back(token);
	}

	return splitted;
}

std::map<std::string, std::string> parseProperties(std::string str) {
	std::map<std::string, std::string> properties;

	// '-' stands for no properties
	if (str == "-") {
		return properties;
	}

	std::vector<std::string> parts = util::split(str, ',');
	for (auto it = parts.begin(); it != parts.end(); ++it) {
		if (*it == "") {
			continue;
		}
		size_t index = it->find('=');
		assert(index != std::string::npos);
		std::string key = it->substr(0, index);
		std::string value = it->substr(index + 1);
		properties[key] = value;
	}
	return properties;
}

} /* namespace util */
} /* namespace mapcrafter */
