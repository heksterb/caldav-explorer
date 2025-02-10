/*
	Dynamic
	
	Parsing arbitrary iCalendar items
	
	2019/05/19	Originated
	
	Copyright © 2019-2023 by: Ben Hekster

	iCalendar handling; independent of transport or storage
*/

#pragma once

#include <map>
#include <optional>
#include <set>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Calendar.h"



/*	DynamicCalendar
	iCalendar Object that can hold every known and unknown property
	*** almost: most components allow "iana-prop" and "x-prop", but we only collect those at the top calendar stream level
	
	Note that parsing and writing a calendar item will, in general, not produce the same text; for at least
	the following reasons:
	1)	the standards mostly don't dictate order of properties within components
	2)	we don't emit values that are also the default for that property
*/
template <typename Char = wchar_t>
struct DynamicCalendar : public Calendar<Char> {
	using string = std::basic_string<Char>;	
	using string_view = std::basic_string_view<Char>;
	using istream = std::basic_istream<Char>;
	using ostream = std::basic_ostream<Char>;
	
	// really not sure why all these are needed
	/* Generic programming does appear to make code less 'clean' looking */
	using Action = typename Calendar<Char>::Action;
	using Classification = typename Calendar<Char>::Classification;
	using Date = typename Calendar<Char>::Date;
	using DateTime = typename Calendar<Char>::DateTime;
	using Duration = typename Calendar<Char>::Duration;
	using Scale = typename Calendar<Char>::Scale;
	using StatusEvent = typename Calendar<Char>::StatusEvent;
	using StatusToDo = typename Calendar<Char>::StatusToDo;
	using Transparency = typename Calendar<Char>::Transparency;
	using UTCOffset = typename Calendar<Char>::UTCOffset;
	using ValueType = typename Calendar<Char>::ValueType;
	
	
	/*	RecurrenceDateTime
		Recurrence date-time (§3.8.5.2)
	*/
	using RecurrenceDateTime = std::variant<
		DateTime,
		Date
		// *** , Period
		>;


	/*	Alarm
		Alarm component §3.6.6
	*/
	struct Alarm {
		using Trigger = std::variant<std::monostate, Duration, DateTime>;
		
		Action		fAction {};
		string		fDescription;
		Trigger		fTrigger;
		std::map<string, std::pair<string, string>> fLines;
		std::map<string, std::map<string, string>> fParameters;
		
		// VALARM Extensions for iCalendar 4
		std::optional<DateTime> fAcknowledged;
		string		fUID;
		};


	/*	RecurrenceRule

	*/
	class RecurrenceRule : public Calendar<Char>::Parser::RecurrenceRule {
	public:
		using Unit = typename Calendar<Char>::Parser::RecurrenceRule::Unit;
		using Weekday = typename Calendar<Char>::Parser::RecurrenceRule::Weekday;
		
		
		Unit		fFrequency { Unit::kNone };
		unsigned	fInterval {};
		std::variant<std::monostate, Date, DateTime> fUntil;
		std::set<std::pair<signed char, Weekday>> fByDay;
		std::set<unsigned char> fMonths0;

	protected:
		virtual void	Frequency(Unit),
				Until(Date),
				Until(DateTime),
				Interval(unsigned),
				ByDay(Weekday, signed char ordinal),
				ByMonth0(unsigned char);
		};


	/*	Component
		Event/To-Do/Journal §3.6.3 Calendar Components
	*/
	struct Component {
		std::vector<Alarm> fAlarms;

		// descriptive properties §3.8.1
		// attachment
		// categories
		Classification	fClassification {};
		// comment
		string		fDescription;
		// geographic position
		string		fLocation;
		unsigned char	fPriority {};
		// resources
		string		fSummary;

		// date and time component properties §3.8.2
		std::variant<std::monostate, Date, DateTime> fStart;
		string		fStartTimeZoneID,
				fEndTimeZoneID;
		//		duration
		//		free/busy time

		// time zone component properties §3.8.3

		// relationship component properties §3.8.4
		//		attendee
		//		contact
		//		organizer
		//		recurrence ID
		//		related to
		string		fURL;
		string		fUID;

		// recurrence component properties §3.8.5
		//		exception date-times
		//		recurrence date-times
		//		recurrence rule
		std::optional<RecurrenceRule> fRecurrenceRule;

		// alarm component properties §3.8.6

		// change management component properties §3.8.7
		std::optional<DateTime>
				fCreated,
				fStamp,
				fLastModified;
		unsigned	fSequence {};

		// miscellaneous component properties §3.8.8
		std::map<string, string> fExtra;
		};


	/*	Event
		Event calendar component §3.6.1
	*/
	struct Event : public Component {
		// descriptive properties §3.8.1
		StatusEvent	fStatus {};

		// date and time component properties §3.8.2
		std::variant<std::monostate, Date, DateTime> fEnd;
		Transparency	fTransparency {};
		};


	/*	ToDo
		To-Do calendar component §3.6.2
	*/
	struct ToDo : public Component {
		// descriptive properties §3.8.1
		// percent complete
		StatusToDo	fStatus {};

		// date and time component properties §3.8.2
		//		completed
		std::variant<std::monostate, Date, DateTime> fDue;
		string		fDueTimeZoneID;
		};

	
	/*	TimeZone
		Time zone component (§3.6.5)
	*/
	struct TimeZone {
		struct Division {
			enum { kStandard, kDaylight } fKind;
			
			// 
			std::variant<
				std::monostate,
				Date,
				DateTime
				>
					fStart;
			std::optional<UTCOffset>
					fOffsetFrom,
					fOffsetTo;

			std::optional<RecurrenceRule> fRecurrenceRule;
			
			// *** comment
			std::vector<RecurrenceDateTime> fRecurrence;
			string		fName;

			// *** extra
			};
		

		// time zone component properties §3.8.3
		string		fID;
		
		// *** last-mod
		// *** tzurl

		// miscellaneous component properties §3.8.8
		std::map<string, string> fExtra;
		
		std::vector<Division> fDivisions;
		};
	
	
	/*	Parser
	
	*/
	struct Parser : public Calendar<Char>::Parser {
	protected:
		using typename Calendar<Char>::Parser::Context;
		
		
		/* *** Some (but only some) of these array sizes need to be specified explicitly, or
		   the std::begin() template selection doesn't work. */
		const static typename Context::Line::Parameter
				gContextLineParametersDateTimeEnd[],
				gContextLineParametersDateTimeStart[],
				gContextLineParametersDue[],
				gContextLineParametersTrigger[];
		
		const static typename Context::Line
				gContextLinesAlarm[],
				gContextLinesCalendar[3],
				gContextLinesEvent[],
				gContextLinesToDo[],
				gContextLinesTimeZone[],
				gContextLinesTimeZoneDivision[];
		
		const static Context
				gContext,
				gContextInner[],
				gContextCalendarInner[3],
				gContextCalendarTimeZoneInner[2],
				gContextCalendarEventInner[1],
				gContextCalendarToDoInner[1];
		
		static RecurrenceDateTime ParseRecurrenceDateTime(string_view);
		
		
		// calendar we're parsing into
		DynamicCalendar	&fInto;
		
		// component being parsed
		std::variant<std::monostate, Event, ToDo, TimeZone> fOuter;
		Component	*fComponent;
		std::optional<Alarm> fAlarm;
		std::optional<typename TimeZone::Division> fTimeZoneDivision;
		
		// parameters
		string		fTimeZoneIdentifier;
		ValueType	fValue;
		
		
		// component parsers
		void		BeginEvent();
		bool		EndEvent();
		void		BeginToDo();
		bool		EndToDo();
		void		BeginTimeZone();
		bool		EndTimeZone();
		void		BeginTimeZoneStandard(), BeginTimeZoneDaylight();
		bool		EndTimeZoneDivision();
		virtual void	ParseAlarm();
		
		void		Other(string_view, string_view, string_view),
				OtherAlarmLine(string_view, string_view, string_view),
				OtherComponent(string_view, string_view, string_view),
				OtherTimeZone(string_view, string_view, string_view);
		
		void		OtherAlarmParameter(string_view, string_view, string_view);
		
		// property parameter parsers
		void		TimeZoneIdentifier(string_view),
				Value(string_view);
		
		// property parsers
		void		Acknowledged(string_view),
				Action(string_view),
				Classification(string_view),
				Created(string_view),
				DateTimeStart(string_view),
				DateTimeEnd(string_view),
				DateTimeStamp(string_view),
				Description(string_view),
				DescriptionAlarm(string_view),
				Due(string_view),
				LastModified(string_view),
				Location(string_view),
				Priority(string_view),
				ProductID(string_view),
				RecurrenceRule(string_view),
				Scale(string_view),
				Sequence(string_view),
				StatusEvent(string_view),
				StatusToDo(string_view),
				Summary(string_view),
				TimeZoneID(string_view),
				TimeZoneName(string_view),
				TimeZoneOffsetFrom(string_view),
				TimeZoneOffsetTo(string_view),
				TimeZoneDivisionDateTimeStart(string_view),
				TimeZoneDivisionRecurrenceRule(string_view),
				TimeZoneDivisionRecurrenceDate(string_view),
				Transparency(string_view),
				Trigger(string_view),
				UID(string_view),
				UIDAlarm(string_view),
				URL(string_view),
				Version(string_view),
				XAppleStructuredLocation(string_view);
	
	public:
		explicit	Parser(DynamicCalendar&, istream&);
				Parser(const DynamicCalendar&) = delete;
		};
	
	
	/*	ComponentVariant
		Variant type of different component types
	*/
	using ComponentVariant = 
		std::variant<
			std::monostate,
			Event,
			ToDo, /*
			Journal,
			TimeZoneInformation,
			FreeBusyInformation */
			TimeZone
			>;
	
	
	string		fProductID;
	std::vector<ComponentVariant> fComponents;
	
	// calendar properties §3.7
	Scale		fScale {};
	// version is implied "2.0"
	std::map<string, string> fExtra;
	
	template <typename C>
	friend std::basic_ostream<C> &operator<<(std::basic_ostream<C>&, const DynamicCalendar<C>&);
	};