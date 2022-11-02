#pragma once

#include <vector>
#include <string.h>

//--- Split Function ---//
inline std::vector<std::string> Split(const std::string& in_str, const std::string& in_delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = in_str.find(in_delim, prev);
		if(pos == std::string::npos)
			pos = in_str.length();
		std::string token = in_str.substr(prev, pos - prev);
		if(!token.empty())
			tokens.push_back(token);
		prev = pos + in_delim.length();
	} while(pos < in_str.length() && prev < in_str.length());
	return tokens;
}
inline std::string Trim(std::string in_str)
{
	in_str.erase(in_str.begin(), std::find_if(in_str.begin(), in_str.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	in_str.erase(std::find_if(in_str.rbegin(), in_str.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), in_str.end());
	return in_str;
}
