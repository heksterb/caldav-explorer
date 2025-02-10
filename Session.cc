/*
	Session
	
	Calendar DAV session
	
	2019/05/02	Originated
	2023/05/21	Factored out session management
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#include <assert.h>

#include <fstream>
#include <utility>

#include "AdaptableStreamBuffer.h"
#include "Session.h"
#include "String.h"
#include "Synchronization.h"
#include "WebDAV.h"



/*	wfopen_s
	Like CRT _wfopen_s but returns result as function return; throws exception on failure
	*** move this somewhere in Native
*/
static FILE *wfopen_s(
	const wchar_t	*const name,
	const wchar_t	*const mode
	)
{
FILE *result;

// try to open file
if (_wfopen_s(&result, name, mode) != 0) {
	const static char prefix[] = "can't open file";
	
	// try to get system error message
	char message[80 /* doesn't seem to be a way to determine this; this number used in documentation */];
	static_assert(sizeof prefix - 1 < 94); // as per documentation
	if (_strerror_s(message, sizeof message, prefix) == 0) {
		// trim trailing '\n'
		if (
			const size_t messageL = strlen(message);
			messageL > 0
			)
			/* Documentation claims the message will be NL-terminated; but I don't observe that in practice */
			if (
				char &last = message[messageL - 1];
				last == '\n'
				)
				last = '\0';
		
		throw std::runtime_error(message);
		}
	
	else
		// just throw static message
		throw prefix;
	}

return result;
}



/*

	Session

*/

/*	FindPrincipalPath
	Obtain the user "principal path"
*/
std::wstring Session::FindPrincipalPath(
	CHTTPClient	&client,
	const wchar_t	contextPath[]
	)
{
std::wstring result;

// input argument (presumably from DNS autolocation
if (contextPath[0])
	try { result = CalDAV::GetPrincipalPath(client, contextPath); } catch (...) {}

if (result.empty()) // *** ONLY if the previous step returned HTTP 404
	try { result = CalDAV::GetPrincipalPath(client, L"/"); } catch (...) {}

if (result.empty()) throw "can't find user principal path";

return result;
}


/*	Split
	Split a URL into an optional host address
	
	CalDAV uses both a 'current user principal' and a 'calendar home set' URL which may,
	or may not, indicate a different server.  We like to clearly distinguish these cases
	so that we can reuse server connections when possible.
*/
void Session::Split(
	const wchar_t	url[],
	const std::function<void (const wchar_t path[])> &SameHost,
	const std::function<void (const CHTTPClient::Address&, const wchar_t path[])> &DifferentHost
	)
{
// crack open the URL
URL_COMPONENTS components;
components.dwStructSize = sizeof components;
components.lpszScheme = nullptr;		// return scheme inside URL string
components.dwSchemeLength = ~0;
components.lpszHostName = nullptr;		// return host name inside URL string
components.dwHostNameLength = ~0;
components.lpszUserName = nullptr;		// don't need user name (not expecting to use a different one)
components.dwUserNameLength = 0;
components.lpszPassword = nullptr;		// don't need password (not expecting to use a different one)
components.dwPasswordLength = 0;
components.lpszUrlPath = nullptr;		// return path inside URL string
components.dwUrlPathLength = ~0;
components.lpszExtraInfo = nullptr;		// don't think I need 'extra' information (query or anchor part of URL)
components.dwExtraInfoLength = 0;
BOOL success = WinHttpCrackUrl(url, 0 /* NUL-terminated */, 0 /* flags */, &components);

std::wstring_view scheme, hostname, path;
unsigned short port = 0;

// able to crack the URL?
if (success) {
	if (components.lpszScheme) scheme = std::wstring_view(components.lpszScheme, components.dwSchemeLength);
	if (components.lpszHostName) hostname = std::wstring_view(components.lpszHostName, components.dwHostNameLength);
	if (components.lpszUrlPath) path = std::wstring_view(components.lpszUrlPath, components.dwUrlPathLength);
	if (components.nPort) port = components.nPort;
	}

else
	switch (const DWORD error = GetLastError()) {
		// no scheme in URL?
		case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
			// then assume same host, and the string is just the path
			// *** might not be true, perhaps the URL is a host and path
			path = url;
			break;
		
		default:
			throw error;
		}

// no different hostname found?
if (hostname.empty())
	SameHost(std::wstring(path).c_str());

// found a hostname, implying we need to connect to a different server?
else
	DifferentHost(
		CHTTPClient::Address(
			scheme != L"http",			// assume HTTPS, unless we definitively found "http"
			std::wstring(hostname).c_str(),
			port					// zero indicates to use appropriate default
			),
		std::wstring(path).c_str()
		);
}


/*	MakeFromServiceLocation
	Create a CalDAV service session at the given (possibly service-located) server address
*/
Session Session::MakeFromServiceLocation(
	CHTTPClient::Address &address,
	const wchar_t	contextPath[],
	const wchar_t	username[],
	const wchar_t	password[]
	)
{
return MakeServiceFromContext(
	CHTTPClient(address, username, password),
	contextPath,
	username, password
	);
}


/*	MakeServiceFromContext
	Makes the per-user 'principal' CalDAV service
	
	To get this, we need to first establish the initial CalDAV service at the 'root'
	of the server as defined by its 'contextPath'.	This initial service can only be used
	(as far as I know) to obtain the principal CalDAV service which is what the caller
	interacts with.  Since I don't know of any other purpose of the initial 'context' service,
	I don't bother exposing it either as a separate type or a separate instance.
	
	Note that the principal CalDAV service may or may not exist on a completely different server.
	In the case that it's the same server, we take care to reuse the HTTP session
	already established for the 'context' service.
*/
Session Session::MakeServiceFromContext(
	CHTTPClient	&&contextService,
	const wchar_t	contextPath[],
	const wchar_t	username[],
	const wchar_t	password[]
	)
{
std::optional<Session> result;

Split(
	/* Sometimes, the returned principal path is a relative path;
	   with iCloud, it is a full URL pointing to a completely different server.
	   Update: maybe I'm mistaken?  Now this is just a path, and only the
	   calendar home set URL is on a different server? */
	FindPrincipalPath(contextService, contextPath).c_str(),
	
	// on same server?
	[&](const wchar_t principalPath[]) {
		result.emplace(
			MakeServiceFromPrincipal(
				std::move(contextService),
				principalPath,
				username, password
				)
			);
		},
	
	// on different server?
	[&](const CHTTPClient::Address &principalHostAddress, const wchar_t principalPath[]) {
		// create a client for the new server containing the principal path
		result.emplace(
			MakeServiceFromPrincipal(
				CHTTPClient(principalHostAddress, username, password),
				principalPath,
				username, password
				)
			);
		}
	); assert(result);

return std::move(*result);
}


Session Session::MakeServiceFromPrincipal(
	CHTTPClient	&&principalServer,
	const wchar_t	principalPath[],
	const wchar_t	username[],
	const wchar_t	password[]
	)
{
std::optional<Session> result;

Split(
	/* In iCloud, the principal path is just a path on the same server; but the calendar home set path is on a different server */
	CalDAV::GetCalendarHomeSet(principalServer, principalPath).c_str(),
	
	// on same server?
	[&](const wchar_t homeSetPath[]) {
		result.emplace(
			std::move(principalServer),
			homeSetPath
			);
		},
	
	// on different server?
	[&](const CHTTPClient::Address &homeSetHostAddress, const wchar_t homeSetPath[]) {
		// create a client for the new server
		result.emplace(
			CHTTPClient(homeSetHostAddress, username, password),
			homeSetPath
			);
		}
	); assert(result);

return std::move(*result);
}


/*	Session
	Construct CalDAV client from an already existing HTTP connection to the server
*/
Session::Session(
	CHTTPClient	&&homeSetServer,
	const wchar_t	homeSetPath[]
	) :
	fClient(std::move(homeSetServer)),
	fHomeSetPath(SlashTerminate(homeSetPath))
{
/* None of the following is required anywhere; but it seems like a client might use this information.
   We're just doing this to discover new values. */

// get server options
DAV::GetServerOptions(
	fClient,
	homeSetPath,
	[this](const char allow[], const char dav[]) {
		fHomeSetAllow = DAV::Allow(allow);
		fHomeSetCapabilities = DAV::Capabilities(dav);
		}
	);

// get reports supported at the home set
WebDAV::Find::Properties(
	fClient,
	homeSetPath,
	WebDAV::Depth::zero,
	
	WebDAV::Response(),
	
	VersioningDAV::SupportedReportSet(
		[this](const wchar_t reportName[]) { fHomeSetSupportedReports.Add(reportName); }
		)
	);
}


/*	Session
	Move constructor
*/
Session::Session(
	Session		&&that
	) noexcept :
	fClient(std::move(that.fClient)),
	fHomeSetPath(std::move(that.fHomeSetPath)),
	fHomeSetAllow(that.fHomeSetAllow),
	fHomeSetCapabilities(that.fHomeSetCapabilities),
	fHomeSetSupportedReports(that.fHomeSetSupportedReports)
{
}


/*	ListItems
	Return a vector of paths to each of the given named calendar's items
*/
std::vector<std::wstring> Session::ListItems(
	const wchar_t	name[]
	)
{
std::vector<std::wstring> itemPaths;

// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this, &itemPaths](const wchar_t path[]) {
		// get list of items in collection
		WebDAV::Find::Properties(
			fClient,
			path,
			DAV::Depth::one,
			WebDAV::Response(
				WebDAV::HREF(
					[path, &itemPaths](const wchar_t itemPath[]) {
						/* iCloud also returns the "collection" path here; Horde apparently does not. */
						if (wcscmp(itemPath, path) != 0) itemPaths.push_back(itemPath);
						}
					)
				)
			);
		},
	fHomeSetPath,
	name
	);

return itemPaths;
}


/*	ExportCalendarIndividually
	Export a calendar collection by first obtaining a list of its immediate children,
	then getting the calendar data of each individually through HTTP GET
	
	There is probably no reason to prefer this over the CalDAV calendar-multiget REPORT
	used by ExportCalendarMultiply()
*/
void Session::ExportCalendarIndividually(
	const wchar_t	name[],
	const std::function<void (std::wistream&)> &Recipient
	)
{
// for each of the calendar's items
for (const std::wstring &itemPath: ListItems(name))
	// get the item 
	CalDAV::GetItem(fClient, itemPath.c_str(), Recipient);
}


/*	ExportCalendarMultiply
	Export a calendar collection by first obtaining a list of its immediate children,
	then getting the calendar data of each in bulk through HTTP REPORT
*/
void Session::ExportCalendarMultiply(
	const wchar_t	name[]
	)
{
// can use the home set path, given that the item paths are already exact
/* If the Request-URI is a collection resource,
   then the DAV:href elements MUST refer to calendar object resources
   within that collection, and they MAY refer to calendar object
   resources at any depth within the collection. */
CalDAV::MultiGet::Properties(
	fClient,
	fHomeSetPath.c_str(), DAV::Depth::zero,
	ListItems(name),
	WebDAV::Response<> {},
	CalDAV::CalendarData(
		[](const wchar_t content[]) {
			std::wcout << content << L'\n';
			}
		)
	);
}


/*	SynchronizeCalendar
	*** RFC says might be supported by arbitrary collection
*/
void Session::SynchronizeCalendar(
	const wchar_t	calendarPath[],
	const wchar_t	*const token
	)
{
// was it permitted by OPTIONS?
/* Calling this on Horde/SabreDAV where OPTIONS had not previously allowed this results in
   a hard close of the TCP connection. */
if (!fHomeSetSupportedReports.fSyncCollection) throw "sync-collection not permitted by supported-reports";

// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this, token](const wchar_t path[]) {
		// create calendar
		SynchronizationDAV::Perform(
			fClient,
			path,
			
			// supplied and returned token
			token,
			[](const wchar_t token[]) {
				std::wcout << token << '\n';
				},
			
			WebDAV::Response {},
			
			// query for and respond to 'etag'
			WebDAV::Find::ETag(
				[](const wchar_t characters[]) {
					std::wcout << characters << '\n';
					}
				)
			);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	ReadCalendarItemFromCalDAV
	Read the calendar item at the given path on the server
*/
DynamicCalendar<wchar_t> Session::ReadCalendarItemFromCalDAV(
	const wchar_t	path[]
	)
{
DynamicCalendar<wchar_t> result;

// get the corresponding calendar item
CalDAV::GetItem(
	fClient,
	path,
	[&result](std::wistream &is) {
		// parse it
		DynamicCalendar<wchar_t>::Parser(result, is)();
		}
	);

return result;
}


/*	WriteCalendarItemToCalDAV
	Write the given calendar item to a path on the server
*/
void Session::WriteCalendarItemToCalDAV(
	const wchar_t	path[],
	const DynamicCalendar<wchar_t> &calendarItem
	)
{
// set the corresponding calendar item
CalDAV::SetItem(
	fClient,
	path,
	[&calendarItem](std::wstreambuf &osb) {
		// our calendar interface can only write to an actual output stream
		std::wostream(&osb, true) << calendarItem;
		}
	);
}


/*	CreateCalendar
	Make a calendar collection
*/
void Session::CreateCalendar(
	const wchar_t	calendarPath[],
	const wchar_t	calendarName[]
	)
{
// was it permitted by OPTIONS?
/* Calling this on Horde/SabreDAV where OPTIONS had not previously allowed this results in
   a hard close of the TCP connection. */
if (!fHomeSetAllow.fMakeCollection) throw "MKCOL not permitted by OPTIONS";

// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this, calendarName](const wchar_t path[]) {
		// create calendar
		WebDAV::MakeCollection(fClient, path, calendarName);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	DeleteCalendar
	Delete a calendar collection (actually, deletes anything at the path)
*/
void Session::DeleteCalendar(
	const wchar_t	calendarPath[]
	)
{
// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this](const wchar_t path[]) {
		// delete
		DAV::Delete(fClient, path);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	RenameCalendar
	Rename a calendar collection
*/
void Session::RenameCalendar(
	const wchar_t	calendarPath[],
	const wchar_t	calendarName[]
	)
{
// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this, calendarName](const wchar_t path[]) {
		WebDAV::Patch::Properties(
			fClient,
			path, WebDAV::Depth::zero,
			WebDAV::Response {},
			WebDAV::Patch::Set(
				WebDAV::Patch::DisplayName {}
				),
			calendarName
			);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	ExportCalendar
	Export a calendar
*/
void Session::ExportCalendar(
	const wchar_t	calendarPath[]
	)
{
#if 0
ExportCalendarIndividually(
	calendarPath,
	[](std::wistream &is) {
		// print to standard output
		std::wcout << is.rdbuf();
		}
	);
		
#else
ExportCalendarMultiply(calendarPath);
		
#endif
}


/*	QueryCalendar
	Query a calendar
*/
void Session::QueryCalendar(
	const wchar_t	calendarPath[]
	)
{
// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this](const wchar_t path[]) {
		CalDAV::Query(
			fClient,
			path, WebDAV::Depth::one,
			"<D:prop>"
				"<D:getetag/>"
				"<C:calendar-data>"
					"<C:comp name='VCALENDAR'>"
						"<C:prop name='VERSION'/>"
					"</C:comp>"
				"</C:calendar-data>"
			"</D:prop>"
			"<C:filter>"
				"<C:comp-filter name='VCALENDAR'/>"
			"</C:filter>"
			);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	ListCalendars
	Print on standard output a list of all the calendars provided by the service
*/
void Session::ListCalendars()
{
const std::size_t homeSetPathL = fHomeSetPath.length();

// get all calendars
CalDAV::GetCalendars(
	fClient,
	fHomeSetPath.c_str(),
	[this, homeSetPathL](
		const std::wstring &pathString,
		const std::wstring &displayName
		) {
		std::wstring_view path(pathString);
		
		// remove 'home set' prefix
		assert(path.starts_with(fHomeSetPath));
		path.remove_prefix(homeSetPathL);
		
		// print path and name
		std::wcout << path << L" (" << displayName << L")\n";
		}
	);
}


/*	ListCalendarItems
	List all the items of a named calendar
*/
void Session::ListCalendarItems(
	const wchar_t	calendarPath[]
	)
{
// concatenate home set path with specified calendar path
FormatString(
	L"{}{}/",
	[this](const wchar_t path[]) {
		// list the items of that calendar
		std::wstring itemPath, itemETag, itemLastModified;
		WebDAV::Find::Properties(
			fClient,
			path, WebDAV::Depth::one,
			
			// handle standard WebDAV 'response'
			WebDAV::Response(
				WebDAV::Begin(
					[&]() {
						itemPath.erase();
						itemETag.erase();
						itemLastModified.erase();
						}
					),
				
				WebDAV::HREF(
					[&itemPath](const wchar_t characters[]) { itemPath = characters; }
					),
				
				WebDAV::End(
					[&]() {
						if (!itemETag.empty())
							std::wcout << itemPath << ", " << itemETag << ", " << itemLastModified << std::endl;
						}
					)
				),
			
			// query for and respond to 'etag'
			WebDAV::Find::ETag(
				[&itemETag](const wchar_t characters[]) { itemETag = characters; }
				),
			
			// query for and respond to 'getlastmodified'
			WebDAV::Find::LastModified(
				[&itemLastModified](const wchar_t characters[]) { itemLastModified = characters; }
				)
			);
		},
	fHomeSetPath,
	calendarPath
	);
}


/*	ReadItems
	Print the raw, unintepreted content of the named calendar item
*/
void Session::ReadItem(
	const wchar_t	path[]
	)
{
// get the corresponding calendar item
CalDAV::GetItem(
	fClient,
	path,
	[](std::wistream &is) {
		// print to standard output
		std::wcout << is.rdbuf();
		}
	);
}


/*	WriteItems
	Store the raw, unintepreted content of the named calendar item
*/
void Session::WriteItem(
	const wchar_t	path[],
	const wchar_t	filePath[]
	)
{
FILE *const file = wfopen_s(filePath, L"r,ccs=utf-16le");
std::wifstream ifs(file);
if (!ifs) throw "can't open file";
	
// get the corresponding calendar item
CalDAV::SetItem(
	fClient,
	path,
	[&ifs](std::wstreambuf &osb) {
		// just write file stream directly to output stream buffer
		ifs >> &osb;
		}
	);
}


/*	ReadItemProperties
	Print the raw, uninterpreted content of the named calendar item's properties
*/
void Session::ReadItemProperties(
	const wchar_t	path[]
	)
{
// get 'all properties' of the corresponding calendar item
WebDAV::Find::All(
	fClient,
	path, WebDAV::Depth::zero,
	[](CHTTPClient::Response &response) {
		// present the response as a C++ stream buffer
		/* Here we get the benefit of accessing the response as a standard input stream,
			with CR/LF mapping and conversion to UTF-16.  We're not going through the
			CalDAV layer, and so its line folding is also not happening here. */
		aistreambuf<wchar_t, CHTTPClient::DecodingInputAdapter<wchar_t>> isb(response);
			
		// print to standard output
		std::wcout << &isb;
		}
	);
}


/*	ReadItemPropertyNames
	Print the calendar item's properties
*/
void Session::ReadItemPropertyNames(
	const wchar_t	path[]
	)
{
// get the names of all properties of the corresponding calendar item
/* This is what I'm getting back from iCloud:
	<propstat>
		<prop>
		</prop>
		<status>HTTP/1.1 200 OK</status>
	</propstat>
	<propstat>
		<prop>
			<propname xmlns="DAV:"/>
		</prop>
		<status>HTTP/1.1 404 Not Found</status>
	</propstat>
	
	In particular, I'm not sure now whether the "propname" element encloses the (empty)
	response structure (which is not what RFC 4918 §9.1.4 suggests) or whether
	it's an actual property name (which goes counter to the usual PROPFIND convention
	of looking for the "propname" response element corresponding to the request; and
	counter to the "404" being returned).  My guess is that the former interpretation
	is correct.
*/
WebDAV::Find::Properties(
	fClient, path, WebDAV::Depth::zero,
	WebDAV::Response(),
	WebDAV::Find::PropertyName(
		[](const wchar_t name[]) { std::wcout << name << '\n'; }
		)
	);
}


/*	WriteCalendarItem
	Write the named calendar item from a parsed interpretation
*/
void Session::WriteCalendarItem(
	const wchar_t	path[],
	const wchar_t	filePath[]
	)
{
DynamicCalendar<wchar_t> calendarItem;
	
// open file for UTF-16
FILE *const file = wfopen_s(filePath, L"r,ccs=utf-16le");
std::wifstream ifs(file);				// Windows extension *** I assume this also closes the FILE
if (!ifs) throw "couldn't open file stream";
	
// parse into calendar object
DynamicCalendar<wchar_t>::Parser(calendarItem, ifs)();
	
WriteCalendarItemToCalDAV(path, calendarItem);
}


/*	SupportedReportSet
	Print names of supported reports
*/
void Session::SupportedReportSet(
	const wchar_t	path[]
	)
{
// get the names of the supported reports
WebDAV::Find::Properties(
	fClient,
	path,
	WebDAV::Depth::zero,
	
	WebDAV::Response(),
	
	VersioningDAV::SupportedReportSet(
		[](const wchar_t reportName[]) { std::wcout << reportName << '\n'; }
		)
	);
}


/*	SupportedCollationSett
	Print names of supported collations
*/
void Session::SupportedCollationSett(
	const wchar_t	path[]
	)
{
// get the names of the supported reports
/* *** haven't gotten this to actually return anything */
WebDAV::Find::Properties(
	fClient,
	path,
	WebDAV::Depth::zero,
	WebDAV::Response(),
	CalDAV::SupportedCollationSet([](const wchar_t collation[]) { std::wcout << collation << '\n'; })
	);
}
