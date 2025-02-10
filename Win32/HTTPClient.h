/*
	HTTPClient
	
	HTTP experimentation
	Win32
	
	2018/09/09	Originated
	2023/03/30	Refactored using adaptable stream buffers
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#pragma once

#include <functional>
#include <map>
#include <streambuf>

#include <STRINGAPISET.H>

#include "NHTTP.h"
#include "NMemory.h"

#include "../AdaptableStreamBuffer.h"



/*	CHTTPClient
	HTTP client session
*/
struct CHTTPClient {
public:
	/*	Address
		Internet address to make request to
	*/
	struct Address {
		friend CHTTPClient;

		bool		fSecure;
		const wchar_t	*fHost;
		unsigned short	fPort;

				Address(bool secure, const wchar_t host[], unsigned short port);
		};
	
	
	/*	Header
		Header to add to request
	*/
	struct Header {
		const wchar_t	*const fName,
				*const fValue;
		};
	
	
	/*	Request
		Represent body to HTTP request
		
		Yes, this could have been done as two functors passed to Request(); which effectively
		then creates two anonymous classes that don't share state.  That state sharing could
		have been done by functor captures to the enclosing function.  Not sure what that would
		gain over traditional subclassing other than possibly slightly less syntax?
	*/
	struct Rekwest {
		friend CHTTPClient;
	
	protected:
		// data immediately available
		const void	*fData;
		size_t		fDataL;
		
		/* Specifically not 'const' since computing the length may trigger generation of body data. */
		virtual size_t	Length() { return fDataL; }
		virtual void	Data(const std::function<void (const void*, size_t)>&) {};
		virtual void	Rewind() {};
	
	public:
				Rekwest(const void *data, size_t dataL) : fData(data), fDataL(dataL) {}
				Rekwest() : fData(nullptr), fDataL(0) {}
		virtual		~Rekwest() = default;
		};
	
	
	/*	Response
		Response to HTTP request
	*/
	struct Response {
		friend CHTTPClient;
		
		enum StandardHeader {
			kHeaderAllow = WINHTTP_QUERY_ALLOW
			};

		static constexpr wchar_t kHeaderDAV[] = L"DAV";

	protected:
		Win32::HTTP::Request &fRequest;
		
		
		explicit	Response(Win32::HTTP::Request&);
		
		size_t		Available() const { return fRequest.QueryDataAvailable(); }
		size_t		Read(void *buffer, size_t length) { return fRequest.Read(buffer, length); }
	
	public:
		Win32::Memory::Global Content() const;
		unsigned	GetLength(StandardHeader) const,
				GetLength(const wchar_t header[]) const;
		void		Get(StandardHeader header, char *buffer, unsigned bufferL) const,
				Get(const wchar_t header[], char *buffer, unsigned bufferL) const;
		};
	
	
	/*	InputAdapter
		Stream buffer adapter to be used with aistream/aostream
	*/
	struct InputAdapter;
	
	
	/*	InputAdapter
		Stream buffer adapters to be used with aistream/aostream
		
		These perform CR/LF conversion and wide character conversion.
		
		These come in 'char' and 'wchar_t' versions; the former works in the (multi)byte
		character set of the HTTP connection, while the latter converts to the native
		double-byte character set 
		
		The adapters may themselves be further refined by subclassing
	*/
	template <typename Char>
	struct DecodingInputAdapter;
	
	
	/*	OutputAdapter
		Stream buffer adapter to abstract over HTTP peculiarities
		
		Note that these don't contain any reference to the Win32 HTTP request handle:
		the presumption is that anyone using these streaming adapters won't generally know
		the exact content-length of the filtered data; and that to know that, the content
		will have to be entirely filtered in advance anyway.  So we take advantage of
		aostream's ability to buffer the entire filtered output.
	*/
	// struct OutputAdapter;
	
	template <typename Char>
	struct EncodingOutputAdapter;

protected:
	Win32::HTTP::Session fSession;
	Win32::HTTP::Connection fConnection;
	bool		fSecure;
	DWORD		fAuthenticationScheme;
	const wchar_t	*fUsername,
			*fPassword;

public:
			CHTTPClient(const Address&, const wchar_t username[], const wchar_t password[]);
			CHTTPClient(CHTTPClient&&);
	
	void		Request(
				const wchar_t	path[],
				const wchar_t	verb[],
				const std::function<void (const std::function<void (const wchar_t*, const wchar_t*)>&)> &Headers,
				Rekwest&&,
				const std::function<void (Response&)>&
				);
	};



/*

	CHTTPClient::InputAdapter

*/

struct CHTTPClient::InputAdapter {
protected:
	Win32::HTTP::Request &fRequest;

public:
			InputAdapter(CHTTPClient::Response &response) :
				fRequest(response.fRequest)
				{}
	
	size_t		available();
	size_t		house(char*, size_t);
	};


/*	available
	Return how many characters are available to be read from the associated character sequence
*/
inline size_t CHTTPClient::InputAdapter::available() {
	// return how much data is available on HTTP
	return fRequest.QueryDataAvailable();
	}


/*	house
	'House' data from the associated character sequence into the given buffer
*/
inline size_t CHTTPClient::InputAdapter::house(
	char		*buffer,
	size_t		bufferL
	) {
	return fRequest.Read(buffer, bufferL);
	}



/*

	CHTTPClient::DecodingInputAdapter<char>

*/

template<>
struct CHTTPClient::DecodingInputAdapter<char> : public CHTTPClient::InputAdapter {
	// fraction of buffer space that may be needed to properly filter some amount of input:
	static constexpr unsigned
			kOverflowNumerator = 1,
			kOverflowDenominator = 1;
	
	
			DecodingInputAdapter(CHTTPClient::Response &response) :
				CHTTPClient::InputAdapter(response)
				{}
	
	char		*filter(char *begin, char *end, const char *limit);
	};



/*

	CHTTPClient::DecodingInputAdapter<wchar_t>

*/

template<>
struct CHTTPClient::DecodingInputAdapter<wchar_t> {
protected:
	// narrow-character adapter and stream buffer that actually contains the user-defined filters
	aistreambuf<char, DecodingInputAdapter<char>> fNarrow;

public:
	// fraction of buffer space that may be needed to properly filter some amount of input (1/1)
	static constexpr unsigned
			kOverflowNumerator = 1,
			kOverflowDenominator = 1;
	
	
			DecodingInputAdapter(CHTTPClient::Response &response) :
				fNarrow(response)
				{}
	
	size_t		available();
	size_t		house(wchar_t*, size_t);
	wchar_t		*filter(wchar_t *begin, wchar_t *end, const wchar_t *limit) { return end; }
	};



/*

	CHTTPClient::EncodingOutputAdapter<char>

*/

template<>
struct CHTTPClient::EncodingOutputAdapter<char> /* : public CHTTPClient::InputAdapter */ {
public:
			EncodingOutputAdapter()	{}
	
	size_t		evict(const char*, size_t) { return 0; }
	char		*filter(char *begin, char *end, const char *limit);
	};



/*

	CHTTPClient::EncodingOutputAdapter<wchar_t>

*/

template<>
struct CHTTPClient::EncodingOutputAdapter<wchar_t> {
protected:
	// narrow-character adapter and stream buffer that actually contains the user-defined filters
	aostreambuf<char, EncodingOutputAdapter<char>> fNarrow;

public:
			EncodingOutputAdapter() {}
	
	aostreambuf<char, EncodingOutputAdapter<char>> &narrow() { return fNarrow; }
	
	size_t		evict(const wchar_t*, size_t);
	wchar_t		*filter(wchar_t *begin, wchar_t *end, const wchar_t *limit) { return end; }
	};




/*	CHTTPClientMock
	Mock HTTP client
*/
struct CHTTPClientMock {
public:
	/*	Response
		Mock response to HTTP request
		
		Must have an actual mock class here, since there doesn't appear to be a way to *set* fields
		in an actual WinHTTP response HANDLE
	*/
	struct Response {
		friend CHTTPClientMock;
		
		enum Header {
			kHeaderAllow,
			kHeaderDAV
			};
	
	protected:
		CHTTPClientMock &fClient;
		std::map<Header, std::string> fHeaders;
		size_t		fRead;
		
		
				Response(CHTTPClientMock&, std::map<Header, std::string> &&headers);
		
		size_t		Available() const;
		size_t		Read(void *buffer, size_t length);
	
	public:
		HGLOBAL		Content() const { return fClient.fBody; }
		unsigned	GetLength(Header) const;
		void		Get(Header, char *buffer, unsigned bufferL) const;
		};
	
	typedef CHTTPClient::Rekwest Rekwest;

protected:
	Win32::Memory::Global fBody;

public:
	explicit	CHTTPClientMock(const CHTTPClient::Address&, const wchar_t username[], const wchar_t password[]);

	void		Request(
				const wchar_t	path[],
				const wchar_t	verb[],
				const std::function<void (const std::function<void (const wchar_t*, const wchar_t*)>&)> &Headers,
				Rekwest&&,
				const std::function<void (Response&)>&
				);
	};



/*	Address
	Represent the Internet address of a server
	If 'port' is zero, the well-known default is assumed
*/
inline CHTTPClient::Address::Address(
	bool		secure,
	const wchar_t	host[],
	unsigned short	port
	) :
	fSecure(secure),
	fHost(host),
	fPort(
		port != 0 ? port :
		secure ? 443 : 80
		)
	{}


/*	Response
	Represent the response to an HTTP request
*/
inline CHTTPClient::Response::Response(
	Win32::HTTP::Request &request
	) :
	fRequest(request)
	{}
