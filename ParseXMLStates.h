/*
	ParseXMLStates
	
	Simple state-based XML parser
	This potentially works well in the cases where the reply structure is well-defined;
	less so when the reply is open-ended
	
	2018/09/27	Originated
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#pragma once

#include <stack>
#include <vector>
#include <variant>

#include "ParseXML.h"



/*	StateParser
	Parse an XML document by states

	Presumes the XML is well-formed; can't be used to parse recursive structures

NOTES
	In favor of this approach:
	*	don't have to construct the entire result if only a part is needed
	*	don't need to 'parse the response twice' by traversing the in-memory structure
	
	Against this approach:
	*	a lot of manual work writing the structures
	*	the 'extra' work constructing the 'entire' result may not amount to much if a response is
		essentially tailored to the request
	*	difficult to debug
*/
struct StateParser {
public:
	/*	Response
		Handle specific events
	*/
	struct Response {
		void		*dummy;	// *** force alignment
		};
	
	
	/*	State
		Parse state
	*/
	struct State {
		struct Transition {
			XMLParser<StateParser>::Literal fElement;
			const State	*fState;
			unsigned	fResponseOffset;
			};
		
		const Transition *fForward;
		
		// callbacks
		void		(Response::*FStart)(const XMLParser<StateParser>::String),
				(Response::*FEnd)(),
				(Response::*FCharacters)(const XMLParser<StateParser>::String);
		};
	
	void		StartDocument();
	void		EndDocument();
	void		StartElement(const XMLParser<StateParser>::String namespaceURI, const XMLParser<StateParser>::String name, XMLParser<StateParser>::Attributes attributes);
	void		EndElement(const XMLParser<StateParser>::String namespaceURI, const XMLParser<StateParser>::String name);
	void		Characters(const XMLParser<StateParser>::String);

protected:
	const State	&fDocument;
	const State	*fState;
	std::stack<const State*> fStack;
	std::stack<Response*> fResponsesStack;
	unsigned	fInner;
	Response	*fResponse;

public:
	explicit	StateParser(const State &document, Response&);
			StateParser(const StateParser&) = delete;
	};
