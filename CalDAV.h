/*
	CalDAV
	
	CalDAV calendaring transport and storage as dependent on WebDAV
	
	2019/05/02	Originated
	2023/05/21	Factored out session management
	
	Copyright © 2019-2023 by: Ben Hekster
	
	RFC 4791	Calendaring Extensions to WebDAV (CalDAV)
*/

#pragma once

#include <istream>
#include <sstream>
#include <string>

#include "HTTPClient.h"
#include "WebDAV.h"



/*	CalDAV
	Principal CalDAV service
*/
namespace CalDAV {
	std::wstring	GetPrincipalPath(CHTTPClient&, const wchar_t contextPath[]);
	std::wstring	GetCalendarHomeSet(CHTTPClient&, const wchar_t principalPath[]);
	void		GetCalendars(CHTTPClient&, const wchar_t path[], const std::function<void (const std::wstring&, const std::wstring&)>&);
	void		GetItem(CHTTPClient&, const wchar_t path[], const std::function<void (std::wistream&)> &Recipient);
	void		SetItem(CHTTPClient&, const wchar_t path[], const std::function<void (std::wstreambuf&)> &Sender);
	void		Query(CHTTPClient&, const wchar_t path[], WebDAV::Depth, const char query[]);
	
	
	template <typename Callable>
	struct CalendarData;
	
	template <typename Callable>
	struct CalendarHomeSet;
	
	template <typename Callable>
	struct SupportedCollationSet;
	
	
	// calendar-multiget RFC 4719 §7.9
	namespace MultiGet {
		template <class A, class... Is>
		void		Properties(
					CHTTPClient	&client,
					const wchar_t	path[],
					DAV::Depth	depth,
					const std::vector<std::wstring> &paths,
					A		a,
					Is...		is
					);
		
		template <class A, class... Is>
		struct Query;
		}
	};



/*	CalendarData
	Specify which parts of calendar object resources need to be returned
	RFC 4791 §9.6
*/
template <typename Callable>
struct CalDAV::CalendarData : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"calendar-data";
	static constexpr std::wstring_view gXML = L"<C:calendar-data/>";
	
	static constexpr StateParser::State
		gState {
			{},
			nullptr, nullptr,
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&CalendarData::Characters)
			};
	
	
			CalendarData(Callable c) : FContent(c) {}
	};


/*	CalendarHomeSet
	Return calendar collection resources associated with a given principal resource
	RFC 4791 §6.2.1
	
	This seems actually to be returned within the response 'href', not within the 'calendar-home-set'.
	On iCloud:
	
	<response xmlns="DAV:">
		<href>/</href>
		<propstat>
			<prop></prop>
			<status>HTTP/1.1 200 OK</status>
		</propstat>
	</response>
*/
template <typename Callable>
struct CalDAV::CalendarHomeSet : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"calendar-home-set";
	static constexpr std::wstring_view gXML = L"<calendar-home-set xmlns='urn:ietf:params:xml:ns:caldav'/>";
	
	static constexpr StateParser::State gStateHREF {
			{},
			{}, {},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&CalendarHomeSet::Characters)
			};
			
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"href", &gStateHREF },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			CalendarHomeSet(Callable c) : FContent(c) {}
	};


/*	SupportedCollationSet
	Return supported collation methods
	Example of a property with internal structure
	RFC 4791 §7.5.1
*/
template <typename Callable>
struct CalDAV::SupportedCollationSet : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"supported-collation-set";
	static constexpr std::wstring_view gXML = L"<supported-collation-set xmlns='urn:ietf:params:xml:ns:caldav'/>";
	
	static constexpr StateParser::State gStateAnything {
			{},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&SupportedCollationSet::Characters)
			};
			
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"supported-collation", &gStateAnything },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			SupportedCollationSet(Callable c) : FContent(c) {}
	};



/*

	MultiGet -- retrieve specific calendar object resources from within a collection

*/

/*	MultiGet::Query
	Represents a request for a set of properties
*/
template <class A, class... Is>
struct CalDAV::MultiGet::Query : public WebDAV::Multistatus<A, WebDAV::PropertyTransitions<Is...>> {
	using PT = WebDAV::PropertyTransitions<Is...>;

protected:
	// note the end tag has a formatter for the <href> list (see RFC 4719 §9.10)
	static constexpr std::wstring_view
			gStartTag = LR"(<C:calendar-multiget xmlns:D="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav">)",
			gEndTag = L"{}</C:calendar-multiget>";
	
	
	PT		fProperties;

public:
	
	
	// length of XML query string
	static constexpr std::size_t gXMLL = gStartTag.size() + PT::gXML.size() + gEndTag.size();
	
	// XML query string
	static constexpr std::array<wchar_t, gXMLL + 1> gXMLa = []() {
			/* If the array is too short for the string, compilation will fail due to a call to "_CrtDbgReport" */
			std::array<wchar_t, gXMLL + 1> result {};
			
			// concatenate string into 'result' (note 'mutable' to maintain the iterator between invocations)
			auto append = [resultAt = result.begin()](const std::wstring_view &s) mutable {
				resultAt = std::copy(s.begin(), s.end(), resultAt);
				};
			append(gStartTag);
			/* Note that you must construct using an explicit size; otherwise we'll try to read past the end
			   of the array at compile time, looking for the NUL terminator. */
			append(std::wstring_view { PT::gXML.data(), PT::gXML.size() });
			append(gEndTag);
			// note we're NUL terminated because the std::array was allocated one too long, and default-initialized
			
			return result;
			}();
	
	static constexpr std::wstring_view gXML { gXMLa.data(), gXMLa.size() };
	
	
			Query(
					A		a,
					Is...		is
					) :
					WebDAV::Multistatus<A, PT>(a),
					fProperties(is...)
					{}
	};


/*	MultiGet::Properties
	Compile-time construction of obtaining arbitrary WebDAV properties
*/
template <class A, class... Is>
void CalDAV::MultiGet::Properties(
	CHTTPClient	&client,
	const wchar_t	path[],
	DAV::Depth	depth,
	const std::vector<std::wstring> &paths,
	A		a,
	Is...		is
	)
{
// create XML property query string and response parser data structures
WebDAV::Basic response { Query<A, Is...>(a, is...) };

// stream list of <href> into a string
/* This is a fairly lazy way of constructing the list.  Also, for this particular case it would probably be
   more efficient to generate the entire body into a stream; in particular since DAV::Report is doing
   that already for the wide-to-narrow conversion. */
std::wostringstream os;
for (const std::wstring &p: paths)
	os << L"<D:href>" << p.c_str() << L"</D:href>";
const std::wstring &oss = os.str();

// make HTTP 'REPORT' request
FormatString(
	decltype(response)::gXML.data(),
	[&](const wchar_t body[]) {
		DAV::Report(
			client,
			path, depth,
			body,
			[&](CHTTPClient::Response &httpResponse) {
				// parse XML response
				StateParser events(decltype(response)::gStateDocument, response);
				XMLParser parser(events);
				parser(httpResponse.Content());
				}
			);
		},
	oss
	);
}
