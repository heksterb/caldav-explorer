/*
	WebDAV
	
	'XML parts' of WebDAV (e.g., the XML response bodies)
	The other 'HTTP parts' of DAV are in DAV
	Note that the specifications don't make this distinction
	
	2018/09/09	Originated
	2023/03/15	Factored from base class DAV
	
	Copyright © 2018-2023 by: Ben Hekster
	
	RFC 4918	HTTP Extensions for Web Distributed Authoring and Versioning (WebDAV)
	
	Not handled at all:
	
	RFC 3744	Web Distributed Authoring and Versioning (WebDAV) Access Control Protocol
*/

#pragma once

#include <array>

#include "DAV.h"
#include "ParseXMLStates.h"
#include "String.h"



/*	WebDAV
	WebDAV [RFC 4918]
*/
namespace WebDAV {
	using Depth = typename DAV::Depth;
	
	
	template <class... Is>
	struct Transitions;
	
	template <class... Is>
	struct PropertyTransitions;
	
	template <class... M>
	struct Basic;
	
	template <class A, class PT>
	struct Multistatus;
	
	template <typename Callable>
	struct HREF;
	
	template <typename Callable>
	struct Begin;
	
	template <typename Callable>
	struct End;
	
	template <class... Is>
	struct Response;
	
	
	// PROPFIND §9.1
	namespace Find {
		void		All(CHTTPClient&, const wchar_t path[], Depth, const std::function<void (CHTTPClient::Response&)>&);
		
		template <class A, class... Is>
		void		Properties(
					CHTTPClient	&client,
					const wchar_t	path[],
					DAV::Depth	depth,
					A		a,
					Is...		is
					);
		
		template <class A, class... Is>
		struct Query;
		
		template <typename Callable>
		struct PropertyName;
		
		template <typename Callable>
		struct ContentType;
		
		template <typename Callable>
		struct CreationDate;
		
		template <typename Callable>
		struct DisplayName;
		
		template <typename Callable>
		struct ETag;
		
		template <typename Callable>
		struct LastModified;
		
		template <typename Callable>
		struct ResourceType;
		
		template <typename Callable>
		struct CurrentUserPrincipal;
		}
	
	
	// PROPPATCH §9.2
	namespace Patch {
		template <class A, class SR, typename... Ts>
		void		Properties(
					CHTTPClient	&client,
					const wchar_t	path[],
					DAV::Depth	depth,
					A		a,
					SR		sr,
					Ts...		ts
					);
		
		template <class A, class SR>
		struct Query;
		
		template <class... Is>
		struct Set;
		
		struct DisplayName;
		}
	
	// MKCOL §9.3
	void		MakeCollection(CHTTPClient&, const wchar_t path[], const wchar_t name[]);
	};


/*

	sets of property types represented by <prop>

*/

/*	Transitions
	Build (inductively) the XML response parser state transition structures, and
	the XML query string, corresponding to the template argument WebDAV properties
*/
template <>
struct WebDAV::Transitions<> {
	// can't use a zero-size std::array because it "may or may not return NULL"
	static constexpr std::wstring_view gXML {};
	
	// end-of-array sentinel
	static constexpr std::array<StateParser::State::Transition, 1> gTransitions {};
	
	void		*dummy;				// for alignment
	};


template <class I, class... Is>
struct WebDAV::Transitions<I, Is...> {
protected:
	static constexpr std::size_t gXMLL = I::gXML.size() + Transitions<Is...>::gXML.size();
	static constexpr std::array<wchar_t, gXMLL> gXMLa = []() {
			/* If the array is too short for the string, compilation will fail due to a call to "_CrtDbgReport" */
			std::array<wchar_t, gXMLL> result;
			
			// concatenate string into 'result' (note 'mutable' to maintain the iterator between invocations)
			auto append = [resultAt = result.begin()](const std::wstring_view &s) mutable {
				resultAt = std::copy(s.begin(), s.end(), resultAt);
				};
			
			// concatenate string into 'result'
			append(I::gXML);
			append(Transitions<Is...>::gXML);
			
			return result;
			}();
	
	Transitions<Is...> fInners;
	I		fInner;

public:
	/* Note that you must construct using an explicit size; otherwise we'll try to read past the end
	   of the array at compile time, looking for the NUL terminator. */
	static constexpr std::wstring_view gXML { gXMLa.data(), gXMLa.size() };
	
	// calculate response parser structures
	static constexpr std::array<StateParser::State::Transition, 1 + sizeof...(Is) + 1> gTransitions = []() {
			std::array<StateParser::State::Transition, 1 + sizeof...(Is) + 1> result;
			
			// add transition to 'fInner' state
			result.at(0) = StateParser::State::Transition {
				I::tag,
				&I::gState,
				sizeof(void*) + sizeof(Transitions<Is...>)
				};
			
			// copy transitions to previous states
			std::copy(
				std::begin(Transitions<Is...>::gTransitions),
				std::end(Transitions<Is...>::gTransitions),
				++result.begin()
				);
			
			return result;
			}();
	
	
			Transitions(I i, Is... is) :
					fInners(is...),
					fInner(i)
					{}
	};



/*	PropertyTransitions
	Wraps the XML query string from Transitions in a WebDAV "<prop>...</prop>" element
*/
template <class... Is>
struct WebDAV::PropertyTransitions : public WebDAV::Transitions<Is...> {
	static constexpr std::wstring_view
			gStartTag = L"<D:prop>",
			gEndTag = L"</D:prop>";
	
	// length of XML query string
	static constexpr std::size_t gXMLL = gStartTag.size() + Transitions<Is...>::gXML.size() + gEndTag.size();
	
	// XML query string
	static constexpr std::array<wchar_t, gXMLL> gXML = []() {
			/* If the array is too short for the string, compilation will fail due to a call to "_CrtDbgReport" */
			std::array<wchar_t, gXMLL> result;
			
			// concatenate string into 'result' (note 'mutable' to maintain the iterator between invocations)
			auto append = [resultAt = result.begin()](const std::wstring_view &s) mutable {
				resultAt = std::copy(s.begin(), s.end(), resultAt);
				};
			append(gStartTag);
			/* Note that you must construct using an explicit size; otherwise we'll try to read past the end
			   of the array at compile time, looking for the NUL terminator. */
			append(std::wstring_view { Transitions<Is...>::gXML.data(), Transitions<Is...>::gXML.size() });
			append(gEndTag);
			
			return result;
			}();
	
			PropertyTransitions(Is... is) : Transitions<Is...>(is...) {}
	};



/*

	basic WebDAV <multistatus> handling

*/

/*
NOTE
	This could be written as three separate lambdas; which essentially does the same thing but by
	implicitly generating three separate classes.  So this does appear to be cleaner.
*/
template <typename Callable>
struct WebDAV::HREF {
	Callable	c;
	
	void		RespondHREF(const wchar_t href[]) { c(href); }
	};


template <typename Callable>
struct WebDAV::Begin {
	Callable	c;
	
	void		RespondBegin() { c(); }
	};


template <typename Callable>
struct WebDAV::End {
	Callable	c;
	
	void		RespondEnd() { c(); }
	};


/*	Response
	Becomes a derived class of all its template arguments
*/
template <class... Is>
struct WebDAV::Response : public Is... {
	};

template <>
struct WebDAV::Response<> {
	void		*dummy = nullptr;
	};


/*	Multistatus
	Set up response parser structures for the 'basic' parts of WebDAV:
	
	<response>
		<href>/calendars/__uids__/B1206638-BF02-404E-A9D4-B9BC3107C01C/calendar/</href>
		<propstat>
			<prop>...</prop>
			<status>HTTP/1.1 200 OK</status>
		</propstat>
	</response>
	
	The structures under <prop> are provided by the 'PT' template argument
	(which is expected to be a PropertyTransitions).
	
	Handlers for the begin and end tags of <response> and for the content of <href> are provided
	by the 'A' template argument (which is expected to be a Response).
*/
template <class A, class PT>
struct WebDAV::Multistatus : StateParser::Response {
	A		fAdapter;
	
	/* These three functions can only compile if the adapter actually provides the respective
	   operations; however they will only actually be instantiated if the corresponding State
	   pointer-to-member refers to them when the 'constexpr-if' evaluation says they exist. */
	void		RespondHREF(const XMLParser<StateParser>::String href) { fAdapter.RespondHREF(href); }
	void		RespondBegin(const XMLParser<StateParser>::String href) { fAdapter.RespondBegin(); }
	void		RespondEnd() { fAdapter.RespondEnd(); }
	
	
	static constexpr StateParser::State
			gStateHREF = {
				nullptr,
				nullptr,
				nullptr,
				[]() {
					// does adapter define a handler for 'href'?
					/* Note that previously in the run-time model we would always invoke through this
					   the pointer to member, and always invoke a virtual function to decide whether
					   to respond to 'href'.  Now we can determine at compile time whether a response
					   is necessary. */
					if constexpr (requires(A &a) { a.RespondHREF(L""); })
						return static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&Multistatus::RespondHREF);
					
					else
						return nullptr;
					}()
				};
	
	static constexpr StateParser::State
			gStateProperty = {
				PT::gTransitions.data(),
				nullptr, nullptr,
				nullptr
				};
	
	static constexpr StateParser::State
			gStateStatus = {
				{},
				nullptr,
				nullptr
				};
	
	static constexpr StateParser::State::Transition
			gTransitionsFromPropertyStatus[] = {
				{ L"prop", &gStateProperty, sizeof(StateParser::Response) + sizeof(A) },
				{ L"status", &gStateStatus },
				{}
				};
	
	static constexpr StateParser::State
			gStatePropertyStatus = { gTransitionsFromPropertyStatus };
	
	static constexpr StateParser::State::Transition
			gTransitionsFromResponse[] = {
				{ L"href", &gStateHREF },
				{ L"propstat", &gStatePropertyStatus },
				{}
				};
	
	static constexpr StateParser::State
			gState = {
				gTransitionsFromResponse,
				
				[]() {
					// does adapter define a handler for the beginning of a 'response'?
					if constexpr (requires(A &a) { a.RespondBegin(); })
						return static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&Multistatus::RespondBegin);
					
					else
						return nullptr;
					}(),
				
				[]() {
					// does adapter define a handler for the end of a 'response'?
					if constexpr (requires(A &a) { a.RespondEnd(); })
						return static_cast<void (StateParser::Response::*)()>(&Multistatus::RespondEnd);
					
					else
						return nullptr;
					}()
				};
	
	static constexpr wchar_t tag[] = L"response";
	static constexpr std::wstring_view gXML = L"";
	
	
			Multistatus(A adapter) : fAdapter(adapter) {}
	};


/*	Basic
	Set up response parser structures for the 'multistatus' part of WebDAV:
	
	<multistatus xmlns='DAV:'>
	</multistatus>
	
	Handlers for inner XML are specified by template arguments, which usually includes at least Multistatus
*/
template <class... M>
struct WebDAV::Basic : public StateParser::Response {
protected:
	static constexpr std::wstring_view gDocument = LR"(<?xml version="1.0" encoding="utf-8"?>)";
	
	// length of XML query string
	static constexpr std::size_t gXMLL = gDocument.size() + Transitions<M...>::gXML.size();
	
	// XML query string
	static constexpr std::array<wchar_t, gXMLL + 1> gXMLa = []() {
			/* If the array is too short for the string, compilation will fail due to a call to "_CrtDbgReport" */
			std::array<wchar_t, gXMLL + 1> result {};
			
			// concatenate string into 'result' (note 'mutable' to maintain the iterator between invocations)
			auto append = [resultAt = result.begin()](const std::wstring_view &s) mutable {
				resultAt = std::copy(s.begin(), s.end(), resultAt);
				};
			append(gDocument);
			/* Note that you must construct using an explicit size; otherwise we'll try to read past the end
			   of the array at compile time, looking for the NUL terminator. */
			append(std::wstring_view { Transitions<M...>::gXML.data(), Transitions<M...>::gXML.size() });
			// note we're NUL terminated because the std::array was allocated one too long, and default-initialized
			
			return result;
			}();
	
	
	static constexpr StateParser::State
			gStateMultistatus = { Transitions<M...>::gTransitions.data() };
	
	static constexpr StateParser::State::Transition
			gTransitionsFromDocument[] = {
				{ L"multistatus", &gStateMultistatus },
				{}
				};
	
	Transitions<M...> fTransitions;

public:
	static constexpr StateParser::State
			gStateDocument = { gTransitionsFromDocument };
	
	static constexpr std::wstring_view
			gXML = { gXMLa.data(), gXMLa.size() };
	
	
			Basic(M... m) : fTransitions(m...) {} 
	};



/*

	PROPFIND -- property access

*/

/*	PropertyName
	Example of a property with internal structure; and the internal structure consists of
	unpredictable XML elements
	RFC 4918 §14.21
*/
template <typename Callable>
struct WebDAV::Find::PropertyName : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"propname";
	static constexpr std::wstring_view gXML = L"<D:propname/>";
	
	static constexpr StateParser::State gStateAnything {
			{},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&PropertyName::Characters)
			};
			
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"", &gStateAnything },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			PropertyName(Callable c) : FContent(c) {}
	};


/*	ContentType
	The Content-Type header value (RFC 2616 §14.17) as it would be returned by a GET method
	RFC 4918 §15.5
*/
template <typename Callable>
struct WebDAV::Find::ContentType : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"getcontenttype";
	static constexpr std::wstring_view gXML = L"<D:getcontenttype/>";
	
	static constexpr StateParser::State gState {
		{}, {}, {},
		static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&ContentType::Characters)
		};
	
	
			ContentType(Callable c) : FContent(c) {}
	};


/*	CreationDate
	Time and date a resource was created
	RFC 4918 §15.1
*/
template <typename Callable>
struct WebDAV::Find::CreationDate : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"creationdate";
	static constexpr std::wstring_view gXML = L"<D:creationdate/>";
	
	static constexpr StateParser::State gState {
		{}, {}, {},
		static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&CreationDate::Characters)
		};
	
	
			CreationDate(Callable c) : FContent(c) {}
	};


/*	DisplayName
	Resource name suitable for presentation to user
	RFC 4918 §15.2
*/
template <typename Callable>
struct WebDAV::Find::DisplayName : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"displayname";
	static constexpr std::wstring_view gXML = L"<D:displayname/>";
	
	static constexpr StateParser::State gState {
		{}, {}, {},
		static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&DisplayName::Characters)
		};
	
	
			DisplayName(Callable c) : FContent(c) {}
	};


/*	ETag

*/
template <typename Callable>
struct WebDAV::Find::ETag : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"getetag";
	static constexpr std::wstring_view gXML = L"<D:getetag/>";
	
	static constexpr StateParser::State gState {
		{}, {}, {},
		static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&ETag::Characters)
		};
	
	
			ETag(Callable c) : FContent(c) {}
	};


/*	LastModified
	The Last-Modified header value (RFC 2616 §14.29) as it would be returned by a GET method
	RFC 4918 §15.7
*/
template <typename Callable>
struct WebDAV::Find::LastModified : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"getlastmodified";
	static constexpr std::wstring_view gXML = L"<D:getlastmodified/>";
	
	static constexpr StateParser::State gState {
		{}, {}, {},
		static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&LastModified::Characters)
		};
	
	
			LastModified(Callable c) : FContent(c) {}
	};


/*	CurrentUserPrincipal
	The current user's principal resource (?)
	RFC 5397
	
		The DAV:href element contains a URL to a principal resource corresponding to
		the currently authenticated user
	
	This seems to be returned within the response 'href' as well as the 'current-user-principal';
	on iCloud at least:
	
	<response xmlns="DAV:">
		<href>/123456789/principal/</href>
		<propstat>
			<prop>
				<current-user-principal xmlns="DAV:">
					<href xmlns="DAV:">/123456789/principal/</href>
				</current-user-principal>
			</prop>
		<status>HTTP/1.1 200 OK</status>
		</propstat>
	</response>
	
	which is why I used to get this out of the 'response' 'href'.  However, on Twisted
	it's only returned in the 'current-user-principal' 'href':
	
	<response>
		<href>/</href>
		<propstat>
			<prop>
				<current-user-principal>
					<href>/principals/__uids__/01234567-89AB-CDEF-0123-456789ABCDEF/</href>
				</current-user-principal>
			</prop>
			<status>HTTP/1.1 200 OK</status>
		</propstat>
	</response>
*/
template <typename Callable>
struct WebDAV::Find::CurrentUserPrincipal : public StateParser::Response {
protected:
	Callable	FContent;
	
	void		Characters(const wchar_t content[]) { FContent(content); }

public:
	static constexpr wchar_t tag[] = L"current-user-principal";
	static constexpr std::wstring_view gXML = L"<D:current-user-principal/>";
	
	static constexpr StateParser::State gStateHREF {
			{},
			{}, {},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&CurrentUserPrincipal::Characters)
			};
			
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"href", &gStateHREF },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			CurrentUserPrincipal(Callable c) : FContent(c) {}
	};


/*	ResourceType
	Return a type that specifies the nature of the resource
	RFC 4918 §15.9
*/
template <typename Callable>
struct WebDAV::Find::ResourceType : public StateParser::Response {
protected:
	Callable	FCalendarBegin;
	
	void		CalendarBegin(const wchar_t[]) { FCalendarBegin(); }

public:
	static constexpr wchar_t tag[] = L"resourcetype";
	static constexpr std::wstring_view gXML = L"<D:resourcetype/>";
	
	static constexpr StateParser::State gStateCalendar {
			{},
			static_cast<void (StateParser::Response::*)(const XMLParser<StateParser>::String)>(&ResourceType::CalendarBegin)
			};
			
	static constexpr StateParser::State::Transition gTransitionsFromState[2] = {
			{ L"calendar", &gStateCalendar },
			{}
			};
	
	static constexpr StateParser::State gState {
			gTransitionsFromState
			};
	
	
			ResourceType(Callable c) : FCalendarBegin(c) {}
	};


/*	Find::Query
	Represents a request for a set of properties
*/
template <class A, class... Is>
struct WebDAV::Find::Query : public WebDAV::Multistatus<A, PropertyTransitions<Is...>> {
	using PT = PropertyTransitions<Is...>;

protected:
	static constexpr std::wstring_view
			gStartTag = LR"(<D:propfind xmlns:D="DAV:">)",
			gEndTag = L"</D:propfind>";
	
	
	void		*dummy;
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
	
	static constexpr std::wstring_view gXML = { gXMLa.data(), gXMLa.size() };
	
	
			Query(
					A		a,
					Is...		is
					) :
					WebDAV::Multistatus<A, PT>(a),
					fProperties(is...)
					{}
	};


/*	Find::Properties
	Compile-time construction of obtaining arbitrary WebDAV properties
*/
template <class A, class... Is>
void WebDAV::Find::Properties(
	CHTTPClient	&client,
	const wchar_t	path[],
	DAV::Depth	depth,
	A		a,
	Is...		is
	)
{
// create XML property query string and response parser data structures
Basic response { Query<A, Is...>(a, is...) };

// query properties
DAV::PropertyFind(
	client,
	path, depth,
	decltype(response)::gXML.data(),
	[&response](CHTTPClient::Response &httpResponse) {
		// parse XML response
		StateParser events(decltype(response)::gStateDocument, response);
		XMLParser parser(events);
		parser(httpResponse.Content());
		}
	);
}



/*

	PROPPATCH -- property patching

*/

/*	Patch::Query
	Represents a PROPPATCH request for set or reset of specific properties
*/
template <class A, class SR>
struct WebDAV::Patch::Query : public WebDAV::Multistatus<A, typename SR::PT> {
protected:
	// start and end tag for the query
	static constexpr std::wstring_view
			gStartTag = LR"(<D:propertyupdate xmlns:D="DAV:">)",
			gEndTag = L"</D:propertyupdate>";
	
	// length of XML query string
	static constexpr std::size_t gXMLL = gStartTag.size() + SR::gXML.size() + gEndTag.size();
	
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
			append(std::wstring_view { SR::gXML.data(), SR::gXML.size() });
			append(gEndTag);
			
			return result;
			}();
	
	
	// must be first data field of this class *** nasty
	SR		fSetOrReset;

public:
	static constexpr std::wstring_view gXML = { gXMLa.data(), gXMLa.size() };
	
	
			Query(
					A		adapter,
					SR		sr
					) :
					WebDAV::Multistatus<A, SR::PT>(adapter),
					fSetOrReset(sr)
					{}
	};



/*	Patch::Set
	Set properties
	RFC 4918 §14.26
*/
template <class... Is>
struct WebDAV::Patch::Set : public StateParser::Response {
	using PT = PropertyTransitions<Is...>;

protected:
	static constexpr std::wstring_view
			gStartTag = L"<D:set>",
			gEndTag = L"</D:set>";
	
	// length of XML query string
	static constexpr std::size_t gXMLL = gStartTag.size() + PT::gXML.size() + gEndTag.size();
	
	// XML query string
	static constexpr std::array<wchar_t, gXMLL> gXMLa = []() {
			std::array<wchar_t, gXMLL> result;
			
			// concatenate string into 'result' (note 'mutable' to maintain the iterator between invocations)
			auto append = [resultAt = result.begin()](const std::wstring_view &s) mutable {
				resultAt = std::copy(s.begin(), s.end(), resultAt);
				};
			append(gStartTag);
			append(std::wstring_view { PT::gXML.data(), PT::gXML.size() } );
			append(gEndTag);
			
			return result;
			}();

public:
	/*	gXML
		request XML
	*/
	static constexpr std::wstring_view gXML { gXMLa.data(), gXMLa.size() };
	
	
	/* The constructor is here only to support CTAD. */
			Set(Is...) {}
	};


/*	DisplayName
	Set name of resource, suitable for presentation to user
*/
struct WebDAV::Patch::DisplayName : public StateParser::Response {
public:
	/*	gXML
		request XML
	*/
	static constexpr std::wstring_view gXML = L"<D:displayname>{}</D:displayname>";
	
	
	/*	response parser structures
		Can't find this documented.  Twisted just returns the empty property "<displayname/>"	
	*/
	static constexpr wchar_t tag[] = L"displayname";
	
	static constexpr StateParser::State gState {};
	
	
			DisplayName() {}
	};


/*	Patch::Properties
	Create the XML request and prepare the response parser structures to apply
	the given PatchSet and PatchRemove (TBD) patches
*/
template <class A, class SR, typename... Ts>
void WebDAV::Patch::Properties(
	CHTTPClient	&client,
	const wchar_t	path[],
	DAV::Depth	depth,
	A		a,
	SR		sr,
	Ts...		ts
	)
{
// create 'propertyupdate' XML document and response parser data structures
Basic response { Query<A, SR>(a, sr) };

// supply run-time arguments to statically-constructed format string
FormatString(
	decltype(response)::gXML,
	[&](const wchar_t query[]) {
		// patch properties
		DAV::PropertyPatch(
			client,
			path, depth,
			query,
			[&response](CHTTPClient::Response &httpResponse) {
				// parse XML response
				StateParser events(decltype(response)::gStateDocument, response);
				XMLParser parser(events);
				parser(httpResponse.Content());
				}
			);
		},
	ts...
	);
}
