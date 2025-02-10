/*
	Session
	
	Calendar DAV session
	
	2019/05/02	Originated
	2023/05/21	Factored out session management
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#pragma once

#include <string>

#include "DAV.h"
#include "Dynamic.h"
#include "Versioning.h"
#include "WebDAV.h"
#include "CalDAV.h"



/*	Session
	Principal CalDAV service session
*/
struct Session {
protected:
	static std::wstring FindPrincipalPath(CHTTPClient&, const wchar_t contextPath[]);
	
	static void	Split(
				const wchar_t url[],
				const std::function<void (const wchar_t path[])> &SameHost,
				const std::function<void (const CHTTPClient::Address&, const wchar_t path[])> &DifferentHost
				);
	
	static Session	MakeServiceFromContext(
				CHTTPClient	&&contextServer,
				const wchar_t	contextPath[],
				const wchar_t	username[],
				const wchar_t	password[]
				);
	
	static Session	MakeServiceFromPrincipal(
				CHTTPClient	&&principalServer,
				const wchar_t	principalPath[],
				const wchar_t	username[],
				const wchar_t	password[]
				);
	
	
	CHTTPClient	fClient;
	
	
	std::wstring	fHomeSetPath;					// guaranteed to be terminated by a slash
	DAV::Allow	fHomeSetAllow;
	DAV::Capabilities fHomeSetCapabilities;
	VersioningDAV::SupportedReports fHomeSetSupportedReports;

public:
	static Session	MakeFromServiceLocation(
				CHTTPClient::Address&,
				const wchar_t	contextPath[],
				const wchar_t	username[],
				const wchar_t	password[]
				);
	
	
			Session(CHTTPClient&&, const wchar_t homeSetPath[]);
			Session(Session&&) noexcept;
	
	CHTTPClient	&Client() { return fClient; }
	
	DAV::Allow	HomeSetAllow() const { return fHomeSetAllow; }
	DAV::Capabilities HomeSetCapabilities() const { return fHomeSetCapabilities; }
	
	std::vector<std::wstring> ListItems(const wchar_t name[]);
	void		ExportCalendarIndividually(const wchar_t name[], const std::function<void (std::wistream&)> &Recipient);
	void		ExportCalendarMultiply(const wchar_t name[]);
	
	DynamicCalendar<wchar_t> ReadCalendarItemFromCalDAV(
				const wchar_t	path[]
				);
	
	void		WriteCalendarItemToCalDAV(
				const wchar_t	path[],
				const DynamicCalendar<wchar_t> &calendarItem
				);
	
	void		CreateCalendar(const wchar_t calendarPath[], const wchar_t calendarName[]);
	void		DeleteCalendar(const wchar_t calendarPath[]);
	void		RenameCalendar(const wchar_t calendarPath[], const wchar_t calendarName[]);
	void		ExportCalendar(const wchar_t calendarPath[]);
	void		SynchronizeCalendar(const wchar_t calendarPath[], const wchar_t *token);
	void		QueryCalendar(const wchar_t calendarPath[]);
	void		ListCalendars();
	void		ListCalendarItems(const wchar_t calendarPath[]);
	void		ReadItem(const wchar_t path[]);
	void		WriteItem(const wchar_t path[], const wchar_t filePath[]);
	void		ReadItemProperties(const wchar_t path[]);
	void		ReadItemPropertyNames(const wchar_t path[]);
	void		WriteCalendarItem(const wchar_t path[], const wchar_t filePath[]);
	void		SupportedReportSet(const wchar_t path[]);
	void		SupportedCollationSett(const wchar_t path[]);
	};
