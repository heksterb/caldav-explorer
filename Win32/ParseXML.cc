/*
	ParseXML
	
	Event-Driven XML Parser
	for Management
	
	2018/10/09	Originated
	
	Copyright © 2018 by: Ben Hekster
*/

#include <COMBASEAPI.H>

#include "ParseXML.h"



/* From social.msdn.m.c: XMLLite isn't a COM library, it only looks like one.
   For one, you can't regsvr it.  Maybe because it *uses* COM internally? */


/*	XMLParser
	How to communicate the character set?
*/
template <class Events>
XMLParser<Events>::XMLParser(
	Events		&callback
	) :
	fReader(nullptr),
	fCallback(callback)
{
// create XML parser
if (CreateXmlReader(__uuidof(IXmlReader), &reinterpret_cast<void*&>(fReader), nullptr) != S_OK) throw GetLastError();
}


/*	~XMLParser

*/
template <class Events>
XMLParser<Events>::~XMLParser()
{
fReader->Release();
}


/*	StartElement

*/
template <class Events>
void XMLParser<Events>::StartElement()
{
const wchar_t *name;
if (fReader->GetLocalName(&name, nullptr /* string length */) != S_OK) throw GetLastError();

const wchar_t *namespays;
if (fReader->GetNamespaceUri(&namespays, nullptr /* string length */) != S_OK) throw GetLastError();

// *** attributes

fCallback.StartElement(namespays, name, {});

// empty element? (self-closing tag)
if (fReader->IsEmptyElement())
	// reader doesn't generate an 'EndElement' event
	fCallback.EndElement(namespays, name);
}


/*	EndElement
	Interpret 'end element' event
*/
template <class Events>
void XMLParser<Events>::EndElement()
{
const wchar_t *name;
if (fReader->GetLocalName(&name, nullptr /* string length */) != S_OK) throw GetLastError();

const wchar_t *namespays;
if (fReader->GetNamespaceUri(&namespays, nullptr /* string length */) != S_OK) throw GetLastError();

fCallback.EndElement(namespays, name);
}


/*	Characters

*/
template <class Events>
void XMLParser<Events>::Characters()
{
const wchar_t *value;
if (fReader->GetValue(&value, nullptr /* string length */) != S_OK) throw GetLastError();

fCallback.Characters(value);
}


/*	()
	Parse
*/
template <class Events>
void XMLParser<Events>::operator()(
	HGLOBAL		data
	)
{
// create stream representation of body
IStream *stream;
HRESULT result = CreateStreamOnHGlobal(data, false /* delete handle on release */, &stream);

// associate stream with parser
fReader->SetInput(stream);

// parse until end
for (XmlNodeType nodeType; fReader->Read(&nodeType) == S_OK;) {
	switch (nodeType) {
		case XmlNodeType_None: break;
		case XmlNodeType_Element: StartElement(); break;
		case XmlNodeType_Attribute: break;
		case XmlNodeType_Text: Characters(); break;
		case XmlNodeType_CDATA: Characters(); break;
		case XmlNodeType_ProcessingInstruction: break;
		case XmlNodeType_Comment: break;
		case XmlNodeType_DocumentType: break;
		case XmlNodeType_Whitespace: break;
		case XmlNodeType_EndElement: EndElement(); break;
		case XmlNodeType_XmlDeclaration: fCallback.StartDocument(); break;
		}
	}

// infer end of XML document
fCallback.EndDocument();

stream->Release();
}


// explicit instantiation
#include <../ParseXMLStates.h>
template XMLParser<StateParser>;
