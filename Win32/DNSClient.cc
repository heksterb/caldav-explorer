/*
	DNSClient
	
	DNSexperimentation
	Win32
	
	2019/04/06	Originated
	
	Copyright © 2019 by: Ben Hekster
*/

#include <assert.h>

#include "NDNS.h"

#include "DNSClient.h"



/*	QuerySRV
	Query for an SRV record
*/
void CDNSClient::QuerySRV(
	const wchar_t	name[],
	const std::function<void (const SRV&)> &Callback
	)
{
// for each result returned from a query
for (const DNS_RECORDW &result: Win32::MyDNS::Query(name, DNS_TYPE_SRV, DNS_QUERY_STANDARD)) {
	assert(result.wType == DNS_TYPE_SRV);
	
	// call back the result
	Callback(static_cast<const SRV&>(result.Data.SRV));
	}
}


/*	QueryTXT
	Query for a TXT record
*/
void CDNSClient::QueryTXT(
	const wchar_t	name[],
	const std::function<void (const TXT&)> &Callback
	)
{
// for each result returned from a query
for (const DNS_RECORDW &result: Win32::MyDNS::Query(name, DNS_TYPE_TEXT, DNS_QUERY_STANDARD))
	/* Apparently, query for TXT caldav.icloud.com returns CNAME; don't understand if that's legal. */
	if (result.wType == DNS_TYPE_TEXT)
		// call back the result
		Callback(static_cast<const TXT&>(result.Data.TXT));
}

