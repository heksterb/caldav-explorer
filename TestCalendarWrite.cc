#include <WINDEF.H>
#include <WINBASE.H>
#include <WINNT.H>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <string>

#include "CppUnitTest.h"
#include "Dynamic.h"
#include "HTTPStreamBuf.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(TestCalendarWrite) {
public:
	static std::set<std::wstring> CollectLines(std::wistream&);
	static void	Compare(const char name[]);
	
	
	TestCalendarWrite() {
		// set current directory to where the test data is
		// *** somehow pick this up from environment or such
		if (!SetCurrentDirectory(L"L:\\\\Ben\\Documents\\Projects\\Time Management\\")) throw GetLastError();
		}
	
	TEST_METHOD(CompareCheapEvent2) {
		Compare("7b84cd2d-8cd9-4ed4-82bc-89fe145a001e.ics");
		}
	
	TEST_METHOD(CompareCheapTodo) {
		Compare("D4DC6FDA-667C-461B-8681-2025FA436BAE.ics");
		}
	
	TEST_METHOD(CompareICloudToDo) {
		Compare("F5065FD0-2F27-407C-86AC-B9AD321F2B3A.ics");
		}
	
	TEST_METHOD(CompareICloudToDoAlarm) {
		Compare("22D6CABC-E414-435F-881A-41B0178FD7E9.ics");
		}
	};


/*	CollectLines

*/
std::set<std::wstring> TestCalendarWrite::CollectLines(
	std::wistream	&input
	)
{
std::set<std::wstring> lines;

// for each line in the input stream
for (std::wstring line; std::getline(input, line); )
	lines.emplace(line);

return lines;
}


/*	Compare

*/
void TestCalendarWrite::Compare(
	const char	name[]
	)
{
std::set<std::wstring> reference; {
	// open the source calendar object as a filtered stream
	wnstreambuf<icalstreambuf<std::filebuf>> ifsbuf;
	ifsbuf.narrowbuf().open(name, std::ios_base::in);
	std::wistream ifs(&ifsbuf);
	
	// collect the filtered lines into the reference set
	reference = CollectLines(ifs);
	
	// we erase SEQUENCE:0; that is the default
	if (const auto &found = reference.find(L"SEQUENCE:0"); found != reference.cend())
		reference.erase(found);

	if (const auto &found = reference.find(L"TRANSP:OPAQUE"); found != reference.cend())
		reference.erase(found);
	
	std::wregex
		trigger(L"TRIGGER.*;VALUE=DURATION"),
		valueDuration(L";VALUE=DURATION");
	for (std::set<std::wstring>::iterator rp = reference.begin(); rp != reference.end();)
		if (std::regex_search(*rp, trigger)) {
			std::wstring replace = std::regex_replace(*rp, valueDuration, std::wstring());
			rp = reference.erase(rp);
			reference.insert(replace);
			}

		else
			++rp;
	}

std::set<std::wstring> output; {
	// open the source calendar object as a filtered stream
	wnstreambuf<icalstreambuf<std::filebuf>> ifsbuf;
	ifsbuf.narrowbuf().open(name, std::ios_base::in);
	std::wistream ifs(&ifsbuf);
	
	// parse into our calendar object
	DynamicCalendar<> calendarItem;
	DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
	
	// emit the calendar object into a stream of lines
	std::wstringstream emitted;
	emitted << calendarItem;
	
	// collect the lines into the output set
	output = CollectLines(emitted);
	}

// make sure every line in the reference object set is also in our output set
std::set<std::wstring> referenceNotOutput;
std::set_difference(
	reference.cbegin(), reference.cend(),
	output.cbegin(), output.cend(),
	std::inserter(referenceNotOutput, referenceNotOutput.begin())
	);

// make sure every line in out output set was also present in the reference object
std::set<std::wstring> outputNotReference;
std::set_difference(
	output.cbegin(), output.cend(),
	reference.cbegin(), reference.cend(),
	std::inserter(outputNotReference, outputNotReference.begin())
	);

Assert::IsTrue(referenceNotOutput.size() == 0);
Assert::IsTrue(outputNotReference.size() == 0);
}
