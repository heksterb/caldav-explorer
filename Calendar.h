/*
	Calendar
	
	iCalendar items
	
	2019/05/19	Originated
	
	Copyright © 2019 by: Ben Hekster

	iCalendar handling; independent of transport or storage
*/

#pragma once

#include <functional>
#include <istream>
#include <string_view>


/*	Calendar
	Represents the ability to parse a stream into an iCalendar object

NOTE
	Derived from RFC 5455, which is itself
	based on/derived from RFC 2425 ("A MIME Content-Type for Directory Information")
*/
template <typename Char>
struct Calendar {
	using string = std::basic_string<Char>;
	using string_view = std::basic_string_view<Char>;
	using istream = std::basic_istream<Char>;
	
	
	enum class Action : unsigned char { kNone, kAudio, kDisplay, kEMail, kOther };
	enum class Classification : unsigned char { kNone, kPublic, kPrivate, kConfidential, kOther };
	enum class Scale : unsigned char { kNone, kGregorian, kOther };
	enum class StatusEvent : unsigned char { kNone, kTentative, kConfirmed, kCancelled };
	enum class StatusJournal : unsigned char { kNone, kDraft, kFinal, kCancelled };
	enum class StatusToDo : unsigned char { kNone, kNeedsAction, kCompleted, kInProcess, kCancelled };
	enum class Transparency : unsigned char { kNone, kTransparent, kOpaque };
	enum class ValueType : unsigned char { kNone, kBinary, kBoolean, kCalendarAddress, kDate, kDateTime, kDuration, kFloat, kInteger, kPeriod, kRecurrence, kText, kURI, kUTCOffset, kOther };
	
	
	/*	Date
		Date (§3.3.4)
	*/
	struct Date {
		unsigned short	fYear;
		unsigned char	fMonth0,
				fDay0;
		};
	
	
	/*	Duration
		Duration (§3.3.6)
	*/
	struct Duration {
		enum class Style : unsigned char {
			kNone, kWeek, kDate, kDateTime, kTime
			};
		
		enum class Unit : unsigned char {
			kHour, kMinute, kSecond, kNone
			};
		
		using Value = unsigned short;
		
		bool		fNegative;
		Style		fStyle;
		union {
			// weeks
			Value		fWeek;
			
			// date/date-time/time
			struct {
				Value		fDay;
				Unit		fFrom, fTo;
				Value		fHours, fMinutes, fSeconds;
				};
			};
		};
	
	
	/*	Time
		Time (§3.3.12)
	*/
	struct Time {
		enum class Zone : unsigned char {
			kNone,
			kUTC
			};
		
		unsigned char	fHour,
				fMinute,
				fSecond;
		Zone		fZone;
		};
	
	
	/*	DateTime
		Date-Time (§3.3.5)
	*/
	struct DateTime {
		static DateTime MakeForNowUTC();
		
		Date		fDate;
		Time		fTime;
		};
	
	
	/*	UTCOffset
		UTC Offset (§3.3.14)
	*/
	struct UTCOffset {
		signed char	fHour;
		unsigned char	fMinute,
				fSecond;
		
		operator	bool() const { return fHour || fMinute || fSecond; }
		};
	
	
	/*	Parser
		iCalendar stream parser
	*/
	struct Parser {
		/*	RecurrenceRule
			Recurrence rule (§3.3.10)
		*/
		class RecurrenceRule {
		public:
			enum class Unit : unsigned char { kNone, kSecondly, kMinutely, kHourly, kDaily, kWeekly, kMonthly, kYearly };
			enum class Weekday : unsigned char { kNone, kSunday, kMonday, kTuesday, kWednesday, kThursday, kFriday, kSaturday };
		
		private:
			void		ParseFrequency(string_view),
					ParseUntil(string_view),
					ParseCount(string_view),
					ParseInterval(string_view),
					ParseBySecond(string_view),
					ParseByMinute(string_view),
					ParseByHour(string_view),
					ParseByDay(string_view),
					ParseByMonthDay(string_view),
					ParseByYearDay(string_view),
					ParseByWeekNumber(string_view),
					ParseByMonth(string_view),
					ParseBySetPos(string_view),
					ParseWkst(string_view);
		
		protected:
			virtual void	Frequency(Unit) = 0,
					Until(Date) = 0,
					Until(DateTime) = 0,
					Interval(unsigned) = 0,
					ByDay(Weekday, signed char ordinal) = 0,
					ByMonth0(unsigned char) = 0;
		
		public:
			void		Parse(string_view);
			};
	
	protected:
		typedef void (Parser::*Extra)(string_view name, string_view parameters, string_view value);
		
		
		/*	Context
			Define parseable VBEGIN/VEND context
		*/
		struct Context {
			/*	Line
				Parseable property
			*/
			struct Line {
				/*	Parameter
					Parseable property parameter
				*/
				struct Parameter {
					const Char	*name;
					
					void		(Parser::*Callback)(string_view value);
					
					operator	const string_view() const { return name; }
					};
				
				
				const Char	*key;
				void		(Parser::*Callback)(string_view value);
				const Parameter	*fParameters, *fParametersE;
				
				operator	const string_view() const { return key; }
				};
			
			// BEGIN name
			const Char	*fName;
			
			// link to containing context; array of contained contexts
			const Context	*fOuter;
			const Context	*fInner;
			const Context	*fInnerE;
			
			// recognized content lines
			const Line	*fLines;
			const Line	*fLinesE;
			
			// BEGIN and END events
			void		(Parser::*FBegin)();
			bool		(Parser::*FEnd)();
			
			// unrecognized content lines
			void		(Parser::*FExtraLine)(string_view name, string_view parameters, string_view value);
			void		(Parser::*FExtraParameter)(string_view name, string_view parameterName, string_view parameterValue);
			
			operator	const string_view() const { return fName; }
			};
	
	public:
		static Action	ParseAction(string_view);
		static Classification ParseClassification(string_view);
		static Date	ParseDate(string_view);
		static DateTime	ParseDateTime(string_view);
		static Duration	ParseDuration(string_view);
		static Scale	ParseScale(string_view);
		static StatusEvent ParseStatusEvent(string_view);
		static StatusToDo ParseStatusToDo(string_view);
		static Transparency ParseTransparency(string_view);
		static UTCOffset ParseUTCOffset(string_view);
		static ValueType ParseValueType(string_view);
		
		
		void		BeginAlarm();
		bool		EndAlarm();
	
	protected:
		istream		&fInput;
		const Context	*fContext;
		
		// work around iCloud bug: see operator()
		void		(Parser::*FHackNextLine)(string_view value) = {};
		
		
		virtual void	ParseAlarm() = 0;
		
		bool		End(string_view, string_view);
		void		Begin(string_view, string_view),
				Property(const Context&, const typename Context::Line&, string_view, string_view);
	
	public:
		explicit	Parser(const Context&, istream&);
		
		void		operator()();
		};
	};
