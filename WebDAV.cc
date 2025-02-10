/*
	WebDAV
	
	WebDAV experimentation
	
	2018/09/09	Originated
	2023/03/15	Factored from base class DAV
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#include <ctype.h>
#include <string.h>

#include <iostream>

#include "WebDAV.h"



/*

	response XML parsing

*/

/*	Find::All
	Get 'all properties' of the corresponding resource
	
	See [RFC 4918 §9.1, §14.2, and Appendix F] for some background on what this actually does,
	replacing an older definition in [RFC 2518 §12.14.1]
	
	The interface returns the HTTP response because there doesn't seem to be anything
	reasonable that is more specific.  In general, we can't really XML parse it since we
	don't know and are not declaring what elements to expect.
*/
void WebDAV::Find::All(
	CHTTPClient	&client,
	const wchar_t	path[],
	Depth		depth,
	const std::function<void (CHTTPClient::Response&)> &Response
	)
{
DAV::PropertyFind(
	client,
	path, depth,
	LR"(<?xml version="1.0" encoding="utf-8"?><D:propfind xmlns:D="DAV:"><D:allprop/></D:propfind>)",
	Response
	);
}


/*	MakeCollection
	Make WebDAV collection §9.3
	*** actually this has calendar stuff in it too
*/
void WebDAV::MakeCollection(
	CHTTPClient	&client,
	const wchar_t	path[],
	const wchar_t	name[]
	)
{
// format the name into the XML request body
FormatString(
	// *** I think we can use the static construction approach here as well: note <prop> again
	LR"(<?xml version="1.0" encoding="utf-8"?><D:mkcol xmlns:D="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav"><D:set><D:prop><D:resourcetype><D:collection/><C:calendar/></D:resourcetype><D:displayname>{}</D:displayname></D:prop></D:set></D:mkcol>)",
	[&](const wchar_t body[]) {
		// use aostream<wchar_t> for UTF-16-to-UTF-8 conversion
		/* An alternative approach would be to use the string stream to also format the name into the body. */
		aostreambuf<wchar_t, CHTTPClient::EncodingOutputAdapter<wchar_t>> osb;
		std::wostream(&osb) << body;
		osb.pubsync();
		const std::string_view narrow(osb.adapter().narrow().data(), osb.adapter().narrow().size());
		
		// make DAV request
		DAV::MakeCollection(
			client, path, narrow,
			// response
			[](CHTTPClient::Response &response) {
				// present the response as a C++ stream
				aistreambuf<wchar_t, CHTTPClient::DecodingInputAdapter<wchar_t>> isb(response);
		
				std::wcout << &isb;
				}
			);
		},
	name
	);
}
