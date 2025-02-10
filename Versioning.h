/*
	Versioning
	
	Versioning extensions to WebDAV
	
	2023/05/20	Originated
	
	Copyright © 2023 by: Ben Hekster
	
	RFC 3253	Versioning Extensions to WebDAV (Web Distributed Authoring and Versioning)
	
	Actually, this only exists to embody the existence of 'versioning extensions'
	and, specifically, the existence of the 'REPORT' method used by CalDAV.
	The specific REPORT actually used by CalDAV is in the CalDAV compilation unit.
*/

#pragma once

#include "WebDAV.h"


/*	VersioningDAV
	Versioning extensions to WebDAV
*/
namespace VersioningDAV {
	void		ExpandProperty(
				CHTTPClient&,
				const wchar_t	path[],
				WebDAV::Depth,
				const wchar_t	properties[]
//				WebDAV::Response&
				);
	
	template <typename Callable>
	struct SupportedReportSet;
	
	
	/*	SupportedReports
		Collection of supportable report types
		*** CalDAV and CardDAV leaking in here
		This probably isn't useful other than a way of discovering new report types
	*/
	struct SupportedReports {
		bool		fACLPrincipalPropSet {},
				fPrincipalMatch {},
				fPrincipalPropertySearch {},
				fExpandProperty {},
				fCalendarServerPrincipalSearch {},
				fCalendarQuery {},
				fCalendarMultiGet {},
				fFreeBusyQuery {},
				fAddressbookQuery {},
				fAddressbookMultiGet {},
				fSyncCollection {};
		
		void		Add(const wchar_t[]);
		};
	};


/*	SupportedReportSet
	Identify reports supported by the resource
	Example of a property with multiple level internal structure; and the internal structure
	consists of unpredictable XML elements
	RFC 3253 §3.1.5
*/
template <typename Callable>
struct VersioningDAV::SupportedReportSet : public StateParser::Response {
protected:
	Callable	FTag;
	
	void		Begin(const wchar_t content[]) { FTag(content); }

public:
	static constexpr wchar_t tag[] = L"supported-report-set";
	static constexpr std::wstring_view gXML = L"<D:supported-report-set/>";
	
	
	/* ***	This will produce wrong results if the content of the <report> element is structured;
		which I don't *think* it is allowed to be:
		
			<!ELEMENT report ANY>
			ANY value: a report element type
		
		because of "report element type".  Nevertheless, it would be better to handle that case.
	*/
	static constexpr StateParser::State gStateAnything {
			{},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&SupportedReportSet::Begin)
			};
	
	static constexpr StateParser::State::Transition gTransitionsFromReport[2] = {
			{ L"", &gStateAnything },
			{}
			};
	
	static constexpr StateParser::State gStateReport {
			gTransitionsFromReport
			};
	
	static constexpr StateParser::State::Transition gTransitionsFromSupportedReport[2] = {
			{ L"report", &gStateReport },
			{}
			};
	
	static constexpr StateParser::State gStateSupportedReport {
			gTransitionsFromSupportedReport
			};
	
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"supported-report", &gStateSupportedReport },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			SupportedReportSet(Callable c) : FTag(c) {}
	};
