/*
	main
 
	Command-line entry point
 	for Management

	2018/09/09	Originated
 
	Copyright © 2018-2023 by: Ben Hekster
*/

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <io.h>
#include <fcntl.h>

#include <algorithm>
#include <codecvt>

#include "Dynamic.h"
#include "Edit.h"
#include "ServiceLocation.h"
#include "Session.h"



/*	CreateCalendar
	Make a calendar collection
*/
static void CreateCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// command subarguments
if (argc != 2) throw "create-calendar path name";
const wchar_t
	*const calendarPath = (--argc, *argv++),
	*const calendarName = (--argc, *argv++);

session.CreateCalendar(calendarPath, calendarName);
}


/*	DeleteCalendar
	Delete a calendar collection (actually, deletes anything at the path)
*/
static void DeleteCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// calendar name
if (argc != 1) throw "delete-calendar path";
const wchar_t *const calendarPath = (--argc, *argv++);

session.DeleteCalendar(calendarPath);
}


/*	RenameCalendar
	Rename a calendar collection
*/
static void RenameCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// calendar path and new name
if (argc != 2) throw "rename-calendar path name";
const wchar_t
	*const calendarPath = (--argc, *argv++),
	*const calendarName = (--argc, *argv++);

session.RenameCalendar(calendarPath, calendarName);
}


/*	ExportCalendar
	Export a calendar
*/
static void ExportCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// calendar name
if (argc != 1) throw "export-calendar path";
const wchar_t *const calendarPath = (--argc, *argv++);

session.ExportCalendar(calendarPath);
}


/*	SynchronizeCalendar
	Synchronize calendar using a synchronization token
*/
static void SynchronizeCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// calendar name
if (argc < 1 || argc > 2) throw "export-calendar path [token]";
const wchar_t
	*const calendarPath = (--argc, *argv++),
	*const token = argc > 0 ? (--argc, *argv++) : nullptr;

session.SynchronizeCalendar(calendarPath, token);
}


/*	QueryCalendar
	Query a calendar
*/
static void QueryCalendar(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// calendar name
if (argc != 1) throw "query-calendar path";
const wchar_t *const calendarPath = (--argc, *argv++);

session.QueryCalendar(calendarPath);
}


/*	ListCalendars
	Print on standard output a list of all the calendars provided by the service
*/
static void ListCalendars(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
if (argc != 0) throw "list-calendars";

session.ListCalendars();
}


/*	ListCalendarItems
	List all the items of a named calendar
*/
static void ListCalendarItems(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// command subarguments
if (argc != 1) throw "list-items calendar-path";
const wchar_t *const calendarPath = (--argc, *argv++);

session.ListCalendarItems(calendarPath);
}


/*	ReadItems
	Print the raw, unintepreted content of the named calendar items
*/
static void ReadItems(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc > 0; --argc, argv++)
	// get the corresponding calendar item
	session.ReadItem(*argv);
}


/*	WriteItems
	Store the raw, unintepreted content of the named calendar items
*/
static void WriteItems(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc >= 2; --argc, argv += 2)
	session.WriteItem(argv[0], argv[1]);
}


/*	ReadItemsProperties
	Print the raw, uninterpreted content of the named calendar items' properties
*/
static void ReadItemsProperties(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc > 0; --argc, argv++)
	session.ReadItemProperties(*argv);
}


/*	ReadItemsPropertyNames
	Print the calendar items' properties
*/
static void ReadItemsPropertyNames(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc > 0; --argc, argv++)
	session.ReadItemPropertyNames(*argv);
}


/*	ReadCalendarItems
	Print the parsed interpretation of the named calendar items
*/
static void ReadCalendarItems(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc > 0; --argc, argv++)
	// print parsed calendar item to standard output
	std::wcout << session.ReadCalendarItemFromCalDAV(*argv);
}


/*	WriteCalendarItems
	Write the named calendar items from a parsed interpretation
*/
static void WriteCalendarItems(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
// for each URL argument
for (; argc >= 2; --argc, argv += 2)
	session.WriteCalendarItem(argv[0], argv[1]);
}


/*	EditCalendarItem
	Apply changes to a calendar item on a CalDAV server
*/
static void EditCalendarItem(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
if (argc < 1) throw "edit-cal-items: path [commands...]";
const wchar_t *const path = (--argc, *argv++);

// read from file
DynamicCalendar<wchar_t> calendarItem = session.ReadCalendarItemFromCalDAV(path);

// apply editing commands to calendar item
ApplyEditsToCalendarItem(calendarItem, argc, argv);

// write back
session.WriteCalendarItemToCalDAV(path, calendarItem);
std::wcout << calendarItem;
}


/*	SupportedReportSet
	Print names of supported reports
*/
static void SupportedReportSet(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
if (argc != 1) throw "supported-report-set: path";
const wchar_t *const path = (--argc, *argv++);

session.SupportedReportSet(path);
}


/*	SupportedCollationSett
	Print names of supported collations
*/
static void SupportedCollationSett(
	Session		&session,
	int		argc,
	const wchar_t	*argv[]
	)
{
if (argc != 1) throw "supported-collation-set: path";
const wchar_t *const path = (--argc, *argv++);

session.SupportedCollationSett(path);
}


/*	main
	Command-line entry point
*/
int wmain(
	int		argc,
	const wchar_t	*argv[]
	)
{
/* BTW, the documentation is correct and you actually _will_ get a run-time assertion if you've previously
   done narrow-character output through std::cout and then attempt wide-character output through std::wcout.
   So pick one, and stick with it consistently. */
/* This is needed for the Console to work; appears to be orthogonal to whatever iostreams is doing. */
// do console I/O in UTF-16
_setmode(_fileno(stdin), _O_U16TEXT);
_setmode(_fileno(stdout), _O_U16TEXT);
_setmode(_fileno(stderr), _O_U16TEXT);

// create the global locale for external I/O
std::locale gLocale(
	std::wcout.getloc(),
		
	// *** codecvt_utf16 is deprecated; maybe take wnstreambuf and move it into a custom codecvt
	// ***** remember to turn SDL back on
	/* previous comment: "generate_header is needed to avoid the weird UCS4 expansion";
	   however, this produces a visible BOM on console output.  My guess is that the double
	   UCS4 expansion happened because of what I now understand PowerShell does with $OutputEncoding */
	new std::codecvt_utf16<
		wchar_t,
		0x10ffff,
		static_cast<std::codecvt_mode>(std::little_endian)
		>
	);
std::wcout.imbue(gLocale);

try {
	// must have at least all required command-line arguments
	if (argc < 5) throw "caldavutil hostname username password command";
	
	// skip over executable name
	--argc, argv++;
	
	// extract required command-line arguments
	const wchar_t
		*const hostname = (--argc, *argv++),
		*const username = (--argc, *argv++),
		*const password = (--argc, *argv++);
	
	
	/*
	
		commands that need service location, but not connection
	
	*/
	
	std::optional<DAVServiceLocation> locationp = DAVServiceLocation::Locate(
		DAVServiceLocation::kCalDAVSecure, L"tcp",
		hostname
		);
	if (!locationp) throw "unable to locate service";
	DAVServiceLocation &location = *locationp;
	
	// locate service?
	if (std::wcscmp(*argv, L"location") == 0) {
		// print the resolved location of the CalDAV service
		std::wcout << location.fHost << ':' << location.fPort << location.fPath << '\n';
		}
	
	
	/*
	
		commands that need service connection
	
	*/
	
	else {
		const static struct Command {
			const wchar_t	*name;
			void		(*action)(Session&, int argc, const wchar_t *argv[]);
			} commands[] = {
			{ L"create-calendar", CreateCalendar },
			{ L"delete-calendar", DeleteCalendar },
			{ L"rename-calendar", RenameCalendar },
			{ L"export-calendar", ExportCalendar },
			{ L"synchronize-calendar", SynchronizeCalendar },
			{ L"query-calendar", QueryCalendar },
			{ L"list-calendars", ListCalendars },
			{ L"list-items", ListCalendarItems },
			{ L"read-items", ReadItems },
			{ L"read-items-properties", ReadItemsProperties },
			{ L"read-items-property-names", ReadItemsPropertyNames },
			{ L"write-items", WriteItems },
			{ L"read-cal-items", ReadCalendarItems },
			{ L"write-cal-items", WriteCalendarItems },
			{ L"edit-cal-item", EditCalendarItem },
			{ L"supported-report-set", SupportedReportSet },
			{ L"supported-collation-set", SupportedCollationSett }
			};
		
		// find command to perform
		const Command *const command = std::find_if(
			std::begin(commands), std::end(commands),
			[arg = *argv](const Command &c) { return wcscmp(c.name, arg) == 0; }
			); if (command == std::end(commands)) throw "unknown command";
		--argc, argv++;
		
		// resolve the DNS address
		CHTTPClient::Address address(true, location.fHost.c_str(), location.fPort);
		
		// connect to the service
		Session session = Session::MakeFromServiceLocation(
			address,
			location.fPath.c_str(),
			username, password
			);
		
		// perform command
		(*command->action)(session, argc, argv);
		}
	}

catch (const std::exception &error) {
	std::wcerr << "error: " << error.what() << std::endl;
	}

catch (const char error[]) {
	std::wcerr << "error: " << error << std::endl;
	}

catch (const unsigned long error) {
	std::wcerr << "error " << error << std::endl;
	}

catch (...) {
	std::wcerr << "error" << std::endl;
	}

return 0;
}
