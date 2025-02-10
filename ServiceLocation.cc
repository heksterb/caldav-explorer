/*
	ServiceLocation
	
	RFC 2782 service location
	
	2019/04/18	Originated
	
	Copyright © 2019 by: Ben Hekster

	RFC 2782	A DNS RR for Specifying the Location of Services (DNS SRV)
	RFC 5785	Defining Well-Known Uniform Resource Identifiers (URIs)
	RFC 6763	DNS-Based Service Discovery
	RFC 6764	Locating Services for Calendaring Extensions to WebDAV (CalDAV) and vCard Extensions to WebDAV (CardDAV)
*/

#include "DNSClient.h"
#include "HTTPClient.h"
#include "ServiceLocation.h"
#include "String.h"


/*	LocateService
	Attempt service location as per RFC 6763 ("DNS-SD")
*/
void LocateService(
	const wchar_t	service[],
	const wchar_t	protocol[],
	const wchar_t	domainname[],
	const std::function<void (const wchar_t hostname[], unsigned short port, unsigned short weight, unsigned short priority)> &Instance,
	const std::function<void (const std::wstring_view &key, const wchar_t value[])> &Metadata
	)
{
// prepare the DNS hostname to query
FormatString(
	L"_{}._{}.{}",
	[&Instance, &Metadata](const wchar_t record[]) {
		// locate service host address from SRV record [§6.2]
		CDNSClient().QuerySRV(
			record,
			[&Instance](const CDNSClient::SRV &srv) {
				Instance(srv.Name(), srv.Port(), srv.Weight(), srv.Priority());
				}
			);
		
		// locate service context path from TXT record  [§6.3]
		CDNSClient().QueryTXT(
			record,
			[&Metadata](const CDNSClient::TXT &txt) {
				// for all records in the TXT
				for (unsigned i = 0; i < txt.RecordsN(); ++i)
					if (
						const wchar_t
							// get the record
							*const record = txt[i],
							
							// find the key/value separator
							*const separator = wcschr(record, L'=');
						
						// has theseparator?
						separator
						) {
						Metadata(
							std::wstring_view(record, separator - record),
							separator + 1
							);
						}
				}
			);
		},
	service,
	protocol,
	domainname
	);
}



/*	gServiceLabels
	SRV service labels for CalDAV (RFC 6764)
*/
const wchar_t *DAVServiceLocation::gServiceLabels[] = {
	L"caldav",
	L"caldavs",
	L"carddav",
	L"carddavs"
	};


/*	gWellKnownContextPaths
	Fall-back context path in case none is found through DNS TXT
*/
const wchar_t *DAVServiceLocation::gWellKnownContextPaths[] = {
	L"/.well-known/caldav",
	L"/.well-known/caldav", // linker will coalesce identical strings
	L"/.well-known/carddav",
	L"/.well-known/carddav"
	};


/*	Locate
	DAV service location as per RFC 6764
*/
std::optional<DAVServiceLocation> DAVServiceLocation::Locate(
	Service		service,
	const wchar_t	protocol[],
	const wchar_t	domainname[]
	)
{
std::optional<DAVServiceLocation> result;

// attempt to perform service discovery
LocateService(
	gServiceLabels[service],
	protocol,
	domainname,
	
	// callback with service instances
	[&result](
		const wchar_t	hostname[],
		unsigned short	port,
		unsigned short	weight,
		unsigned short	priority
		) {
		// *** should honor priority and weight (RFC 6764)
		assert(!result.has_value());
		
		DAVServiceLocation &location = result.emplace();
		location.fHost = hostname;
		location.fPort = port;
		},
	
	// callback with service metadata
	[&result](
		const std::wstring_view &key,
		const wchar_t	value[]
		) {
		// *** should have received one instance
		assert(result.has_value());
		DAVServiceLocation &location = *result;
		
		// context path? [§6.3]
		if (key == L"path")
			location.fPath = value;
		}
	);

// got at least a SRV record?
if (result) {
	DAVServiceLocation &location = *result;
	
	// but didn't get context path through TXT?
	/* RFC 6763 appears to dictate that TXT must be present if SRV is.  However, RFC 6764
	   explicitly requires (§4) the use of a 'well-known' URI when the TXT is absent.
	   This is in fact the mechanism that works for iCloud. */
	if (result->fPath.empty())
		result->fPath = gWellKnownContextPaths[service];
	}

return result;
}