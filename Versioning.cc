/*
	Versioning
	
	Versioning extensions to WebDAV
	
	2023/05/20	Originated
	
	Copyright © 2023 by: Ben Hekster
*/

#include <malloc.h>

#include <iostream>

#include "DAV.h"
#include "Versioning.h"


/*	ExpandProperty
	Obtain complex information about a resource
	***** not tested; can't find a use case for it
*/
void VersioningDAV::ExpandProperty(
	CHTTPClient	&client,
	const wchar_t	path[],
	WebDAV::Depth	depth,
	const wchar_t	properties[]
//	WebDAV::Response &response
	)
{
// format the requested properties into the XML request body
FormatString(
	// *** I think we can use the static construction approach here as well: note <prop> again
	LR"(<?xml version="1.0" encoding="utf-8" ?><D:expand-property xmlns:D="DAV:">{}</D:expand-property>)",
	[&](const wchar_t body[]) {
		DAV::Report(
			client, path, depth, body,
			
			// response
			[&](CHTTPClient::Response &httpResponse) {
				#if 0
				// parse XML response
				StateParser events(WebDAV::Response::gStateDocument, response);
				XMLParser parser(events);
				parser(httpResponse.Content());
				
				#else
				// present the response as a C++ stream buffer
				aistreambuf<wchar_t, CHTTPClient::DecodingInputAdapter<wchar_t>> isb(httpResponse);
				
				// print to standard output
				std::wcout << &isb;
				#endif
				}
			);
		},
	properties
	);
}


/*	SupportedReports::Add
	Add the given supported report to the set
*/
void VersioningDAV::SupportedReports::Add(
	const wchar_t	report[]
	)
{
// expected supported reports
static const struct ReportFlag {
	const wchar_t	*name;
	bool		(SupportedReports::*flag);
	} reports[] = {
	{ L"acl-principal-prop-set", &SupportedReports::fACLPrincipalPropSet },
	{ L"principal-match", &SupportedReports::fPrincipalMatch },
	{ L"principal-property-search", &SupportedReports::fPrincipalPropertySearch },
	{ L"expand-property", &SupportedReports::fExpandProperty },
	{ L"calendarserver-principal-search", &SupportedReports::fCalendarServerPrincipalSearch },
	{ L"calendar-query", &SupportedReports::fCalendarQuery },
	{ L"calendar-multiget", &SupportedReports::fCalendarMultiGet },
	{ L"free-busy-query", &SupportedReports::fFreeBusyQuery },
	{ L"addressbook-query", &SupportedReports::fAddressbookQuery },
	{ L"addressbook-multiget", &SupportedReports::fAddressbookMultiGet },
	{ L"sync-collection", &SupportedReports::fSyncCollection }
	};

// find it
if (
	const ReportFlag *const reportFlag = std::find_if(
		std::begin(reports), std::end(reports),
		[report](const ReportFlag &rf) { return std::wcscmp(report, rf.name) == 0; }
		);
	reportFlag != std::end(reports)
	)
	// set the appropriate flag
	this->*reportFlag->flag = true;

else
	DebugBreak();
}
