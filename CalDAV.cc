/*
	CalDAV
	
	CalDAV experimentation
	
	2019/05/02	Originated
	2023/05/21	Factored out session management
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#include <iostream>
#include <sstream>
#include <vector>

#include "AdaptableStreamBuffer.h"
#include "CalDAV.h"
#include "Versioning.h"



/*

	CalDAVIAdapter::DecodingInputAdapter
	
	Unfolding of content lines [RFC5545 §3.1]

*/

struct CalDAVIAdapter : CHTTPClient::DecodingInputAdapter<wchar_t> {
protected:
	bool		fConsumedLF;
	
	void		filterData(Splicer<wchar_t>&),
			filterEOF(Splicer<wchar_t>&);

public:
			CalDAVIAdapter(CHTTPClient::Response &response) :
				CHTTPClient::DecodingInputAdapter<wchar_t>(response),
				fConsumedLF(false)
				{}
	
	wchar_t		*filter(wchar_t *begin, wchar_t *end, const wchar_t *limit);
	};


/*	filterData
	Filter the splicer buffer
*/
void CalDAVIAdapter::filterData(
	Splicer<wchar_t> &ci
	)
{
// for each character of the buffer
while (ci) {
	const wchar_t c = ci.read();
	
	if (fConsumedLF) {
		fConsumedLF = false;
		
		// linear whitespace?
		if (c == L' ' || c == L'\t')
			// LF had been legitimately removed; and the whitespace is a continuation 'control' character
			;
		
		else {
			// LF was mistakenly removed, restore it
			ci << L'\n';
			
			ci << c;
			}
		}
	
	else
		if (c == L'\n')
			// hold back the LF in case we're followed by 'continuation' whitespace
			fConsumedLF = true;
		
		else
			ci << c;
	}
}


/*	filterEOF
	Filter at the end of input
*/
void CalDAVIAdapter::filterEOF(
	Splicer<wchar_t> &ci
	)
{
// if we had presumptively swallowed a LF, then restore it
if (fConsumedLF) {
	ci << L'\n';
		
	fConsumedLF = false;
	}
}


/*	filter
	Filter the given data
*/
wchar_t *CalDAVIAdapter::filter(
	wchar_t		*begin,
	wchar_t		*end,
	const wchar_t	*limit
	)
{
Splicer<wchar_t> ci(begin, end, limit);

// data in input, so not at end of input stream?
if (ci)
	filterData(ci);

// at end of input
else
	filterEOF(ci);

return ci.end();
}



/*

	CalDAVOAdapter

	*** make sure not to fold in the middle of a UTF-8/UTF-16 sequence;

*/

#if 0
struct CalDAVOAdapter {
	void		putout(Buffer<char>::Splicer&, char); // ***** not implemented yet
	};
#endif



/*	GetPrincipalPath
	Get path to principal resource of current authenticated user [RFC 5397]
*/
std::wstring CalDAV::GetPrincipalPath(
	CHTTPClient	&client,
	const wchar_t	path[]
	)
{
std::wstring href;

/* Depth zero is important or else you get results for every other resource beneath:
  <d:response>
    <d:href>/rpc/</d:href>
    <d:propstat>[...]</d:propstat>
  </d:response>
  <d:response>
    <d:href>/rpc/principals/</d:href>
    <d:propstat>[...]</d:propstat>
  </d:response>
  <d:response>
    <d:href>/rpc/calendars/</d:href>
    <d:propstat>[...]</d:propstat>
  </d:response>
  <d:response>
    <d:href>/rpc/addressbooks/</d:href>
    <d:propstat>[...]</d:propstat>
  </d:response>
  <d:response>
    <d:href>/rpc/kronolith/</d:href>
  [...]
*/

WebDAV::Find::Properties(
	client,
	path, WebDAV::Depth::zero,
	WebDAV::Response(),
	WebDAV::Find::CurrentUserPrincipal(
		[&href](const wchar_t characters[]) { href = characters; }
		)
	);

return href;
}


/*	GetCalendarHomeSet

*/
std::wstring CalDAV::GetCalendarHomeSet(
	CHTTPClient	&client,
	const wchar_t	principalPath[]
	)
{
std::wstring result;

/* depth must be 'zero' here or iCloud returns 404 Bad Request */
/* Note: Template Argument Deduction used in two places here:
   *	deduction of class template argument of CalendarHomeSet<> from constructor argument
   *	deduction of function template argument of GetProperties from argument
   */
WebDAV::Find::Properties(
	client,
	principalPath, WebDAV::Depth::zero,
	WebDAV::Response(),
	CalDAV::CalendarHomeSet([&result](const wchar_t characters[]) { result = characters; })
	);

return result;
}


/*	GetCalendars
	Return paths to all resources in the given path (usually the 'calendar home set')
	that are 'calendar' types
*/
void CalDAV::GetCalendars(
	CHTTPClient	&client,
	const wchar_t	path[],
	const std::function<void (const std::wstring&, const std::wstring&)> &Result
	)
{
struct Item {
	bool		isCalendar {};
	std::wstring	path;
	std::wstring	displayName;
	};
std::optional<Item> item;

// find all direct descendent 'resource type' and 'display name' properties
WebDAV::Find::Properties(
	client,
	path,
	WebDAV::Depth::one,
	
	WebDAV::Response(
		WebDAV::Begin(
			[&]() {
				// should not be currently parsing an item
				assert(!item);
				
				// create an empty item to parse into
				item.emplace();
				}
			),
		
		WebDAV::HREF(
			[&](const wchar_t characters[]) { item->path = characters; }
			),
		
		WebDAV::End(
			[&]() {
				// should have been parsing an item
				assert(item);
				
				// did we determine it was in fact a calendar?
				if (item->isCalendar)
					// provide result to caller
					Result(item->path, item->displayName);
				
				item.reset();
				}
			)
		),
	
	WebDAV::Find::DisplayName(
		[&](const wchar_t name[]) { item->displayName = name; }
		),
	
	WebDAV::Find::ResourceType(
		[&]() { item->isCalendar = true; }
		)
	);
}


/*	GetItem
	Get the CalDAV item at the given path as text
	
	This is practically identical to a hypothetical 'WebDAV::GetItem' except that it performs
	CalDAV line folding.  There is, in fact, no WebDAV::GetItem because that is identical to HTTP GET.
*/
void CalDAV::GetItem(
	CHTTPClient	&client,
	const wchar_t	path[],
	const std::function<void (std::wistream&)> &Recipient
	)
{
// make HTTP 'GET' request
client.Request(
	path,
	L"GET",
	[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
	CHTTPClient::Rekwest(),
	[&](CHTTPClient::Response &response) {
		// present the response as a C++ stream
		aistreambuf<wchar_t, CalDAVIAdapter> isb(response);
		std::wistream is(&isb);
		
		// present response
		Recipient(is);
		}
	);
}


/*	SeItem
	Set the CalDAV item at the given path from text
*/
void CalDAV::SetItem(
	CHTTPClient	&client,
	const wchar_t	path[],
	const std::function<void (std::wstreambuf&)> &Sender
	)
{
// buffer entire converted stream so we can establish its length
aostreambuf<wchar_t, CHTTPClient::EncodingOutputAdapter<wchar_t>> osb;
Sender(osb);
osb.pubsync();

// make HTTP 'PUT' request
client.Request(
	path,
	L"PUT",
	[](const std::function<void (const wchar_t*, const wchar_t*)> &AcceptHeaders) {
		AcceptHeaders(L"Content-Type", L"text/calendar; charset=utf-8");
		},
	CHTTPClient::Rekwest(osb.adapter().narrow().data(), osb.adapter().narrow().size()),
	[&](CHTTPClient::Response &response) {
		// *** ignore response body
		}
	);
}


/*	Query
	Search for calendar resources per filter
*/
void CalDAV::Query(
	CHTTPClient	&client,
	const wchar_t	path[],
	WebDAV::Depth	depth,
	const char	query[]
	)
{
// construct REPORT XML body
static const char bodyTemplate[] =
	"<?xml version='1.0' encoding='utf-8' ?>"
	"<C:calendar-query xmlns:D='DAV:' xmlns:C='urn:ietf:params:xml:ns:caldav'>"
	"%s"
	"</C:calendar-query>"
	;
const unsigned bodyN = sizeof bodyTemplate + (-2 + strlen(query));
char *const body = static_cast<char*>(alloca(bodyN));
const unsigned bodyL = snprintf(body, bodyN, bodyTemplate, query);
assert(bodyL == bodyN - 1);

// make HTTP 'REPORT' request
client.Request(
	path,
	L"REPORT",
	DAV::DepthHeader(depth),
	CHTTPClient::Rekwest(body, bodyL),
	[&](CHTTPClient::Response &response) {
		// present the response as a C++ stream
		aistreambuf<wchar_t, CHTTPClient::DecodingInputAdapter<wchar_t>> isb(response);
		
		std::wcout << &isb;
		}
	);
}
