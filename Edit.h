/*
	Edit
	
	Editing iCalendar items
	
	2023/04/22	Originated
	
	Copyright © 2023 by: Ben Hekster
*/

#pragma once

#include "Dynamic.h"


extern void ApplyEditsToCalendarItem(
	DynamicCalendar<wchar_t> &calendarItem,
	int		argc,
	const wchar_t	*argv[]
	);
