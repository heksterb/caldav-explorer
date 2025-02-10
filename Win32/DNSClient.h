/*
	DNSClient
	
	DNSexperimentation
	Win32
	
	2019/04/06	Originated
	
	Copyright © 2019 by: Ben Hekster
*/

#pragma once

#include <functional>

#include <WINDEF.H>
#include <WINBASE.H>
#include <WINDNS.H>


/*	CDNSClient
	DNS client session
*/
struct CDNSClient {
	/*	SRV
		Result of SRV RR
	*/
	struct SRV : public DNS_SRV_DATA {
		const wchar_t	*Name() const { return pNameTarget; }
		unsigned short	Port() const { return wPort; }
		unsigned short	Weight() const { return wWeight; }
		unsigned short	Priority() const { return wPriority; }
		};


	/*	TXT
		Result of TXT RR
	*/
	struct TXT : public DNS_TXT_DATA {
		unsigned	RecordsN() const { return dwStringCount; }
		const wchar_t	*operator[](unsigned i) const { return pStringArray[i]; }
		};


	void		QuerySRV(const wchar_t name[], const std::function<void (const SRV&)>&),
			QueryTXT(const wchar_t name[], const std::function<void (const TXT&)>&);
	};
