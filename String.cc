/*
	String
	
	String experimentation
	
	2019/04/15	Originated
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#include <string>



/*	SlashTerminate
	Return the given string guaranteed with a trailing slash	
*/
std::wstring SlashTerminate(
	const wchar_t	homeSetPath[]
	)
{
std::wstring result = homeSetPath;

// not terminated by a slash?
if (result.empty() || result.back() != L'/')
	// append the slash
	result.push_back(L'/');

return result;
}
