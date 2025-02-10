/*
	String
	
	String experimentation
	
	2019/04/15	Originated
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

#include <assert.h>
#include <stdio.h>

#include <format>
#include <functional>



/*	FormatString
	Apply std::format_string to format a string on the stack
	*** sometimes (always?) a string_view would obviate the strlen()
*/
template <typename Callable, typename... Ts>
void FormatString(
	std::wformat_string<Ts...> format,
	Callable	F,
	Ts		&&...args
	)
{
// compute required size of formatted string
const size_t length = std::formatted_size(format, args...);

// allocate stack space for string
wchar_t *const string = static_cast<wchar_t*>(alloca((length + 1) * sizeof(wchar_t)));

// format string and NUL-terminate
*std::format_to(string, format, args...) = L'\0';

// provide string to callback
F(string);
}


std::wstring SlashTerminate(const wchar_t[]);
