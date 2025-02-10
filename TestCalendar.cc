#include <WINDEF.H>
#include <WINBASE.H>
#include <WINNT.H>

#include <malloc.h>
#include <fstream>

#include "CppUnitTest.h"
#include "Dynamic.h"
#include "HTTPClient.h"
#include "HTTPStreamBuf.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(TestCalendar) {
public:
	TestCalendar() {
		// set current directory to where the test data is
		// *** somehow pick this up from environment or such
		if (!SetCurrentDirectory(L"L:\\\\Ben\\Documents\\Projects\\Time Management\\")) throw GetLastError();
		}
	
	
	TEST_METHOD(ParseCheapTodo) {
		std::wifstream ifs("D4DC6FDA-667C-461B-8681-2025FA436BAE.ics");
		Assert::IsTrue(ifs.is_open());
		
		DynamicCalendar<> calendarItem;
		DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
		
		DynamicCalendar<>::ToDo &todo = std::get<DynamicCalendar<>::ToDo>(calendarItem.fComponents[0]);
		Assert::IsTrue(todo.fUID == L"D4DC6FDA-667C-461B-8681-2025FA436BAE");
		Assert::IsTrue(todo.fSummary == L"My Task");
		Assert::IsTrue(todo.fPriority == 5);
		}
	
	TEST_METHOD(ParseCheapEvent1) {
		std::wifstream ifs("84F34FBF-E678-4D49-AA9D-AAB54221332C.ics");
		Assert::IsTrue(ifs.is_open());
		
		DynamicCalendar<wchar_t> calendarItem;
		DynamicCalendar<wchar_t>::Parser(calendarItem, ifs).operator()();
		
		DynamicCalendar<wchar_t>::Event &event = std::get<DynamicCalendar<>::Event>(calendarItem.fComponents[0]);
		Assert::IsTrue(event.fUID == L"84F34FBF-E678-4D49-AA9D-AAB54221332C");
		Assert::IsTrue(event.fSummary == L"On phone");
		Assert::IsTrue(event.fClassification == DynamicCalendar<>::Classification::kPublic);
		Assert::IsTrue(event.fStatus == DynamicCalendar<>::StatusEvent::kConfirmed);
		Assert::IsTrue(event.fTransparency == DynamicCalendar<>::Transparency::kOpaque);
		
		DynamicCalendar<>::TimeZone &timeZone = std::get<DynamicCalendar<>::TimeZone>(calendarItem.fComponents[1]);
		Assert::IsTrue(timeZone.fID == L"America/Los_Angeles");
		Assert::IsTrue(timeZone.fDivisions.size() == 18);
		}
	
	TEST_METHOD(ParseCheapEvent2) {
		std::wifstream ifs("7b84cd2d-8cd9-4ed4-82bc-89fe145a001e.ics");
		Assert::IsTrue(ifs.is_open());
		
		DynamicCalendar<> calendarItem;
		DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
		
		DynamicCalendar<>::Event &event = std::get<DynamicCalendar<>::Event>(calendarItem.fComponents[0]);
		Assert::IsTrue(event.fUID == L"7b84cd2d-8cd9-4ed4-82bc-89fe145a001e");
		Assert::IsTrue(event.fSummary == L"Stella");
		Assert::IsTrue(event.fClassification == DynamicCalendar<>::Classification::kPrivate);
		}
	
	TEST_METHOD(ParseICloudEvent) {
		wnstreambuf<icalstreambuf<std::filebuf>> ifsbuf;
		ifsbuf.narrowbuf().open("01814FE0-6735-42DF-B4A3-D31427200BEE.ics", std::ios_base::in);
		Assert::IsTrue(ifsbuf.narrowbuf().is_open());
		std::wistream ifs(&ifsbuf);
		
		DynamicCalendar calendarItem;
		DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
		
		Assert::IsTrue(calendarItem.fScale == DynamicCalendar<>::Scale::kGregorian);
		
		DynamicCalendar<>::Event &event = std::get<DynamicCalendar<>::Event>(calendarItem.fComponents[0]);
		Assert::IsTrue(event.fUID == L"01814FE0-6735-42DF-B4A3-D31427200BEE");
		// ***** the escapes should also be processed
		Assert::IsTrue(event.fLocation == L"Kaiser Permanente Fremont Medical Center\\, Niles #307\\n\\, 39400 Paseo Padre Pkwy\\, Fremont\\, CA  94538");
		Assert::IsTrue(event.fSequence == 0);
		}
	
	TEST_METHOD(ParseICloudToDo) {
// ***** should save/open these as UTF-16??
		wnstreambuf<icalstreambuf<std::filebuf>> ifsbuf;
		ifsbuf.narrowbuf().open("F5065FD0-2F27-407C-86AC-B9AD321F2B3A.ics", std::ios_base::in);
		Assert::IsTrue(ifsbuf.narrowbuf().is_open());
		std::wistream ifs(&ifsbuf);
		
		DynamicCalendar calendarItem;
		DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
		
		Assert::IsTrue(calendarItem.fScale == DynamicCalendar<>::Scale::kGregorian);
		
		DynamicCalendar<>::ToDo &todo = std::get<DynamicCalendar<>::ToDo>(calendarItem.fComponents[0]);
		Assert::IsTrue(todo.fSummary == L"Midi solution");
		Assert::IsTrue(todo.fStatus == DynamicCalendar<>::StatusToDo::kNeedsAction);
		}
	
	
	TEST_METHOD(ParseICloudToDoAlarm) {
		wnstreambuf<icalstreambuf<std::filebuf>> ifsbuf;
		ifsbuf.narrowbuf().open("22D6CABC-E414-435F-881A-41B0178FD7E9.ics", std::ios_base::in);
		Assert::IsTrue(ifsbuf.narrowbuf().is_open());
		std::wistream ifs(&ifsbuf);
		
		DynamicCalendar calendarItem;
		DynamicCalendar<>::Parser(calendarItem, ifs).operator()();
		
		Assert::IsTrue(calendarItem.fScale == DynamicCalendar<>::Scale::kGregorian);
		
		DynamicCalendar<>::ToDo &todo = std::get<DynamicCalendar<>::ToDo>(calendarItem.fComponents[0]);
		Assert::IsTrue(todo.fSummary == L"Blah");
		Assert::IsTrue(todo.fStatus == DynamicCalendar<>::StatusToDo::kNeedsAction);
		
		DynamicCalendar<>::TimeZone &timeZone = std::get<DynamicCalendar<>::TimeZone>(calendarItem.fComponents[1]);
		Assert::IsTrue(timeZone.fID == L"America/Los_Angeles");
		
		DynamicCalendar<>::TimeZone::Division &division = timeZone.fDivisions[0];
		Assert::IsTrue(division.fKind == DynamicCalendar<>::TimeZone::Division::kStandard);
		DynamicCalendar<>::DateTime &divisionStart = std::get<DynamicCalendar<>::DateTime>(division.fStart);
		Assert::IsTrue(divisionStart.fDate.fYear == 1883);
		Assert::IsTrue(divisionStart.fDate.fMonth0 == 10);
		Assert::IsTrue(divisionStart.fDate.fDay0 == 17);
		Assert::IsTrue(divisionStart.fTime.fHour == 12);
		Assert::IsTrue(divisionStart.fTime.fMinute == 7);
		Assert::IsTrue(divisionStart.fTime.fSecond == 2);
		Assert::IsTrue(divisionStart.fTime.fZone == DynamicCalendar<>::Time::Zone::kNone);
		// *** RDATE
		Assert::IsTrue(division.fName == L"PST");
		Assert::IsTrue(division.fOffsetFrom.has_value());
		Assert::IsTrue(division.fOffsetFrom->fHour == -7);
		Assert::IsTrue(division.fOffsetFrom->fMinute == 52);
		Assert::IsTrue(division.fOffsetFrom->fSecond == 58);
		Assert::IsTrue(division.fOffsetTo.has_value());
		Assert::IsTrue(division.fOffsetTo->fHour == -8);
		Assert::IsTrue(division.fOffsetTo->fMinute == 0);
		Assert::IsTrue(division.fOffsetTo->fSecond == 0);
		}
	};


TEST_CLASS(TestCalendarFilter) {
protected:
	struct Chunk {
		const char	*input;
		const char	*output;
		};
	
	static void	Run(const Chunk[], unsigned chunksN);

public:
	TEST_METHOD(Idempotent) {
		static const Chunk chunks[] = {
			{ "ab c", "ab c" }
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	
	
	TEST_METHOD(DontFilterCR) {
		static const Chunk chunks[] = {
			{ "ab\nc", "ab\nc" }
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	
	
	TEST_METHOD(DoFilterCR) {
		static const Chunk chunks[] = {
			{ "ab\n  c", "ab c" }
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	
	
	TEST_METHOD(DoFilterCRAtEnd) {
		static const Chunk chunks[] = {
			{ "abc\n", "abc" },
			{ " def", "def" }
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	
	
	TEST_METHOD(DontFilterCRAtEndFits) {
		static const Chunk chunks[] = {
			{ "abc\n", "abc" },		// consumes '\n' because must presume it's not 'real'
			{ "de\n f", "\ndef" }		// must reinsert '\n'; fortunately buffer has another one that's not real
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	
	
	TEST_METHOD(DontFilterCRAtEndNoFit) {
		static const Chunk chunks[] = {
			{ "abc\n", "abc" },		// consumes '\n' because must presume it's not 'real'
			{ "def", "\nde" },		// must reinsert '\n'; that shifts the trailing character out
			{ nullptr, "f" }
			};
		
		Run(chunks, sizeof chunks / sizeof *chunks);
		}
	};


/*	Run
	Pass a sequence of chunks through the filter
*/
void TestCalendarFilter::Run(
	const Chunk	chunks[],
	const unsigned	chunksN
	)
{
Filter filter;

// for each buffer in the sequence
for (const Chunk *chunk = chunks, *const chunkE = chunks + chunksN; chunk < chunkE; ++chunk)
	// test input-output
	if (chunk->input) {
		const unsigned
			inputL = strlen(chunk->input),
			outputL = strlen(chunk->output);
		char *const buffer = static_cast<char*>(alloca(inputL));
		(void) memcpy(buffer, chunk->input, inputL);
		const unsigned bufferL = filter(buffer, inputL);

		Assert::IsTrue(bufferL == outputL);
		Assert::IsTrue(memcmp(buffer, chunk->output, outputL) == 0);
		}

	// check contents of pushback buffer
	else {
		const unsigned outputL = strlen(chunk->output);

		const Filter::Buffer tail = filter.Pushback();

		Assert::IsTrue(tail.end - tail.begin == outputL);
		Assert::IsTrue(memcmp(tail.begin, chunk->output, outputL) == 0);
		}
}
