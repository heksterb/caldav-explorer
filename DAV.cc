/*
	DAV
	
	'HTTP parts' of WebDAV (e.g., the new HTTP verbs and headers)
	The other 'XML parts' of DAV are in WebDAV
	Note that the specifications don't make this distinction
	
	2018/09/09	Originated
	2023/03/15	Factored from base class DAV
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#include <cctype>
#include <string.h>

#include <functional>
#include <iostream>
#include <string_view>

#include "DAV.h"



/*	TokenizeTrim
	Apply the function ‘f’ to each token in the given string, trimming whitespace off both ends
*/
static void TokenizeTrim(
	std::string_view line,
	const char	c,
	const std::function <void (const std::string_view)> &F
	)
{
// for all property parameters
for (
	size_t separatorI;
	
	!line.empty();
	
	line.remove_prefix(separatorI != std::string_view::npos ? separatorI + 1 : line.length())
	) {
	// find the end of this token
	separatorI = line.find_first_of(c);
	std::string_view token(line.data(), separatorI != std::string_view::npos ? separatorI : line.length());
	
	// remove whitespace on either side of the separator
	while (!token.empty() && std::isspace(token.front())) token.remove_prefix(1);
	while (!token.empty() && std::isspace(token.back())) token.remove_suffix(1);
	
	// call back
	F(token);
	}
}



/*

	DAV
	
	*** A lot of these functions are very similar
	*** Stream interface (as opposed to string) to provide body?

*/

/*	DepthHeader::gStrings
	HTTP values for 'Depth' header
*/
const wchar_t *const DAV::DepthHeader::gStrings[] = {
	L"0",
	L"1",
	L"infinity"
	};


/*	DepthHeader
	Prepare to provide proper 'Depth' header
*/
DAV::DepthHeader::DepthHeader(
	DAV::Depth	depth
	) :
	fDepth(gStrings[static_cast<unsigned>(depth)])
{
}


/*	DepthHeader::()
	Provide headers
*/
void DAV::DepthHeader::operator()(
	const std::function<void (const wchar_t*, const wchar_t*)> &SupplyHeader
	)
{
SupplyHeader(L"Depth", fDepth);
SupplyHeader(L"Content-Type", L"application/xml; charset=utf-8");
}


/*	PropertyFind
	Access WebDAV properties ß9.1
	'body' is the entire XML document including the declaration
*/
void DAV::PropertyFind(
	CHTTPClient	&client,
	const wchar_t	path[],
	Depth		depth,
	std::wstring_view body,
	const std::function<void (CHTTPClient::Response&)> &Recipient
	)
{
// use aostream<wchar_t> for wide-to-narrow conversion
aostreambuf<wchar_t, CHTTPClient::EncodingOutputAdapter<wchar_t>> osb;
std::wostream(&osb) << body;
osb.pubsync();
		
// make HTTP request
// MUST supply Depth header [RFC 4918 ß9.1]
client.Request(
	path,
	L"PROPFIND",
	
	// headers
	DepthHeader(depth),
	
	// request body
	CHTTPClient::Rekwest(osb.adapter().narrow().data(), osb.adapter().narrow().size()),
	
	// response
	Recipient
	);
}


/*	PropertyPatch
	Modify WebDAV properties ß9.2
	'body' is the entire XML document including the declaration
*/
void DAV::PropertyPatch(
	CHTTPClient	&client,
	const wchar_t	path[],
	Depth		depth,
	std::wstring_view body,
	const std::function<void (CHTTPClient::Response&)> &Recipient
	)
{
// use aostream<wchar_t> for wide-to-narrow conversion
aostreambuf<wchar_t, CHTTPClient::EncodingOutputAdapter<wchar_t>> osb;
std::wostream(&osb) << body;
osb.pubsync();
		
// make HTTP request
// MUST supply Depth header [RFC 4918 ß9.1]
client.Request(
	path,
	L"PROPPATCH",
	
	// headers
	DepthHeader(depth),
	
	// request body
	CHTTPClient::Rekwest(osb.adapter().narrow().data(), osb.adapter().narrow().size()),
	
	// response
	Recipient
	);
}


/*	Delete

*/
void DAV::Delete(
	CHTTPClient	&client,
	const wchar_t	path[]
	)
{
// delete
client.Request(
	path,
	L"DELETE",
	
	// headers
	[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
	
	// request body
	{},
	
	// response
	[](CHTTPClient::Response&) {}
	);
}


/*	GetServerOptions
	HTTP 1.1 OPTIONS
*/
void DAV::GetServerOptions(
	CHTTPClient	&client,
	const wchar_t	path[],
	const OnConnectCallback &OnConnect
	)
{
// get server options
client.Request(
	path,
	L"OPTIONS",
	
	// headers
	[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
	
	// request body
	CHTTPClient::Rekwest(),
	
	// response
	[&OnConnect](const CHTTPClient::Response &response) {
		// get "Allow:" header
		const unsigned allowHeaderL = response.GetLength(CHTTPClient::Response::kHeaderAllow);
		char *const allowHeader = static_cast<char*>(alloca(allowHeaderL + 1));
		response.Get(CHTTPClient::Response::kHeaderAllow, allowHeader, allowHeaderL + 1);
		
		// get "DAV:" header
		const unsigned davHeaderL = response.GetLength(CHTTPClient::Response::kHeaderDAV);
		char *const davHeader = static_cast<char*>(alloca(davHeaderL + 1));
		response.Get(CHTTPClient::Response::kHeaderDAV, davHeader, davHeaderL + 1);
		
		// call back
		OnConnect(allowHeader, davHeader);
		}
	);
}


/*	MakeCollection
	Modify WebDAV properties ß9.2
	'body' is the entire XML document including the declaration
*/
void DAV::MakeCollection(
	CHTTPClient	&client,
	const wchar_t	path[],
	std::string_view body,
	const std::function<void (CHTTPClient::Response&)> &Recipient
	)
{
// make HTTP request
client.Request(
	path,
	L"MKCOL",
	
	// headers
	[](const std::function<void (const wchar_t*, const wchar_t*)> &SupplyHeader) {
		SupplyHeader(L"Content-Type", L"application/xml; charset=utf-8");
		},
	
	// request body
	CHTTPClient::Rekwest(body.data(), body.size()),
	
	// response
	Recipient
	);
}



/*	Report
	RFC 3253 ß3.6
*/
void DAV::Report(
	CHTTPClient	&client,
	const wchar_t	path[],
	Depth		depth,
	const wchar_t	query[],
	const std::function<void (CHTTPClient::Response&)> &Recipient
	)
{
// use aostream<wchar_t> for wide-to-narrow conversion
aostreambuf<wchar_t, CHTTPClient::EncodingOutputAdapter<wchar_t>> osb;
std::wostream(&osb) << query;
osb.pubsync();

// make HTTP 'REPORT' request
client.Request(
	path,
	L"REPORT",
	
	/* Note that the Depth header is optional and defaults to zero; but [RFC 3253 ß3.6] suggests
	   that without it the response might not predictably be 207 Multi-Status. */
	DepthHeader(depth),
	
	// request body
	CHTTPClient::Rekwest(osb.adapter().narrow().data(), osb.adapter().narrow().size()),
	
	// response
	Recipient
	);
}



/*

	DAV::Allow 

*/

DAV::Allow::Allow(
	const char	string[]
	)
{
// tokenize ëAllow:í header
TokenizeTrim(
	string,
	',',
	// for each token verb
	[this](const std::string_view token) {
		// expected verbs
		static const struct VerbFlag {
			const char	*verb;
			bool		(Allow::*flag);
			} tokens[] = {
			{ "OPTIONS", &Allow::fOptions },
			{ "GET", &Allow::fGet },
			{ "HEAD", &Allow::fHead },
			{ "DELETE", &Allow::fDelete },
			{ "PROPFIND", &Allow::fPropFind },
			{ "PUT", &Allow::fPut },
			{ "POST", &Allow::fPost },
			{ "PROPPATCH", &Allow::fPropPatch },
			{ "COPY", &Allow::fCopy },
			{ "MOVE", &Allow::fMove },
			{ "REPORT", &Allow::fReport },
			{ "LOCK", &Allow::fLock },
			{ "UNLOCK", &Allow::fUnlock },
			{ "MKCALENDAR", &Allow::fMakeCalendar },
			{ "MKCOL", &Allow::fMakeCollection },		// *** have never actually seen this, but appears in RFC 5689
			{ "ACL", &Allow::fACL }				// RFC 3744
			};
		
		// find it
		if (
			const VerbFlag *const verbFlag = std::find_if(
				std::begin(tokens), std::end(tokens),
				[&token](const VerbFlag &vf) { return token == vf.verb; }
				);
			verbFlag != std::end(tokens)
			)
			// set the appropriate flag
			this->*verbFlag->flag = true;
		
		else
			DebugBreak();
		}
	);
}



/*

	DAV::Capabilities
 
 NOTE
 	Move up to an HTTP layer

*/

DAV::Capabilities::Capabilities(
	const char	string[]
	)
{
// tokenize ëDAV:í header
TokenizeTrim(
	string,
	',',
	[this](const std::string_view token) {
		// expected capabilities
		static const struct CapabilityFlag {
			const char	*capability;
			bool		(Capabilities::*flag);
			} tokens[] = {
			{ "1", &Capabilities::fOne },					// RFC 4918
			{ "2", &Capabilities::fTwo },
			{ "3", &Capabilities::fThree },
			{ "extended-mkcol", &Capabilities::fExtendedMakeCollection },	// RFC 5689
			{ "calendar-access", &Capabilities::fCalendarAccess },		// RFC 4791
			{ "calendar-audit", &Capabilities::fCalendarAudit },
			{ "calendar-availability", &Capabilities::fCalendarAvailability },
			{ "calendar-default-alarms", &Capabilities::fCalendarDefaultAlarms },
			{ "calendar-managed-attachments", &Capabilities::fCalendarManagedAttachments },
			{ "calendar-no-timezone", &Capabilities::fCalendarNoTimezone },
			{ "calendar-proxy", &Capabilities::fCalendarProxy },
			{ "calendar-query-extended", &Capabilities::fCalendarQueryExtended },
			{ "calendar-schedule", &Capabilities::fCalendarSchedule },
			{ "calendar-auto-schedule", &Capabilities::fCalendarAutoSchedule },	// RFC 6638
			{ "addressbook", &Capabilities::fAddressBook },
			{ "access-control", &Capabilities::fAccessControl },	// RFC 3744
			{ "caldavserver-supports-telephone", &Capabilities::fCalDAVServerSupportsTelephone },
			{ "calendarserver-group-attendee", &Capabilities::fCalendarServerGroupAttendee },
			{ "calendarserver-group-sharee", &Capabilities::fCalendarServerGroupSharee },
			{ "calendarserver-home-sync", &Capabilities::fCalendarServerHomeSync },
			{ "calendarserver-partstat-changes", &Capabilities::fCalendarServerPartstatChanges },
			{ "calendarserver-principal-property-search", &Capabilities::fCalendarServerPrincipalPropertySearch },
			{ "calendarserver-principal-search", &Capabilities::fCalendarServerPrincipalSearch },
			{ "calendarserver-private-comments", &Capabilities::fCalendarServerPrivateComments },
			{ "calendarserver-private-events", &Capabilities::fCalendarServerPrivateEvents },
			{ "calendarserver-recurrence-split", &Capabilities::fCalendarServerRecurrenceSplit },
			{ "calendarserver-sharing", &Capabilities::fCalendarServerSharing },
			{ "calendarserver-sharing-no-scheduling", &Capabilities::fCalendarServerSharingNoScheduling },
			{ "calendarserver-subscribed", &Capabilities::fCalendarServerSubscribed },
			{ "inbox-availability", &Capabilities::fInboxAvailability }
			};
		
		// find it
		if (
			const CapabilityFlag *const capabilityFlag = std::find_if(
				std::begin(tokens), std::end(tokens),
				[&token](const CapabilityFlag &cf) { return token == cf.capability; }
				);
			capabilityFlag != std::end(tokens)
			)
			// set the appropriate flag
			this->*capabilityFlag->flag = true;
		
		else
			DebugBreak();
		}
	);
}
