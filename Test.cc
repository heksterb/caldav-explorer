#include <iostream>
#include <string>

#include "CppUnitTest.h"

#include "DAV.h"
#include "HTTPClient.h"
#include "HTTPStreamBuf.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(TestDAV) {
public:
	TestDAV() {
		// set current directory to where the test data is
		// *** somehow pick this up from environment or such
		if (!SetCurrentDirectory(L"L:\\\\Ben\\Documents\\Projects\\Time Management\\")) throw GetLastError();
		}


	TEST_METHOD(ServerOptionsNamecheap) {
		CHTTPClientMock http(
			CHTTPClient::Address(true, CFSTR("server213-1.web-hosting.com"), 2080),
			L"cheap.xml", L""
			);

		DAV<CHTTPClientMock> dav(http);
		dav.GetServerOptions(
			L"",
			[](const char allow[], const char dav[]) {
				/* This isn't required to be called; just a quick-and-easy utility */
				DAV<CHTTPClientMock>::Allow a(allow);
				Assert::IsTrue(a.fOptions);
				Assert::IsFalse(a.fCopy);
		
				DAV<CHTTPClientMock>::Capabilities c(dav);
				Assert::IsTrue(c.fOne);
				Assert::IsFalse(c.fTwo);
				}
			);
		}


	TEST_METHOD(ServerOptionsICloud) {
		CHTTPClientMock http(
			CHTTPClient::Address(true, CFSTR("caldav.icloud.com"), 443),
			L"icloud.xml", L""
			);
		
		DAV<CHTTPClientMock> dav(http);
		dav.GetServerOptions(
			L"",
			[](const char allow[], const char dav[]) {
				/* This isn't required to be called; just a quick-and-easy utility */
				DAV<CHTTPClientMock>::Allow a(allow);
				Assert::IsTrue(a.fOptions);
				Assert::IsFalse(a.fCopy);
		
				DAV<CHTTPClientMock>::Capabilities c(dav);
				Assert::IsTrue(c.fOne);
				Assert::IsFalse(c.fTwo);
				}
			);
		}


	/*	StreamResponse
		Test the iostream-based HTTP response mechanism
	*/
	TEST_METHOD(StreamResponse) {
		CHTTPClient http(
			CHTTPClient::Address(false, CFSTR("www.hekster.org"), 80),
			L"", L""
			);
		
		http.Request(
			L"/",
			L"GET",
			[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
			nullptr, 0,
			[](CHTTPClient::Response &response) {
				httpstreambuf<> isb(response);
				std::istream is(&isb);

				bool foundBody = false;
				unsigned linesN = 0;
				for (std::string line; std::getline(is, line);) {
					if (line == "<BODY>") foundBody = true;
					
					// count number of lines
					linesN++;
					}
				Assert::IsTrue(foundBody);
				Assert::IsTrue(linesN == 83);
				}
			);
		}
	};
