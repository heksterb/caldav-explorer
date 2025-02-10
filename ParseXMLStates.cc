/*
	ParseXMLStates
	
	Simple state-based XML parser
	for Management
	
	2018/09/27	Originated
	
	Copyright © 2018-2023 by: Ben Hekster
*/
#include <assert.h>
#include <string.h>

#include "ParseXMLStates.h"



/*	StateParser
	Initialize a state-based XML parser
*/
StateParser::StateParser(
	const State	&document,
	Response	&response
	) :
	fDocument(document),
	fState(nullptr),
	fInner(0),
	fResponse(&response)
{
}


/*	StartDocument
	Started parsing an XML document
*/
void StateParser::StartDocument()
{
assert(!fState);

// set initial state
fState = &fDocument;
}


/*	EndDocument
	Completed parsing the XML document
*/
void StateParser::EndDocument()
{
assert(fState == &fDocument);

// not in any state anymore
fState = nullptr;
}


/*	StartElement
	Handle start of XML element
*/
void StateParser::StartElement(
	const XMLParser<StateParser>::String namespaceURI,
	const XMLParser<StateParser>::String name,
	const XMLParser<StateParser>::Attributes attributes
	)
{
const State *forwardState = nullptr;
Response *forwardResponse = nullptr;

// still recognizing states?
if (fInner == 0)
	if (const State::Transition *forward = fState->fForward) {
		// for each possible transition out *** std::find
		for (; forward->fState; forward++)
			if (
				// matches anything?
				forward->fElement[0] == '\0' ||
				
				// matches?
				XMLParser<StateParser>::StringRep(name) == forward->fElement
				) break;
		
		// next state, or nullptr for unrecognized element
		forwardState = forward->fState;
		
		// adjust to response sub-object
		forwardResponse = reinterpret_cast<Response*>(reinterpret_cast<char*>(fResponse) + forward->fResponseOffset);
		}

// recognized transition?
if (forwardState) {
	fStack.push(fState);
	fResponsesStack.push(fResponse);
	fState = forwardState;
	fResponse = forwardResponse;
	
	// signal start of state
	if (void (Response::*Start)(const XMLParser<StateParser>::String) = fState->FStart) (fResponse->*Start)(name);
	}

else
	// one level deeper inside unrecognized transition
	fInner++;
}


/*	EndElement
	Handle end of XML element
*/
void StateParser::EndElement(
	const XMLParser<StateParser>::String namespaceURI,
	const XMLParser<StateParser>::String name
	)
{
// inside regular state?
if (fInner == 0) {
	// signal end of state
	if (void (Response::*End)() = fState->FEnd) (fResponse->*End)();
	
	// transition back to parent state
	fState = fStack.top(); fStack.pop();
	fResponse = fResponsesStack.top(); fResponsesStack.pop();
	}

else
	// one level shallower inside unrecognized transition
	--fInner;
}


/*	Characters
	Handle element content
*/
void StateParser::Characters(
	const XMLParser<StateParser>::String characters
	)
{
// inside regular state?
if (fInner == 0)
	// signal characters
	if (void (Response::*Characters)(const XMLParser<StateParser>::String) = fState->FCharacters)
		(fResponse->*Characters)(characters);
}
