/*
	HTTPClient
	
	HTTP experimentation
	Win32
	
	2018/09/09	Originated
	
	Copyright © 2018-2023 by: Ben Hekster
*/

#define _CRT_SECURE_NO_WARNINGS

#include <cassert>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>

#include <STRINGAPISET.H>

#include "NFile.h"

#include "HTTPClient.h"


/*

	CHTTPClient

*/

/*	CHTTPClient
	Connect to an HTTP server
*/
CHTTPClient::CHTTPClient(
	const Address	&server,
	const wchar_t	username[],
	const wchar_t	password[]
	) :
	// create HTTP session
	fSession(L"Casaubon User Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0),

	// establish connection context
	fConnection(fSession, server.fHost, server.fPort),

	fSecure(server.fSecure),
	fAuthenticationScheme(0),
	fUsername(username),
	fPassword(password)
{
}


/*	CHTTPClient
	Move constructor
*/
CHTTPClient::CHTTPClient(
	CHTTPClient	&&that
	) :
	fSession(std::move(that.fSession)),
	fConnection(std::move(that.fConnection)),
	fSecure(that.fSecure),
	fAuthenticationScheme(that.fAuthenticationScheme),
	fUsername(that.fUsername),
	fPassword(that.fPassword)
{
}


/*	Request
	Issue an HTTP request on the session
*/
void CHTTPClient::Request(
	const wchar_t	path[],
	const wchar_t	verb[],
	const std::function<void (const std::function<void (const wchar_t*, const wchar_t*)>&)> &Headers,
	Rekwest		&&rekwest,
	const std::function<void (Response&)> &AcceptResponse
	)
{
// prepare request
Win32::HTTP::Request request(
	fConnection,
	verb,
	path,
	nullptr /* default version */,
	nullptr /* referrer */,
	nullptr /* default accept types */,
	fSecure * WINHTTP_FLAG_SECURE
	);

// construct additional headers string
std::optional<std::wstring> headers;
Headers(
	[&headers](const wchar_t *name, const wchar_t *value) {
		if (!headers)
			headers.emplace();
		
		headers->append(name);
		headers->append(L": ");
		headers->append(value);
		headers->append(L"\r\n");
		}
	);

// last character should not be NUL
/* (iCloud at least will return 405 Bad Request) */
if (rekwest.fData && rekwest.fDataL > 0)
	assert(static_cast<const char*>(rekwest.fData)[rekwest.fDataL - 1] != '\0');

// keep retrying with corrective actions
bool retry;
do {
	retry = false;
	
	// need to authenticate?
	/* If we previously received HTTP_STATUS_DENIED and authenticated, it seems that we have to proactively do its
	   here; otherwise we get an undebuggable ERROR_WINHTTP_INVALID_SERVER_RESPONSE in WinHttpReceiveResponse() */
	if (fAuthenticationScheme)
		// basic authentication? (is OK as long as we're using HTTPS)
		if (fAuthenticationScheme & WINHTTP_AUTH_SCHEME_BASIC)
			request.SetCredentials(WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, fUsername, fPassword);
		
		else
			throw "unsupported authentication scheme";
	
	// find total length of request body
	/* In some cases, calculating this may force the user to generate the entire request body.
	   We attempt to avoid it, so we don't have to buffer the entire body in memory and thereby reduce latency. */
	size_t length = rekwest.Length();
	
	// send request with any initial body data
	request.Send(headers ? headers->c_str() : nullptr, rekwest.fData, rekwest.fDataL, length, 0 /* context */);
	assert(length >= rekwest.fDataL);
	length -= rekwest.fDataL;
	
	// stream remainder of body
	while (length > 0) {
		DebugBreak();
		
		// ask user to push more data
		rekwest.Data(
			[&request, &length](
				const void	*data,
				size_t		dataL
				) {
				// write the additional data through HTTP
				request.Write(data, dataL);
				
				// account
				assert(length >= dataL);
				length -= dataL;
				}
			);
		}
	
	// receive response
	request.Receive();
	switch (const unsigned status = request.QueryHeaderAsUnsigned(WINHTTP_QUERY_STATUS_CODE)) {
		// *** not really sure how to properly handle all these different codes in terms of a function result
		case HTTP_STATUS_OK:
		case HTTP_STATUS_CREATED:
		case HTTP_STATUS_NO_CONTENT:
		case HTTP_STATUS_WEBDAV_MULTI_STATUS:
			break;
		
		// need authentication?
		case HTTP_STATUS_DENIED:
			if (fAuthenticationScheme) throw "can't log in even after authenticating";
			fAuthenticationScheme = request.QueryAuthSchemes().supportedSchemes;
			retry = true;
			break;
		
		case HTTP_STATUS_BAD_REQUEST:
		case HTTP_STATUS_SERVICE_UNAVAIL:
		case HTTP_STATUS_NOT_FOUND:
		case HTTP_STATUS_BAD_METHOD:
		case HTTP_STATUS_SERVER_ERROR:
			throw status;		// *** probably wrap this in an HTTP exception class
		
		default:
			// *** sometimes even an HTTP error will have a (useful) response body (eg, 415)
			DebugBreak();
		}
	
	// must rewind the body data
	rekwest.Rewind();
	} while (retry);

// call back *** maybe skip it with 204 No Content
Response response(request);
AcceptResponse(response);
}



/*

	CHTTPClient::Response

*/

/*	Content
	Get the response body as an HGLOBAL
*/
Win32::Memory::Global CHTTPClient::Response::Content() const
{
// allocate a global heap handle with the expected length of the response
/* CreateStreamOnHGlobal() requires it to be a movable HGLOBAL */
/* Although I think HTTP requires Content-Length in the response, we can be resilient against not receiving it.
   For instance, iCloud PROPFIND responses don't have it. */
const std::optional<unsigned> expected = fRequest.QueryHeaderAsUnsignedOptional(WINHTTP_QUERY_CONTENT_LENGTH);
unsigned
	size = expected ? *expected : 0x100,			// size of handle *** can't allocate zero-length handle?
	length = 0;						// length of data
Win32::Memory::Global handle(size, GMEM_MOVEABLE);

/* WinHttpQueryDataAvailable blocks until data available, or EOF */
while (const unsigned long available = fRequest.QueryDataAvailable()) {
	// not enough space?
	if (const unsigned need = length + available; need > size) {
		// reallocate
		handle.Reallocate(need, 0 /* OK to mean no changes? */);
		size = handle.Size(); // may be more than requested
		}

	// lock
	Win32::Memory::Global::Lok data(handle);

	// read data
	length += fRequest.Read(static_cast<char*>(data.operator void*()) + length, size - length);
	}

// trim handle size to length of data
if (size != length) handle.Reallocate(length, 0);

// return the body handle
return handle;
}


/*	GetLength
	Get the length of a response header string
*/
unsigned CHTTPClient::Response::GetLength(
	StandardHeader	header
	) const {
	// get the wide-character header
	const unsigned headerWideL = static_cast<unsigned>(fRequest.QueryHeaderL(header)); // returns size including NUL terminator
	wchar_t *const headerWide = static_cast<wchar_t*>(alloca(headerWideL));
	const unsigned headerWideL0 = fRequest.QueryHeader(header, headerWide, headerWideL); // returns size without the terminator
	assert(headerWideL0 + sizeof(wchar_t) == headerWideL);

	// calculate the number of bytes needed for the result string (again, without the terminator)
	const int headerL = WideCharToMultiByte(
		65001 /* UTF-8 */,
		WC_ERR_INVALID_CHARS,
		headerWide, -1 /* process until NUL terminator */,
		nullptr, 0,
		nullptr, /* use system default character */
		nullptr /* used default character */
		); if (!headerL) throw GetLastError();

	return headerL;
	}

unsigned CHTTPClient::Response::GetLength(
	const wchar_t	header[]
	) const {
	// get the wide-character header
	const unsigned headerWideL = static_cast<unsigned>(fRequest.QueryHeaderLByName(0, header)); // returns size including NUL terminator
	wchar_t *const headerWide = static_cast<wchar_t*>(alloca(headerWideL));
	const unsigned headerWideL0 = fRequest.QueryHeaderByName(0, headerWide, headerWideL, header); // returns size without the terminator
	assert(headerWideL0 + sizeof(wchar_t) == headerWideL);

	// calculate the number of bytes needed for the result string (again, without the terminator)
	const int headerL = WideCharToMultiByte(
		65001 /* UTF-8 */,
		WC_ERR_INVALID_CHARS,
		headerWide, -1 /* process until NUL terminator */,
		nullptr, 0,
		nullptr, /* use system default character */
		nullptr /* used default character */
		); if (!headerL) throw GetLastError();

	return headerL;
	}


/*	Get
	Get a response header string
*/
void CHTTPClient::Response::Get(
	StandardHeader	header,
	char		*const buffer,
	const unsigned	bufferL
	) const {
	// get the wide-character header
	unsigned headerWideL = static_cast<unsigned>(fRequest.QueryHeaderL(header)); // returns size including NUL terminator
	wchar_t *const headerWide = static_cast<wchar_t*>(alloca(headerWideL));
	const unsigned headerWideL0 = fRequest.QueryHeader(header, headerWide, headerWideL); // returns size without the terminator
	assert(headerWideL0 + sizeof(wchar_t) == headerWideL);
		
	// convert to narrow string
	if (!WideCharToMultiByte(
		65001 /* UTF-8 */,
		WC_ERR_INVALID_CHARS,
		headerWide, -1 /* process until NUL terminator */,
		buffer, bufferL,
		nullptr, /* use system default character */
		nullptr /* used default character */
		)) throw GetLastError();
	}

void CHTTPClient::Response::Get(
	const wchar_t	header[],
	char		*const buffer,
	const unsigned	bufferL
	) const {
	// get the wide-character header
	unsigned headerWideL = static_cast<unsigned>(fRequest.QueryHeaderLByName(0, header)); // returns size including NUL terminator
	wchar_t *const headerWide = static_cast<wchar_t*>(alloca(headerWideL));
	const unsigned headerWideL0 = fRequest.QueryHeaderByName(0, headerWide, headerWideL, header); // returns size without the terminator
	assert(headerWideL0 + sizeof(wchar_t) == headerWideL);
	
	// convert to narrow string
	if (!WideCharToMultiByte(
		65001 /* UTF-8 */,
		WC_ERR_INVALID_CHARS,
		headerWide, -1 /* process until NUL terminator */,
		buffer, bufferL,
		nullptr, /* use system default character */
		nullptr /* used default character */
		)) throw GetLastError();
	}



/*

	CHTTPClient::InputAdapter

*/

/*	filter

*/
char *CHTTPClient::DecodingInputAdapter<char>::filter(
	char		*begin,
	char		*end,
	const char	*limit
	)
{
Splicer<char> ci(begin, end, limit);

// for each character of the buffer
while (ci)
	switch (const char c = ci.read()) {
		// CR?  Assume CR/LF always appear in pairs, so just remove it
		case '\r':	break;
		
		default:	ci << c; break;
		}

return ci.end();
}


/*	available
	Return how many characters are available to be read from the associated character sequence
*/
size_t CHTTPClient::DecodingInputAdapter<wchar_t>::available()
{
size_t result;

// flood narrow character input stream buffer
fNarrow.pubsync();

// any data available to convert?
if (const size_t narrowL = fNarrow.size()) {
	// calculate length of conversion from UTF-8
	const int wideL = MultiByteToWideChar(
		65001 /* UTF-8 */,
		MB_ERR_INVALID_CHARS,
		fNarrow.data(), fNarrow.size(),
		nullptr, 0 // compute required output length
		); if (!wideL) throw GetLastError(); // ***** errors
	
	result = wideL;
	}

else
	result = 0;

return result;
}


/*	house
	Make input characters available at the given buffer; return the number of
	characters produced
*/
size_t CHTTPClient::DecodingInputAdapter<wchar_t>::house(
	wchar_t		*buffer,
	size_t		bufferL
	)
{
size_t result;

// any data available to convert?
if (const size_t narrowL = fNarrow.size()) {
	// convert from UTF-8
	const int wideL = MultiByteToWideChar(
		65001 /* UTF-8 */,
		MB_ERR_INVALID_CHARS,
		fNarrow.data(), narrowL,
		buffer, bufferL
		); if (!wideL) throw GetLastError(); // ***** errors

	// narrow-character data was consumed
	fNarrow.claimed(narrowL);
	
	result = wideL;
	}

else
	result = 0;

return result;
}



/*

	CHTTPClient::OutputAdapter

*/

/*	evict
	Transform buffered wide-character data into the narrow-character buffer
*/
size_t CHTTPClient::EncodingOutputAdapter<wchar_t>::evict(
	const wchar_t	*data,
	size_t		dataL
	)
{
// convert to UTF-8
char *narrow = nullptr;
int narrowL = 0;
do {
	// create space to convert into if size is known (second iteration)
	if (narrowL) narrow = fNarrow.reserve(narrowL);
	
	// compute required buffer size (first iteration) or perform actual conversion (second iteration)
	narrowL = WideCharToMultiByte(
		65001 /* UTF-8 */,
		WC_ERR_INVALID_CHARS,
		data, dataL,
		narrow, narrowL,
		nullptr, /* use system default character */
		nullptr /* used default character */
		); if (!narrowL) throw GetLastError(); // ***** errors
	
	// while conversion not completed yet
	} while (!narrow);

// data was provided
fNarrow.used(narrowL);

return dataL;
}


/*	filter
	Process data to make it suitable for eviction
*/
char *CHTTPClient::EncodingOutputAdapter<char>::filter(
	char		*begin,
	char		*end,
	const char	*limit
	)
{
Splicer<char> ci(begin, end, limit);

// for each character of the buffer
while (ci)
	switch (const char c = ci.read()) {
		// CR?
		/* We're assuming a 'C' style input containing only LF.  But just in case we do encounter CRLF,
		   just consume CR now; and re-insert it when we see the LF.  This way we don't have to manage
		   any other state in the conversion. */
		case '\r':	break;
		
		// LF?
		case '\n':	ci << '\r' << '\n'; break;
		
		default:	ci << c; break;
		}

return ci.end();
}



/*

	CHTTPClientMock

*/

/*	ReadFileIntoHandle
	Read the contents of the file with the given name into a global handle
*/
static Win32::Memory::Global ReadFileIntoHandle(
	const wchar_t	name[]
	)
{
// open the file containing the body
Win32::Fork fork(
	CreateFile(
		name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr
		)
	);

// get length of file
const unsigned bodyL = fork.FileSize<unsigned>();

// allocate handle to contain body
Win32::Memory::Global handle(bodyL, GMEM_MOVEABLE);

// read file into handle in one go
if (fork.Read(Win32::Memory::Global::Lok(handle), bodyL) != bodyL) throw "unexpected";

return handle;
}


/*	CHTTPClientMock
	Construct a mock HTTP client
*/
CHTTPClientMock::CHTTPClientMock(
	const CHTTPClient::Address &server,
	const wchar_t	username[],
	const wchar_t	password[]
	) :
	fBody(ReadFileIntoHandle(username))
{
}


/*	Request
	Issue an HTTP request on the session
*/
void CHTTPClientMock::Request(
	const wchar_t	path[],
	const wchar_t	verb[],
	const std::function<void (const std::function<void (const wchar_t*, const wchar_t*)>&)> &Headers,
	Rekwest&&,
	const std::function<void (Response&)> &AcceptResponse
	)
{
Response response(
	*this,
	// 207, L"multistatus blah blah"
	{
		{ Response::kHeaderAllow, "OPTIONS" },
		{ Response::kHeaderDAV, "1" }
		}
	);

AcceptResponse(response);
}


/*	Response
	Represent a mock response
*/
CHTTPClientMock::Response::Response(
	CHTTPClientMock	&client,
	std::map<Header, std::string> &&headers
	) :
	fClient(client),
	fHeaders(std::move(headers)),
	fRead(0)
{
}


/*	Available
	Return number of bytes available to read
*/
size_t CHTTPClientMock::Response::Available() const
{
// return the length of the response body data
return fClient.fBody.Size() - fRead;
}


/*	Read
	Read data from the response
*/
size_t CHTTPClientMock::Response::Read(
	void		*buffer,
	size_t		length
	)
{
// reduce to amount that is actually available
length = std::min(length, fClient.fBody.Size() - fRead);

// return data
Win32::Memory::Global::Lok lock(fClient.fBody);
(void) memcpy(buffer, static_cast<char*>(lock.operator void*()) + fRead, length);

fRead += length;
return length;
}


/*	GetLength
	Get the length of a response header string
*/
unsigned CHTTPClientMock::Response::GetLength(
	Header		header
	) const
{
// look for the header
const auto found = fHeaders.find(header);
if (found == fHeaders.end()) throw ERROR_WINHTTP_HEADER_NOT_FOUND;

// return its length
return found->second.length();
}


/*	Get
	Get a response header string
*/
void CHTTPClientMock::Response::Get(
	Header		header,
	char		*buffer,
	unsigned	bufferL
	) const
{
// look for the header
const auto found = fHeaders.find(header);
if (found == fHeaders.end()) throw ERROR_WINHTTP_HEADER_NOT_FOUND;

// return it
const unsigned length = found->second.length();
if (bufferL < length + 1) throw ERROR_INSUFFICIENT_BUFFER;
(void) strncpy(buffer, found->second.c_str(), bufferL);
}
