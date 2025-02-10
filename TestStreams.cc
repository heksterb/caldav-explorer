#include <iostream>
#include <string>

#include "CppUnitTest.h"

#include "HTTPClient.h"
#include "HTTPStreamBuf.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(TestStreams) {
public:
	TestStreams() {
		// set current directory to where the test data is
		// *** somehow pick this up from environment or such
		if (!SetCurrentDirectory(L"L:\\\\Ben\\Documents\\Projects\\Time Management\\")) throw GetLastError();
		}
	
	
	/*	StreamHTTP
		Read HTTP response as a std::istream
	*/
	TEST_METHOD(StreamHTTP) {
		CHTTPClientMock http(
			CHTTPClient::Address(true, CFSTR("server213-1.web-hosting.com"), 2080),
			L"streams.xml", L""
			);
		
		http.Request(
			L"/",
			L"GET",
			[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
			nullptr, 0,
			[](CHTTPClientMock::Response &response) {
				httpstreambuf<CHTTPClientMock> isb(response);
				std::istream is(&isb);
				
				static const std::string expectation(
					R"(<?xml version="1.0" encoding="utf-8"?>)" "\r\n"
					R"(<multistatus>)" "\r\n"
					R"(test)" "\r\n"
					R"(</multistatus>)" "\r\n"
					);
				const std::string s = static_cast<const std::ostringstream&>(std::ostringstream() << is.rdbuf()).str();
				Assert::AreEqual(s, expectation);
				}
			);
		}
	
	
	/*	StreamCRLF
		Read HTTP response as a std::istream, converting CR/LF to '\n'
	*/
	TEST_METHOD(StreamCRLF) {
		CHTTPClientMock http(
			CHTTPClient::Address(true, CFSTR("server213-1.web-hosting.com"), 2080),
			L"streams.xml", L""
			);
		
		http.Request(
			L"/",
			L"GET",
			[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
			nullptr, 0,
			[](CHTTPClientMock::Response &response) {
				crlfstreambuf<httpstreambuf<CHTTPClientMock>> isb(response);
				std::istream is(&isb);
				
				static const std::string expectation(
					R"(<?xml version="1.0" encoding="utf-8"?>)" "\n"
					R"(<multistatus>)" "\n"
					R"(test)" "\n"
					R"(</multistatus>)" "\n"
					);
				const std::string s = static_cast<const std::ostringstream&>(std::ostringstream() << is.rdbuf()).str();
				Assert::AreEqual(s, expectation);
				}
			);
		}
	
	
	/*	StreamMultibyte
		Read HTTP response as a std::wistream, converting CR/LF to '\n'
	*/
	TEST_METHOD(StreamMultibyte) {
		CHTTPClientMock http(
			CHTTPClient::Address(true, CFSTR("server213-1.web-hosting.com"), 2080),
			L"streams.xml", L""
			);
		
		http.Request(
			L"/",
			L"GET",
			[](const std::function<void (const wchar_t*, const wchar_t*)>&) {},
			nullptr, 0,
			[](CHTTPClientMock::Response &response) {
				wnstreambuf<crlfstreambuf<httpstreambuf<CHTTPClientMock>>> isb(response);
				std::wistream is(&isb);
				
				static const std::wstring expectation(
					LR"(<?xml version="1.0" encoding="utf-8"?>)" L"\n"
					LR"(<multistatus>)" L"\n"
					LR"(test)" L"\n"
					LR"(</multistatus>)" L"\n"
					);
				const std::wstring s = static_cast<const std::wostringstream&>(std::wostringstream() << is.rdbuf()).str();
				Assert::AreEqual(s, expectation);
				}
			);
		}
	};
