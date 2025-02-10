/*
	ServiceLocation
	
	RFC 6763 service location
	
	RFC 2782 defines the original SRV mechanism
	RFC 1035 defines the TXT record
	RFC 6763 puts the two together and calls it 'DNS-Based Service Discovery' (DNS-SD)
	RFC 6764 applies it to CalDAV specifically
	
	2019/04/18	Originated
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#pragma once

#include <functional>
#include <optional>
#include <string_view>


/*	LocateService
	Internet service location as per RFC 6763 ('DNS-SD')
*/
static void LocateService(
	const wchar_t	service[],
	const wchar_t	protocol[],
	const wchar_t	domainname[],
	const std::function<void (const wchar_t hostname[], unsigned short port, unsigned short weight, unsigned short priority)> &Instance,
	const std::function<void (const std::wstring_view &key, const wchar_t value[])> &Metadata
	);



/*	DAVServiceLocation
	CalDAV and CardDAV service location as per RFC 6764
*/
struct DAVServiceLocation {
protected:
	static const wchar_t *gServiceLabels[];
	static const wchar_t *gWellKnownContextPaths[];

public:
	enum Service {
		kCalDAV,
		kCalDAVSecure,
		kCardDAV,
		kCardDAVSecure
		};
	
	static std::optional<DAVServiceLocation> Locate(
				Service,
				const wchar_t protocol[],
				const wchar_t domainname[]
				);
	
	
	std::wstring	fHost;
	unsigned short	fPort;
	std::wstring	fPath;
	};