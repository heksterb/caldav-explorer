/*
	ParseXML
	
	Event-Driven XML Parser
	for Management
	
	2018/10/09	Originated
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#pragma once

#include <WINDEF.H>
#include <WINBASE.H>
#include <XMLLITE.H>

#include <map>
#include <string>


/*	XMLParser

*/
template <class Events>
struct XMLParser {
public:
	using Data = HGLOBAL;
	using Literal = const wchar_t*;
	using String = wchar_t[];
	using Attributes = const std::map<std::string, std::string>&;


	/*	StringRep
		Concrete access to string
	*/
	struct StringRep {
	protected:
		const wchar_t	*const fString;

	public:
				StringRep(const wchar_t string[]) : fString(string) {}

		bool		operator==(const wchar_t s[]) { return wcscmp(fString, s) == 0; }
		};


	/*	Events
		Parse events callback
		*** keep this to turn into a 'requires' clause
	class Events {
	public:
		virtual void	StartDocument() {}
		virtual void	EndDocument() {}
		virtual void	StartElement(const String namespaceURI, const String name, Attributes) {}
		virtual void	EndElement(const String namespaceURI, const String name) {}
		virtual void	Characters(const String) {}
		};
	*/

protected:
	Events		&fCallback;
	IXmlReader	*fReader;

	void		StartElement(),
			EndElement(),
			Characters();
	
public:
	explicit	XMLParser(Events&);
			~XMLParser();

	void		operator()(const Data);
	};
