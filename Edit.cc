/*
	Edit
	
	Editing iCalendar items
	
	2023/04/22	Originated
	
	Copyright © 2023 by: Ben Hekster
	
	Paragraph references refer to RFC 5545
*/

#include "Dynamic.h"
#include "Edit.h"


using namespace std::string_literals;


/*	Get1ToDo

*/
static DynamicCalendar<wchar_t>::ToDo &Get1ToDo(
	DynamicCalendar<wchar_t> &item
	)
{
DynamicCalendar<wchar_t>::ToDo *todo = nullptr;

// make sure there is exactly one To-Do component
for (auto &component: item.fComponents)
	if (std::holds_alternative<DynamicCalendar<wchar_t>::ToDo>(component)) {
		if (todo) throw "expected exactly one to-do component in calendar item";
		
		todo = &std::get<DynamicCalendar<wchar_t>::ToDo>(component);
		}
if (!todo) throw "no to-do component in calendar item";

return *todo;
}


/*	ApplyDueDateTime
	ApplyDueDate
	Set Due property of To-Do Component as per §3.8.2.3
*/
static void ApplyDueDateTime(
	DynamicCalendar<wchar_t> &item,
	const wchar_t	arg[]
	)
{
Get1ToDo(item).fDue = DynamicCalendar<wchar_t>::Parser::ParseDateTime(arg);
}

static void ApplyDueDate(
	DynamicCalendar<wchar_t> &item,
	const wchar_t	arg[]
	)
{
Get1ToDo(item).fDue = DynamicCalendar<wchar_t>::Parser::ParseDate(arg);
}


/*	DeleteDueTimeZoneIdentifier
	This should probably accompany a change to the due date to have UTC format
*/
static void DeleteDueTimeZoneIdentifier(
	DynamicCalendar<wchar_t> &item
	)
{
// clear the Due property time zone identifier parameter string
Get1ToDo(item).fDueTimeZoneID.clear();
}


/*	DeleteComponentTimeZone
	Delete any Time Zone components
*/
static void DeleteComponentTimeZone(
	DynamicCalendar<wchar_t> &item
	)
{
// remove-erase all Time Zone components
item.fComponents.erase(
	std::remove_if(
		item.fComponents.begin(), item.fComponents.end(),
		[](DynamicCalendar<wchar_t>::ComponentVariant &component) {
			return std::holds_alternative<DynamicCalendar<wchar_t>::TimeZone>(component);
			}
		),
	item.fComponents.end()
	);
}


/*	ApplySummary

*/
static void ApplySummary(
	DynamicCalendar<wchar_t> &item,
	const wchar_t	arg[]
	)
{
// assume for now the calendar item has only one calendar component
if (item.fComponents.size() != 1) throw "expected exactly 1 component in calendar item";
DynamicCalendar<wchar_t>::ComponentVariant &component = item.fComponents[0];

/* This is a compile-time construct; the 'auto' lambda is instantiated once for each of
   the variants; and the 'constexpr' selects the variants that have a summary.
   The alternative would be a run-time construct; to get the equivalent behavior
   I could check for the specific variants; but this isn't future-proof against new
   variants.  The equivalent is to use a dynamic cast (which is analogous to is_base_of<>)
   but now we're duplicating type information in the variant in RTTI. */
std::visit(
	[&arg](auto &c) {
		// only certain component types have a 'summary'
		if constexpr (std::is_base_of_v<DynamicCalendar<wchar_t>::Component, std::remove_reference_t<decltype(c)>>)
			c.fSummary = arg;
					
		else
			throw "can't set summary on this type of calendar component";
		},
	component
	);
}


/*	ApplyEditsToCalendarItem
	Apply changes to a calendar item on a CalDAV server
*/
void ApplyEditsToCalendarItem(
	DynamicCalendar<wchar_t> &calendarItem,
	int		argc,
	const wchar_t	*argv[]
	)
{
// no-argument commands
const static struct Command0 {
	const wchar_t	*name;
	void		(*action)(DynamicCalendar<wchar_t>&);
	} commands0[] = {
	{ L"delete-due-tzid", DeleteDueTimeZoneIdentifier },
	{ L"delete-timezone", DeleteComponentTimeZone }
	};

// single-argument commands
const static struct Command1 {
	const wchar_t	*name;
	void		(*action)(DynamicCalendar<wchar_t>&, const wchar_t arg[]);
	} commands1[] = {
	{ L"due", ApplyDueDateTime },
	{ L"due-datetime", ApplyDueDateTime },
	{ L"due-date", ApplyDueDate },
	{ L"summary", ApplySummary }
	};

// while there are more editing commands
for (; argc > 0; --argc, ++argv)
	// find zero-argument editing command
	if (
		const Command0 *const command = std::find_if(
			std::begin(commands0), std::end(commands0),
			[arg = *argv](const auto &c) { return wcscmp(c.name, arg) == 0 ; }
			);
		command != std::end(commands0)
		)
		// perform zero-argument editing command
		command->action(calendarItem);
	
	// find one-argument editing command
	else if (
		const Command1 *const command = std::find_if(
			std::begin(commands1), std::end(commands1),
			[arg = *argv](const Command1 &c) { return wcscmp(c.name, arg) == 0; }
			);
		command != std::end(commands1)
		) {
		if (! (argc >= 1)) throw "editing command needs argument";
		
		// perform editing command
		command->action(calendarItem, (--argc, *++argv));
		}
	
	else
		throw "unknown editing command";

#if 0
// create DTSTAMP if not present (*** should be set whenever an update is made)
if (DynamicCalendar<wchar_t>::ToDo *const toDo = std::get_if<DynamicCalendar<wchar_t>::ToDo>(&calendarItem.fComponents[0]))
	if (!toDo->fStamp)
		toDo->fStamp = DynamicCalendar<wchar_t>::DateTime::MakeForNowUTC();
#endif
}
