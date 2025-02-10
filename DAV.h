/*
	DAV
	
	'HTTP parts' of WebDAV (e.g., the new HTTP verbs and headers)
	The other 'XML parts' of DAV are in WebDAV
	Note that the specifications don't make this distinction
	
	2018/09/09	Originated
	2023/03/15	Factored from base class DAV
	
	Copyright © 2018-2023 by: Ben Hekster
	
	RFC 4918	HTTP Extensions for Web Distributed Authoring and Versioning (WebDAV)
	
	Not handled at all:
	
	RFC 3253	Versioning Extensions to WebDAV (Web Distributed Authoring and Versioning)
	RFC 3744	Web Distributed Authoring and Versioning (WebDAV) Access Control Protocol
*/

#pragma once

#include "HTTPClient.h"



/*	DAV
	WebDAV [RFC 4918]
*/
namespace DAV {
	using OnConnectCallback = std::function<
		void (const char allow[], const char dav[])
		>;
	
	
	/*	Allow
		Parsed form of HTTP ‘Allow:’ header
	*/
	struct Allow {
		bool		fOptions {},
				fGet {},
				fHead {},
				fDelete {},
				fPropFind {},
				fPut {},
				fPost {},
				fPropPatch {},
				fCopy {},
				fMove {},
				fReport {},
				fLock {},
				fUnlock {},
				fMakeCalendar {},
				fMakeCollection {},
				fACL {};
		
		
				Allow() = default;
				Allow(const char header[]);
		};
	
	
	/*	Capabilities
		Parsed form of HTTP ‘DAV:’ header
		*** clearly we see some CalDAV capabilities leaking in here
	*/
	struct Capabilities {
		bool		fOne {},
				fTwo {},
				fThree {},
				fExtendedMakeCollection {},
				fCalendarAccess {},
				fCalendarAudit {},
				fCalendarAvailability {},
				fCalendarDefaultAlarms {},
				fCalendarManagedAttachments {},
				fCalendarQueryExtended {},
				fCalendarSchedule {},
				fCalendarAutoSchedule {},
				fCalendarNoTimezone {},
				fCalendarProxy {},
				fAddressBook {},
				fAccessControl {},
				fCalDAVServerSupportsTelephone {},
				fCalendarServerGroupAttendee {},
				fCalendarServerGroupSharee {},
				fCalendarServerHomeSync {},
				fCalendarServerPartstatChanges {},
				fCalendarServerPrincipalPropertySearch {},
				fCalendarServerPrincipalSearch {},
				fCalendarServerPrivateComments {},
				fCalendarServerPrivateEvents {},
				fCalendarServerRecurrenceSplit {},
				fCalendarServerSharing {},
				fCalendarServerSharingNoScheduling {},
				fCalendarServerSubscribed {},
				fInboxAvailability {};
		
		
				Capabilities() = default;
				Capabilities(const char header[]);
		};
	
	
	/*	Depth
		Resource depth to which a request applies
	*/
	enum class Depth { zero, one, infinity };
	
	
	/*	DepthHeader
		Provide Depth and Content-Type HTTP headers
	*/
	class DepthHeader {
	protected:
		static const wchar_t *const gStrings[];
		
		const wchar_t	*const fDepth;
	
	public:
		explicit	DepthHeader(DAV::Depth);
		
		virtual void	operator()(const std::function<void (const wchar_t*, const wchar_t*)> &SupplyHeader);
		};
	
	
	// actually HTTP 1.1 [RFC 7231]
	void		GetServerOptions(CHTTPClient&, const wchar_t principalPath[], const OnConnectCallback&);
	
	void		Delete(CHTTPClient&, const wchar_t path[]);
	
	// PROPFIND §9.1
	void		PropertyFind(
				CHTTPClient&,
				const wchar_t	path[],
				Depth,
				std::wstring_view body,
				const std::function<void (CHTTPClient::Response&)>&
				);
	
	// PROPPATCH §9.2
	void		PropertyPatch(
				CHTTPClient&,
				const wchar_t	path[],
				Depth,
				std::wstring_view body,
				const std::function<void (CHTTPClient::Response&)>&
				);
	
	// MKCOL §9.3
	void		MakeCollection(
				CHTTPClient	&client,
				const wchar_t	path[],
				std::string_view body,
				const std::function<void (CHTTPClient::Response&)>&
				);
	
	// REPORT RFC 3253
	void		Report(
				CHTTPClient&,
				const wchar_t	path[],
				Depth,
				const wchar_t	query[],
				const std::function<void (CHTTPClient::Response&)>&
				);
	};
