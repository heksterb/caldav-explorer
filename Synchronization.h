/*
	Synchronization
	
	WebDAV collection synchronization extensions
	
	2023/09/26	Originated
	
	Copyright © 2023 by: Ben Hekster
	
	RFC 6578	Collection Synchronization for Web Distributed Authoring and Versioning (WebDAV)
*/

#pragma once

#include <iostream>

#include "WebDAV.h"

struct CHTTPClient;



/*	SynchronizationDAV
	Collection synchronization extensions to WebDAV
*/
namespace SynchronizationDAV {
	template <typename Callable>
	struct Token;
	
	template <class A, class... Is>
	struct Query;
	
	template <typename ReturnToken, class A, class... Is>
	void		Perform(
				CHTTPClient&,
				const wchar_t	path[],
				const wchar_t	token[],
				ReturnToken,
				A,
				Is...
				);
	};



/*	SynchronizationDAV::Query
	Represents a request for a set of properties
	
	<D:sync-collection xmlns:D="DAV:">
		<D:sync-token/>
		<D:sync-level>1</D:sync-level>
		<D:prop>
			<D:getetag/>
		</D:prop>
	</D:sync-collection>
*/
template <class A, class... Is>
struct SynchronizationDAV::Query : public WebDAV::Multistatus<A, WebDAV::PropertyTransitions<Is...>> {
	using PT = WebDAV::PropertyTransitions<Is...>;

protected:
	static constexpr std::wstring_view
			gStartTag = LR"(<D:sync-collection xmlns:D="DAV:"><D:sync-token>{}</D:sync-token><D:sync-level>1</D:sync-level>)",
			gEndTag = L"</D:sync-collection>";
	
	
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


/*	Token

*/
template <typename Callable>
struct SynchronizationDAV::Token : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Content(const wchar_t token[]) { FContent(token); }

public:
	static constexpr wchar_t tag[] = L"sync-token";
	
	static constexpr StateParser::State gState {
			{},
			{}, {},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&Token::Content)
			};
	
	
			Token(Callable c) : FContent(c) {}
	};




/*	Perform
	
*/
template <typename ReturnToken, class A, class... Is>
void SynchronizationDAV::Perform(
	CHTTPClient	&client,
	const wchar_t	path[],
	const wchar_t	*token,					// NULL for initial
	ReturnToken	RT,
	A		a,
	Is...		is
	)
{
// create XML property query string and response parser data structures
WebDAV::Basic response {
	Query<A, Is...>(a, is...),
	Token(RT)
	};

if (!token) token = L"";

// make HTTP 'REPORT' request
FormatString(
	decltype(response)::gXML.data(),
	[&](const wchar_t body[]) {
		// query properties
		DAV::Report(
			client,
			path,
			
			// 
			DAV::Depth::zero,
			body,
			[&response](CHTTPClient::Response &httpResponse) {
				// parse XML response
				StateParser events(decltype(response)::gStateDocument, response);
				XMLParser parser(events);
				parser(httpResponse.Content());
				}
			);
		},
	
	// provide synchronization token if supplied
	token
	);
}
