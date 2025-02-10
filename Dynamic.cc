/*
	Calendar
	
	iCalendar items
	
	2019/05/19	Originated
	
	Copyright © 2019-2023 by: Ben Hekster
*/

#include <assert.h>

#include <charconv>
#include <iomanip>
#include <string_view>

#include "Dynamic.h"



/*

	DynamicCalendar

*/

// list of property parameter parsers, sorted by key; [RFC 5545]
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line::Parameter
	// [ß3.8.2.2]
	DynamicCalendar<Char>::Parser::gContextLineParametersDateTimeEnd[] = {
		{ L"TZID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneIdentifier) },
		{ L"VALUE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Value) }
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context::Line::Parameter
	DynamicCalendar<Char>::Parser::gContextLineParametersDateTimeStart[] = {
		{ L"TZID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneIdentifier) },
		{ L"VALUE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Value) }
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context::Line::Parameter
	DynamicCalendar<Char>::Parser::gContextLineParametersDue[] = {
		{ L"TZID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneIdentifier) },
		{ L"VALUE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Value) }
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context::Line::Parameter
	DynamicCalendar<Char>::Parser::gContextLineParametersTrigger[] = {
		{ L"VALUE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Value) }
		};


// list of property parsers, sorted by key
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesAlarm[] = {
		{ L"ACKNOWLEDGED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Acknowledged) },
		{ L"ACTION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Action) },
		{ L"DESCRIPTION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DescriptionAlarm) },
		{ L"TRIGGER", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Trigger),
			std::begin(gContextLineParametersTrigger), std::end(gContextLineParametersTrigger) },
		{ L"UID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::UIDAlarm) },
		};
	
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesCalendar[] = {
		{ L"CALSCALE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Scale) },
		{ L"PRODID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::ProductID) },
		{ L"VERSION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Version) }
		};
	
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesToDo[] = {
		// COMPLETED
		{ L"CREATED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Created) },
		{ L"DESCRIPTION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Description) },
		{ L"DTSTAMP", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DateTimeStamp) },
		{ L"DTSTART", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DateTimeStart),
			std::begin(gContextLineParametersDateTimeStart), std::end(gContextLineParametersDateTimeStart) },
		{ L"DUE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Due),
			std::begin(gContextLineParametersDue), std::end(gContextLineParametersDue) },
		{ L"LAST-MODIFIED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::LastModified) },
		{ L"LOCATION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Location) },
		{ L"PRIORITY", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Priority) },
		{ L"SEQUENCE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Sequence) },
		{ L"STATUS", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::StatusToDo) },
		{ L"SUMMARY", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Summary) },
		{ L"UID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::UID) },
		};
	
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesEvent[] = {
		{ L"CLASS", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Classification) },
		{ L"CREATED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Created) },
		{ L"DESCRIPTION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Description) },
		{ L"DTEND", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DateTimeEnd),
			std::begin(gContextLineParametersDateTimeEnd), std::end(gContextLineParametersDateTimeEnd) },
		{ L"DTSTAMP", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DateTimeStamp) },
		{ L"DTSTART", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::DateTimeStart),
			std::begin(gContextLineParametersDateTimeStart), std::end(gContextLineParametersDateTimeStart) },
		{ L"LAST-MODIFIED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::LastModified) },
		{ L"LOCATION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Location) },
		{ L"RRULE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::RecurrenceRule) },
		{ L"SEQUENCE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Sequence) },
		{ L"STATUS", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::StatusEvent) },
		{ L"SUMMARY", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Summary) },
		{ L"TRANSP", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::Transparency) },
		{ L"UID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::UID) },
		{ L"URL", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::URL) },
		{ L"X-APPLE-STRUCTURED-LOCATION", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::XAppleStructuredLocation) }
		};
	
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesTimeZone[] = {
		{ L"LAST-MODIFIED", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::LastModified) },
		{ L"TZID", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneID) },
		};
	
template <typename Char>
const typename Calendar<Char>::Parser::Context::Line
	DynamicCalendar<Char>::Parser::gContextLinesTimeZoneDivision[] = {
		{ L"DTSTART", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneDivisionDateTimeStart) },
		{ L"RDATE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneDivisionRecurrenceDate) },
		{ L"RRULE", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneDivisionRecurrenceRule) },
		{ L"TZNAME", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneName) },
		{ L"TZOFFSETFROM", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneOffsetFrom) },
		{ L"TZOFFSETTO", static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&DynamicCalendar::Parser::TimeZoneOffsetTo) },
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContextCalendarEventInner[] = {
		{
			L"VALARM",
			&gContextCalendarInner[0],
			nullptr, nullptr,
			std::begin(gContextLinesAlarm), std::end(gContextLinesAlarm),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginAlarm),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndAlarm),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherAlarmLine),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherAlarmParameter)
			}
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContextCalendarToDoInner[] = {
		{
			L"VALARM",
			&gContextCalendarInner[2],
			nullptr, nullptr,
			std::begin(gContextLinesAlarm), std::end(gContextLinesAlarm),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginAlarm),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndAlarm),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherAlarmLine),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherAlarmParameter)
			}
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContextCalendarTimeZoneInner[] = {
		{
			L"DAYLIGHT",
			&gContextCalendarInner[1],
			nullptr, nullptr,
			std::begin(gContextLinesTimeZoneDivision), std::end(gContextLinesTimeZoneDivision),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginTimeZoneDaylight),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndTimeZoneDivision)
			},

		{
			L"STANDARD",
			&gContextCalendarInner[1],
			nullptr, nullptr,
			std::begin(gContextLinesTimeZoneDivision), std::end(gContextLinesTimeZoneDivision),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginTimeZoneStandard),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndTimeZoneDivision)
			}
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContextCalendarInner[3] = {
		{
			L"VEVENT",
			&gContextInner[0],
			std::begin(gContextCalendarEventInner), std::end(gContextCalendarEventInner),
			std::begin(gContextLinesEvent), std::end(gContextLinesEvent),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginEvent),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndEvent),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherComponent)
			},

		{
			L"VTIMEZONE",
			&gContextInner[0],
			std::begin(gContextCalendarTimeZoneInner), std::end(gContextCalendarTimeZoneInner),
			std::begin(gContextLinesTimeZone), std::end(gContextLinesTimeZone),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginTimeZone),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndTimeZone),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherTimeZone)
			},

		{
			L"VTODO",
			&gContextInner[0],
			std::begin(gContextCalendarToDoInner), std::end(gContextCalendarToDoInner),
			std::begin(gContextLinesToDo), std::end(gContextLinesToDo),
			static_cast<void (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::BeginToDo),
			static_cast<bool (Calendar<Char>::Parser::*)()>(&DynamicCalendar::Parser::EndToDo),
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::OtherComponent)
			}
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContextInner[] = {
		{
			L"VCALENDAR",
			&gContext,
			std::begin(gContextCalendarInner), std::end(gContextCalendarInner),
			std::begin(gContextLinesCalendar), std::end(gContextLinesCalendar),
			nullptr, nullptr,
			static_cast<void (Calendar<Char>::Parser::*)(string_view, string_view, string_view)>(&DynamicCalendar::Parser::Other)
			}
		};

template <typename Char>
const typename Calendar<Char>::Parser::Context
	DynamicCalendar<Char>::Parser::gContext = {
		L"",
		nullptr,
		gContextInner, gContextInner + 1,
		nullptr, nullptr
		};


/*	ParseRecurrenceDateTime

*/
template <typename Char>
typename DynamicCalendar<Char>::RecurrenceDateTime DynamicCalendar<Char>::Parser::ParseRecurrenceDateTime(
	string_view	recurrence
	)
{
RecurrenceDateTime result;

// DATE-TIME?
if (recurrence.length() == 15 || recurrence.length() == 16)
	result = Calendar<Char>::Parser::ParseDateTime(recurrence);

// DATE?
else if (recurrence.length() == 8)
	result = Calendar<Char>::Parser::ParseDate(recurrence);

else
	throw "not implemented: FORM #3";

return result;
}


/*	Parser
	Prepare to parse any calendar item and retain everything
*/
template <typename Char>
DynamicCalendar<Char>::Parser::Parser(
	DynamicCalendar<Char> &into,
	istream		&input
	) :
	Calendar<Char>::Parser::Parser(gContext, input),
	fInto(into),
	fComponent(nullptr),
	fValue(ValueType::kNone)
{
}


/*	BeginTimeZoneStandard
	Start parsing a STANDARD division of a TIMEZONE
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::BeginTimeZoneStandard()
{
// should not currently be parsing any time zone divisions
assert(!fTimeZoneDivision);

fTimeZoneDivision.emplace();
fTimeZoneDivision->fKind = TimeZone::Division::kStandard;
}


/*	BeginTimeZoneDaylight
	Start parsing a DAYLIGHT division of a TIMEZONE
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::BeginTimeZoneDaylight()
{
// should not currently be parsing any time zone divisions
assert(!fTimeZoneDivision);

fTimeZoneDivision.emplace();
fTimeZoneDivision->fKind = TimeZone::Division::kDaylight;
}


/*	EndTimeZoneDivision
	Complete parsing a TIMEZONE division
*/
template <typename Char>
bool DynamicCalendar<Char>::Parser::EndTimeZoneDivision()
{
// should currently be parsing a standard division
assert(fTimeZoneDivision);

// add it to our calendar item's list
std::get<TimeZone>(fOuter).fDivisions.emplace_back(std::move(*fTimeZoneDivision));
fTimeZoneDivision.reset();

return false;
}


/*	ParseAlarm

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::ParseAlarm()
{
// should be parsing a component
assert(fComponent);

// should not currently be parsing an alarm
if (fAlarm) throw "duplicate alarm";

fAlarm.emplace();

Calendar<Char>::Parser::operator()();

// should currently be parsing an alarm
assert(fAlarm);

// add it to our calendar component's alarm list
fComponent->fAlarms.emplace_back(std::move(*fAlarm));
fAlarm.reset();
}


/*	BeginEvent
	Start parsing a VEVENT
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::BeginEvent()
{
// should not currently be parsing any item
if (!std::holds_alternative<std::monostate>(fOuter)) throw "unexpected start of event";

fOuter = Event();
fComponent = std::get_if<Event>(&fOuter);
}


/*	EndEvent
	Complete parsing a VEVENT
*/
template <typename Char>
bool DynamicCalendar<Char>::Parser::EndEvent()
{
// should currently be parsing an event
fInto.fComponents.emplace_back(std::move(std::get<Event>(fOuter)));

fComponent = nullptr;
fOuter = std::monostate();

return false;
}


/*	BeginToDo
	Start parsing a VTODO
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::BeginToDo()
{
// should not currently be parsing any item
if (!std::holds_alternative<std::monostate>(fOuter)) throw "unexpected to-do";

fOuter = ToDo();
fComponent = std::get_if<ToDo>(&fOuter);
}


/*	EndToDo
	Complete parsing a VTODO
*/
template <typename Char>
bool DynamicCalendar<Char>::Parser::EndToDo()
{
// should currently be parsing a to-do
if (!std::holds_alternative<ToDo>(fOuter)) throw "unexpected end of to-do";

fInto.fComponents.emplace_back(std::move(std::get<ToDo>(fOuter)));
fComponent = nullptr;
fOuter = std::monostate();

return false;
}


/*	BeginTimeZone
	Start parsing a VTIMEZONE
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::BeginTimeZone()
{
// should not currently be parsing any item
if (!std::holds_alternative<std::monostate>(fOuter)) throw "unexpected timezone component";

fOuter = TimeZone();
}


/*	EndTimeZone
	Complete parsing a VTIMEZONE
*/
template <typename Char>
bool DynamicCalendar<Char>::Parser::EndTimeZone()
{
// should currently be parsing a time zone component
if (!std::holds_alternative<TimeZone>(fOuter)) throw "unexpected end of timezone component";

fInto.fComponents.emplace_back(std::move(std::get<TimeZone>(fOuter)));
fOuter = std::monostate();

return false;
}



/*

	property parameters parsers

*/

/*	TimeZoneIdentifier
	Parse Time Zone Identifier parameter value type
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneIdentifier(
	string_view	tzid
	)
{
fTimeZoneIdentifier = tzid;
}


/*	Value
	Parse parameter value type
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Value(
	string_view	value
	)
{
fValue = Calendar<Char>::Parser::ParseValueType(value);
}


/*

	property parsers

*/

/*	Acknowledged
	Alarm acknowledged property
	[VALARM Extensions for iCalendar]
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Acknowledged(
	string_view	acknowledged
	)
{
// should currently be parsing an alarm
assert(fAlarm);

if (fAlarm->fAcknowledged) throw "duplicate acknowledged";
fAlarm->fAcknowledged = Calendar<Char>::Parser::ParseDateTime(acknowledged);
}


/*	Action

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Action(
	string_view	action
	)
{
assert(fAlarm);

if (fAlarm->fAction != Action::kNone) throw "duplicate action";
fAlarm->fAction = Calendar<Char>::Parser::ParseAction(action);
}


template <typename Char>
void DynamicCalendar<Char>::Parser::Classification(
	string_view	classification
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);

event.fClassification = Calendar<Char>::Parser::ParseClassification(classification);
}


/*	Created
	When the calendar component was created
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Created(
	string_view	created
	)
{
if (fComponent->fCreated) throw "multiple created";

fComponent->fCreated = Calendar<Char>::Parser::ParseDateTime(created);
}


template <typename Char>
void DynamicCalendar<Char>::Parser::DateTimeStart(
	string_view	start
	)
{
if (!std::holds_alternative<std::monostate>(fComponent->fStart)) throw "multiple date-time start";

// time zone parameter
std::exchange(fComponent->fStartTimeZoneID, fTimeZoneIdentifier);

// what property value type? ('consumes' the VALUE in the process)
switch (std::exchange(fValue, {})) {
	case ValueType::kNone: [[ fallthrough ]];
	case ValueType::kDateTime:
		fComponent->fStart = Calendar<Char>::Parser::ParseDateTime(start);
		break;

	case ValueType::kDate:
		fComponent->fStart = Calendar<Char>::Parser::ParseDate(start);
		break;
	
	default:
		throw "unknown date-time start value type";
	}
}


/*	DateTimeEnd
	Parse Date-Time End
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::DateTimeEnd(
	string_view	end
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);
if (!std::holds_alternative<std::monostate>(event.fEnd)) throw "multiple date-time end";

// time zone parameter
std::exchange(fComponent->fEndTimeZoneID, fTimeZoneIdentifier);

// what property value type? ('consumes' the VALUE in the process)
switch (std::exchange(fValue, {})) {
	case ValueType::kNone: [[ fallthrough ]];
	case ValueType::kDateTime:
		event.fEnd = Calendar<Char>::Parser::ParseDateTime(end);
		break;

	case ValueType::kDate:
		event.fEnd = Calendar<Char>::Parser::ParseDate(end);
		break;
	
	default:
		throw "unknown date-time end value type";
	}
}


/*	DateTimeStamp
	When the calendar component was last modified
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::DateTimeStamp(
	string_view	stamp
	)
{
// should currently by parsing a component
if (fComponent->fStamp) throw "multiple date-time stamp";

fComponent->fStamp = Calendar<Char>::Parser::ParseDateTime(stamp);
}


/*	Description
	Description in top-level component (ß3.8.1.5)
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Description(
	string_view	description
	)
{
if (!fComponent->fDescription.empty()) throw "multiple description";

fComponent->fDescription = description;
}


/*	DescriptionAlarm
	Description in alarm component (ß3.8.1.5)
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::DescriptionAlarm(
	string_view	description
	)
{
if (!fAlarm->fDescription.empty()) throw "multiple description";

fAlarm->fDescription = description;
}


/*	Due
	Parse Date-Time Due (ß3.8.2.3)
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Due(
	string_view	due
	)
{
// should currently be parsing an to-do
ToDo &todo = std::get<ToDo>(fOuter);
if (!std::holds_alternative<std::monostate>(todo.fDue)) throw "multiple due";

// time zone parameter
std::exchange(todo.fDueTimeZoneID, fTimeZoneIdentifier);

// what property value type?
switch (std::exchange(fValue, {})) {
	case ValueType::kNone:
	case ValueType::kDateTime:
		todo.fDue = Calendar<Char>::Parser::ParseDateTime(due);
		break;

	case ValueType::kDate:
		todo.fDue = Calendar<Char>::Parser::ParseDate(due);
		break;
	
	default:
		throw "unknown date-time end value type";
	}
}


/*	RecurrenceRule
	Parse recurrence rule in a component
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::RecurrenceRule(
	string_view	recurrenceRule
	)
{
if (fComponent->fRecurrenceRule) throw "multiple recurrence rule";

// construct 
fComponent->fRecurrenceRule.emplace().Parse(recurrenceRule);
}


/*	LastModified
	When the calendar component was last modified
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::LastModified(
	string_view	lastModified
	)
{
// should currently be parsing a component
if (!fComponent) throw "unexpected last-modified";
if (fComponent->fLastModified) throw "multiple last-modified";

fComponent->fLastModified = Calendar<Char>::Parser::ParseDateTime(lastModified);
}


/*	Location

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Location(
	string_view	location
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);

if (!event.fLocation.empty()) throw "multiple location";
event.fLocation = location;
}


/*	Priority

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Priority(
	string_view	priority
	)
{
// should currently be parsing a to-do
ToDo &todo = std::get<ToDo>(fOuter);

if (todo.fPriority != 0) throw "multiple priority";
todo.fPriority = static_cast<unsigned char>(stoul(string(string_view(priority.data(), priority.size()))));
}


/*	ProductID

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::ProductID(
	string_view	productID
	)
{
if (!fInto.fProductID.empty()) throw "multiple Product ID";

fInto.fProductID = productID;
}


/*	Scale

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Scale(
	string_view	scale
	)
{
if (fInto.fScale != Scale::kNone) throw "multiple scale";

fInto.fScale = Calendar<Char>::Parser::ParseScale(scale);
}


/*	Sequence

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Sequence(
	string_view	sequence
	)
{
// should currently be parsing a component
if (!fComponent) throw "unexpected sequence";

if (fComponent->fSequence != 0) throw "multiple sequence";
fComponent->fSequence = stoul(string(string_view(sequence.data(), sequence.size())));
}


/*	StatusToDo

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::StatusToDo(
	string_view	status
	)
{
// should currently be parsing a to-do
ToDo &todo = std::get<ToDo>(fOuter);

if (todo.fStatus != StatusToDo::kNone) throw "multiple to-do status";
todo.fStatus = Calendar<Char>::Parser::ParseStatusToDo(status);
}


/*	StatusEvent

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::StatusEvent(
	string_view	status
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);

if (event.fStatus != StatusEvent::kNone) throw "multiple event status";
event.fStatus = Calendar<Char>::Parser::ParseStatusEvent(status);
}


/*	Summary

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Summary(
	string_view	summary
	)
{
// should currently be parsing a component
if (!fComponent) throw "unexpected summary";
if (!fComponent->fSummary.empty()) throw "multiple summary";

fComponent->fSummary = summary;
}


/*	TimeZoneID
	Åß3.6.5
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneID(
	string_view	timeZoneID
	)
{
TimeZone &timeZone = std::get<TimeZone>(fOuter);
if (!timeZone.fID.empty()) throw "multiple time zone ID";

timeZone.fID = timeZoneID;
}


/*	TimeZoneName

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneName(
	string_view	timeZoneName
	)
{
if (!fTimeZoneDivision->fName.empty()) throw "multiple time zone name";

fTimeZoneDivision->fName = timeZoneName;
}


/*	TimeZoneOffsetFrom
	TimeZoneOffsetTo
	Parse Time Zone Offset From (ß3.8.3.3)
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneOffsetFrom(
	string_view	timeZoneOffset
	)
{
if (fTimeZoneDivision->fOffsetFrom) throw "multiple time zone offset from";

fTimeZoneDivision->fOffsetFrom = Calendar<Char>::Parser::ParseUTCOffset(timeZoneOffset);
}

template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneOffsetTo(
	string_view	timeZoneOffset
	)
{
if (fTimeZoneDivision->fOffsetTo) throw "multiple time zone offset to";

fTimeZoneDivision->fOffsetTo = Calendar<Char>::Parser::ParseUTCOffset(timeZoneOffset);
}


/*	TimeZoneDivisionDateTimeStart

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneDivisionDateTimeStart(
	string_view	start
	)
{
assert(fTimeZoneDivision);
if (!std::holds_alternative<std::monostate>(fTimeZoneDivision->fStart)) throw "duplicate date-time start";

// either date-time or date
if (start.length() == 8)
	throw "not supported yet: non-time DTSTART";

else
	// date-time
	fTimeZoneDivision->fStart = Calendar<Char>::Parser::ParseDateTime(start);
}


/*	TimeZoneDivisionRecurrenceDate
	Parse 'rdate' in a Time Zone component division
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneDivisionRecurrenceDate(
	string_view	recurrenceDate
	)
{
fTimeZoneDivision->fRecurrence.emplace_back(ParseRecurrenceDateTime(recurrenceDate));
}


/*	TimeZoneDivisionRecurrenceRule

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::TimeZoneDivisionRecurrenceRule(
	string_view	recurrenceRule
	)
{
if (fTimeZoneDivision->fRecurrenceRule) throw "multiple recurrence rule";

fTimeZoneDivision->fRecurrenceRule.emplace().Parse(recurrenceRule);
}


/*	Transparency

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Transparency(
	string_view	transparency
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);

if (event.fTransparency != Transparency::kNone) throw "multiple transparency";
event.fTransparency = Calendar<Char>::Parser::ParseTransparency(transparency);
}


/*	Trigger
	Parse Alarm component trigger (ß3.8.6.3)
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Trigger(
	string_view	trigger
	)
{
assert(fAlarm);
if (!std::holds_alternative<std::monostate>(fAlarm->fTrigger)) throw "multiple trigger";

// what property value type?
switch (std::exchange(fValue, {})) {
	case ValueType::kDateTime:
		fAlarm->fTrigger = Calendar<Char>::Parser::ParseDateTime(trigger);
		break;
	
	case ValueType::kNone:
	case ValueType::kDuration:
		fAlarm->fTrigger = Calendar<Char>::Parser::ParseDuration(trigger);
		break;

	default:
		throw "unknown trigger value type";
	}
}


/*	UID

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::UID(
	string_view	uid
	)
{
// should currently by parsing a component
assert(fComponent);
if (!fComponent->fUID.empty()) throw "multiple UID";

fComponent->fUID = uid;
}


/*	UIDAlarm

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::UIDAlarm(
	string_view	uid
	)
{
// should currently by parsing an alarm
assert(fAlarm);

if (!fAlarm->fUID.empty()) throw "multiple UID";

fAlarm->fUID = uid;
}


/*	URL

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::URL(
	string_view	url
	)
{
// should currently be parsing an event
Event &event = std::get<Event>(fOuter);

if (!event.fURL.empty()) throw "multiple URL";
event.fURL = url;
}


/*	Version
	Parse version lines
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Version(
	string_view	version
	)
{
if (version != L"2.0") throw "unexpected version";
}


/*	XAppleStructuredLocation

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::XAppleStructuredLocation(
	string_view	location
	)
{
/*
	Produced this property:
	
		PRODID:-//Apple Inc.//iOS 10.0.2//EN (at least)

X-APPLE-STRUCTURED-LOCATION;VALUE=URI;X-ADDRESS=000A B Abc Def Gh\\nAbcdefghi CA 12345-6789\\nUnited States of America;X-APPLE-ABUID=ab://Thuis%20van%20Sinterklaas";X-APPLE-RADIUS=0;X-APPLE-REFERENCEFRAME=1;X-TITLE=000A B Abc Def Gh
Abcdefghi CA 12345-6789
United States of America:geo:12.345678,-123.456789

	Note the embedded actual newlines.  Added the 'FHackNewLine' mechanism specifically for this
	which instructs the calendar parser to invoke us again in case the next line can't be interpreted
	according to the normal rules.
	
	See also:
	
		https://github.com/sabre-io/vobject/issues/402
		https://github.com/nextcloud/calendar/issues/3905
	
	It seems that this is fixed in components produced by:
	
		PRODID:-//Apple Inc.//iOS 10.3.3//EN
*/
this->FHackNextLine = static_cast<void (Calendar<Char>::Parser::*)(string_view)>(&Parser::XAppleStructuredLocation);

// try to record as new property
if (
	auto [ propertyP, inserted ] = fComponent->fExtra.try_emplace(L"X-APPLE-STRUCTURED-LOCATION", location);
	
	// did not insert a new property (because it already existed)?
	!inserted
	) {
	string &property = propertyP->second;
	
	// extend existing property value ***** this loses the distinction of the parameters
	/* ***	This 'fixes' the broken property by creating an escaped newline.  It would probably actually
		be better if we allowed actual newlines in properties and escaped/unescaped in the emitter/parser. */
	property.append(L"\\n");
	property.append(location);
	}
}


/*	Other

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::Other(
	string_view	key,
	string_view	parameters,
	string_view	value
	)
{
// ***** loses the parameters
fInto.fExtra.emplace(
	std::piecewise_construct,
	std::forward_as_tuple(key),
	std::forward_as_tuple(value)
	);
}


/*	OtherAlarmLine
	Parse unrecognized lines in Alarm component
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::OtherAlarmLine(
	string_view	key,
	string_view	parameters,
	string_view	value
	)
{
fAlarm->fLines.try_emplace(
	string(key),
	std::pair<string, string>(parameters, value)
	);
}


/*	OtherAlarmParameter
	Parse unrecognized parameter properties in Alarm component
*/
template <typename Char>
void DynamicCalendar<Char>::Parser::OtherAlarmParameter(
	string_view	key,
	string_view	parameterName,
	string_view	parameterValue
	)
{
// find or construct the map of extra property parameters for this key
std::map<string, string> &parameters =
	fAlarm->fParameters.try_emplace(
		string(key),
		std::map<string, string>()
		).first->second;

parameters.try_emplace(string(parameterName), string(parameterValue));
}


/*	OtherComponent

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::OtherComponent(
	string_view	key,
	string_view	parameters,
	string_view	value
	)
{
// ***** this loses the parameters
fComponent->fExtra.emplace(
	std::piecewise_construct,
	std::forward_as_tuple(key),
	std::forward_as_tuple(value)
	);
}


/*	OtherTimeZone

*/
template <typename Char>
void DynamicCalendar<Char>::Parser::OtherTimeZone(
	string_view	key,
	string_view	parameters,
	string_view	value
	)
{
std::get<TimeZone>(fOuter).fExtra.emplace(
	std::piecewise_construct,
	std::forward_as_tuple(key),
	std::forward_as_tuple(value)
	);
}


/*	RecurrenceRule::Frequency

*/
template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::Frequency(
	Unit		frequency
	)
{
if (fFrequency != Unit::kNone) throw "multiple recurrence rule frequency";

fFrequency = frequency;
}


template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::Until(
	Date		until
	)
{
if (!std::holds_alternative<std::monostate>(fUntil)) throw "multiple recurrence rule until";

fUntil = until;
}

template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::Until(
	DateTime	until
	)
{
if (!std::holds_alternative<std::monostate>(fUntil)) throw "multiple recurrence rule until";

fUntil = until;
}


template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::Interval(
	unsigned	interval
	)
{
if (fInterval) throw "multiple recurrence rule interval";

fInterval = interval;
}


template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::ByDay(
	Weekday		weekday,
	signed char	ordinal
	)
{
fByDay.emplace(std::pair(ordinal, weekday));
}


template <typename Char>
void DynamicCalendar<Char>::RecurrenceRule::ByMonth0(
	unsigned char	month0
	)
{
fMonths0.emplace(month0);
}



/*

	Emission

*/

template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	std::monostate
	) {
	return output;
	}


/*	DynamicCalendar::Action::<<
	Alarm action (ß3.8.6.1)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Action action
	) {
	using Action = typename DynamicCalendar<Char>::Action;

	switch (action) {
		case Action::kNone:		break;
		case Action::kAudio:		output << L"AUDIO"; break;
		case Action::kDisplay:		output << L"DISPLAY"; break;
		case Action::kEMail:		output << L"EMAIL"; break;
		case Action::kOther:		throw "other calendar scale";
		}

	return output;
	}


/*	DynamicCalendar::Scale::<<
	Calendar scale (ß3.7.1)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Scale scale
	) {
	using Scale = typename DynamicCalendar<Char>::Scale;

	switch (scale) {
		case Scale::kNone:		break;
		case Scale::kGregorian:		output << L"GREGORIAN"; break;
		case Scale::kOther:		throw "other calendar scale";
		}

	return output;
	}


/*	DynamicCalendar::Classification::<<
	Classification (ß3.8.1.3)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Classification classification
	) {
	using Classification = typename DynamicCalendar<Char>::Classification;

	switch (classification) {
		case Classification::kNone:		break;
		case Classification::kPublic:		output << L"PUBLIC"; break;
		case Classification::kPrivate:		output << L"PRIVATE"; break;
		case Classification::kConfidential:	output << L"CONFIDENTIAL"; break;
		case Classification::kOther:		throw "other classification";
		}

	return output;
	}


/*	DynamicCalendar::Date::<<

*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Date &date
	) {
	output << std::setfill(L'0') <<
		std::setw(2) << static_cast<unsigned>(date.fYear) <<
		std::setw(2) << 1 + static_cast<unsigned>(date.fMonth0) <<
		std::setw(2) << 1 + static_cast<unsigned>(date.fDay0);

	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Time::Zone zone
	) {
	using Zone = typename DynamicCalendar<Char>::Time::Zone;
	
	switch (zone) {
		case Zone::kNone:	break;
		case Zone::kUTC:	output << 'Z'; break;
		}
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Time &time
	) {
	output << std::setfill(L'0') <<
		std::setw(2) << static_cast<unsigned>(time.fHour) <<
		std::setw(2) << static_cast<unsigned>(time.fMinute) <<
		std::setw(2) << static_cast<unsigned>(time.fSecond) <<
		time.fZone;
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::DateTime &dtz
	) {
	output << dtz.fDate << L'T' << dtz.fTime;
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Duration &duration
	) {
	using Style = typename DynamicCalendar<Char>::Duration::Style;
	using Unit = typename DynamicCalendar<Char>::Duration::Unit;
	
	if (duration.fNegative) output << L'-';
	
	output << L'P';
	
	// week style?
	if (duration.fStyle == Style::kWeek)
		output << duration.fWeek;
	
	// any combination of date and/or time
	else {
		// date?
		if (duration.fStyle == Style::kDate || duration.fStyle == Style::kDateTime)
			output << duration.fDay << L'D';
		
		// time?
		if (duration.fStyle == Style::kDateTime || duration.fStyle == Style::kTime) {
			output << 'T';
			
			// hours?
			if (duration.fFrom <= Unit::kHour && duration.fTo > Unit::kHour)
				output << duration.fHours << L'H';
			
			// minutes?
			if (duration.fFrom <= Unit::kMinute && duration.fTo > Unit::kMinute)
				output << duration.fMinutes << L'M';
			
			// seconds?
			if (duration.fFrom <= Unit::kSecond && duration.fTo > Unit::kSecond)
				output << duration.fSeconds << L'S';
			}
		}
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::RecurrenceRule::Unit frequency
	) {
	static const Char *const frequencies[] = {
		L"SECONDLY",
		L"MINUTELY",
		L"HOURLY",
		L"DAILY",
		L"WEEKLY",
		L"MONTHLY",
		L"YEARLY"
		};
	
	// emit corresponding string
	output << frequencies[static_cast<unsigned>(frequency) - 1];
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::RecurrenceRule &rule
	) {
	// must be first rule part specified
	output << "FREQ=" << rule.fFrequency;
	
	if (rule.fInterval) output << ";INTERVAL=" << rule.fInterval;
	
	if (!std::holds_alternative<std::monostate>(rule.fUntil)) {
		output << ";UNTIL=";
		std::visit([&](const auto &until) { output << until; }, rule.fUntil);
		}
	
	if (!rule.fMonths0.empty()) {
		output << ";BYMONTH=";
		for (auto month0I = rule.fMonths0.cbegin(); month0I != rule.fMonths0.cend(); ++month0I) {
			// separator
			if (month0I != rule.fMonths0.cbegin()) output << ',';
			const unsigned char &month0 = *month0I;

			output << 1 + month0;
			}
		}
	
	if (!rule.fByDay.empty()) {
		output << ";BYDAY=";
		for (auto byDayI = rule.fByDay.cbegin(); byDayI != rule.fByDay.cend(); ++byDayI) {
			// separator
			if (byDayI != rule.fByDay.cbegin()) output << ',';
			const std::pair<signed char, typename DynamicCalendar<Char>::RecurrenceRule::Weekday> &byDay = *byDayI;
			
			// ordinal
			if (byDay.first) output << static_cast<signed>(byDay.first);
			const Char *weekdays[] = {
				L"SU",
				L"MO",
				L"TU",
				L"WE",
				L"TH",
				L"FR",
				L"SA"
				};
			output << weekdays[static_cast<unsigned char>(byDay.second) - 1];
			}
		}
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::UTCOffset &offset
	) {
	// output hour
	if (offset.fHour < 0)
		output << '-'<< std::setfill(L'0') << std::setw(2) << static_cast<unsigned>(-offset.fHour);
	
	else
		output << std::setfill(L'0') << std::setw(2) << static_cast<unsigned>(offset.fHour);
	
	// output minute
	output << std::setfill(L'0') << std::setw(2) << static_cast<unsigned>(offset.fMinute);
	
	// optionally output second
	if (offset.fSecond)
		output << std::setw(2) << static_cast<unsigned>(offset.fSecond);
	
	return output;
	}


/*	DynamicCalendar::StatusEvent::<<
	Emit event status (ß3.8.1.11)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::StatusEvent status
	) {
	using Status = typename DynamicCalendar<Char>::StatusEvent;
	
	switch (status) {
		case Status::kNone:		break;
		case Status::kTentative:	output << L"TENTATIVE"; break;
		case Status::kConfirmed:	output << L"CONFIRMED"; break;
		case Status::kCancelled:	output << L"CANCELLED"; break;
		}
	
	return output;
	}


/*	DynamicCalendar::StatusToDo::<<
	Emit To-Do status (ß3.8.1.11)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::StatusToDo status
	) {
	using Status = typename DynamicCalendar<Char>::StatusToDo;
	
	switch (status) {
		case Status::kNone:		break;
		case Status::kNeedsAction:	output << L"NEEDS-ACTION"; break;
		case Status::kCompleted:	output << L"COMPLETED"; break;
		case Status::kInProcess:	output << L"IN-PROCESS"; break;
		case Status::kCancelled:	output << L"CANCELLED"; break;
		}
	
	return output;
	}


/*	DynamicCalendar::Transparency::<<
	Emit time transparency (ß3.8.2.7)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Transparency transparency
	) {
	using Transparency = typename DynamicCalendar<Char>::Transparency;
	
	switch (transparency) {
		case Transparency::kNone:		break;
		case Transparency::kOpaque:		output << L"OPAQUE"; break;
		case Transparency::kTransparent:	output << L"TRANSPARENT"; break;
		}
	
	return output;
	}


template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Alarm &alarm
	) {
	output << "BEGIN:VALARM\n";
	
	/*	OutputTrigger
		Output any non-default VALUE property parameter
	*/
	const auto OutputTrigger = [&](
		const typename DynamicCalendar<Char>::Alarm::Trigger &trigger
		) {
		const Char *value;
		switch (trigger.index()) {
			case 1:		value = nullptr /* duration */; break;
			case 2:		value = L"DATE-TIME"; break;
			default:	abort();
			}
		
		if (value) output << ";VALUE=" << value;
		};
	
	// *** may be required depending on action type
	if (alarm.fAction != DynamicCalendar<Char>::Action::kNone) output << L"ACTION:" << alarm.fAction << '\n';
	if (!std::holds_alternative<std::monostate>(alarm.fTrigger)) {
		output << L"TRIGGER";
		OutputTrigger(alarm.fTrigger);
		output << ':';
		std::visit([&](const auto &value) { output << value; }, alarm.fTrigger);
		output << '\n';
		}
	if (!alarm.fDescription.empty()) output << L"DESCRIPTION:" << alarm.fDescription << '\n';
	
	if (alarm.fAcknowledged) output << L"ACKNOWLEDGED:" << *alarm.fAcknowledged << '\n';
	if (!alarm.fUID.empty()) output << L"UID:" << alarm.fUID << '\n';
	
	// extra lines
	for (const auto &line: alarm.fLines) {
		// property name
		output << line.first;
		
		// optional property parameters
		if (!line.second.first.empty()) output << ';' << line.second.first;
		
		// property value
		output << ':' << line.second.second << '\n';
		}
	
	output << L"END:VALARM\n";
	
	return output;
	}


/*	DynamicCalendar::Event::<<
	Emit Event component (ß3.6.1)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::Event &event
	)
{
output << "BEGIN:VEVENT\n";

// required properties
if (event.fStamp) output << "DTSTAMP:" << *event.fStamp << '\n'; // not always present in practice
output << "UID:" << event.fUID << '\n';

// conditionally required
if (!std::holds_alternative<std::monostate>(event.fStart)) {
	output << "DTSTART:";
	
	// non-default value type
	if (std::holds_alternative<typename Calendar<Char>::Date>(event.fStart)) output << "VALUE=DATE;";
	
	std::visit([&output](const auto &start) { output << start; }, event.fStart);
	
	output << '\n';
	}

// optional properties
if (event.fClassification != DynamicCalendar<Char>::Classification::kNone) output << "CLASS:" << event.fClassification << '\n';
if (event.fCreated) output << "CREATED:" << *event.fCreated << '\n';
if (!event.fDescription.empty()) output << "DESCRIPTION:" << event.fDescription << '\n';
// *** GEO
if (event.fLastModified) output << "LAST-MODIFIED:" << *event.fLastModified << '\n';
if (!event.fLocation.empty()) output << "LOCATION:" << event.fLocation << '\n';
// *** ORGANIZER
if (event.fPriority != 0) output << "PRIORITY:" << static_cast<unsigned>(event.fPriority) << '\n'; // not specifying is equivalent to zero
if (event.fSequence != 0) output << "SEQUENCE:" << event.fSequence << '\n'; // default is zero, so is okay not to emit it in that case
if (event.fStatus != DynamicCalendar<Char>::StatusEvent::kNone) output << "STATUS:" << event.fStatus << '\n';
if (!event.fSummary.empty()) output << "SUMMARY:" << event.fSummary << '\n';
switch (event.fTransparency) {
	case DynamicCalendar<Char>::Transparency::kNone:
	case DynamicCalendar<Char>::Transparency::kOpaque:	// default
		break;

	default:
		output << "TRANSP:" << event.fTransparency << '\n';
		break;
	}
// *** URL
// *** RECURID

// optional properties
if (event.fRecurrenceRule) output << "RRULE:" << *event.fRecurrenceRule << '\n';

// conditionally optional
if (!std::holds_alternative<std::monostate>(event.fEnd))
	std::visit([&output](const auto &start) { output << "DTEND:" << start << '\n'; }, event.fEnd);

// ***

// extra properties
for (const auto &extra: event.fExtra)
	output << extra.first << ':' << extra.second << '\n';

// alarms *** anything else?
for (const typename DynamicCalendar<Char>::Alarm &alarm: event.fAlarms)
	output << alarm;

output << "END:VEVENT\n";

return output;
}


/*	DynamicCalendar::ToDo::<<
	Emit To-Do component (ß3.6.2)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::ToDo &todo
	)
{
output << L"BEGIN:VTODO\n";

// required properties
if (todo.fStamp) output << L"DTSTAMP:" << *todo.fStamp << '\n'; // not always present in practice
output << L"UID:" << todo.fUID << '\n';

// optional properties
if (todo.fClassification != DynamicCalendar<Char>::Classification::kNone) output << L"CLASS:" << todo.fClassification << '\n';
// COMPLETED
if (todo.fCreated) output << L"CREATED:" << *todo.fCreated << '\n';
if (!todo.fDescription.empty()) output << L"DESCRIPTION:" << todo.fDescription << '\n';
if (!std::holds_alternative<std::monostate>(todo.fStart)) {
	output << L"DTSTART";
	if (!todo.fStartTimeZoneID.empty()) output << L";TZID=" << todo.fStartTimeZoneID;
	output << ':';
	std::visit([&output](const auto &start) { output << start; }, todo.fStart);
	output << '\n';
	}
// GEO
if (todo.fLastModified) output << L"LAST-MODIFIED:" << *todo.fLastModified << '\n';
if (!todo.fLocation.empty()) output << L"LOCATION:" << todo.fLocation << '\n';
// ORGANIZER
// PERCENT
if (todo.fPriority != 0) output << L"PRIORITY:" << static_cast<unsigned>(todo.fPriority) << '\n'; // not specifying is equivalent to zero
// RECURRENCE-ID
if (todo.fSequence != 0) output << L"SEQUENCE:" << todo.fSequence << '\n'; // default is zero, so is okay not to emit it in that case
if (todo.fStatus != DynamicCalendar<Char>::StatusToDo::kNone) output << L"STATUS:" << todo.fStatus << '\n';
if (!todo.fSummary.empty()) output << L"SUMMARY:" << todo.fSummary << '\n';

// optional properties
// RRULE

// optional but exclusive ß3.8.2.3
if (!std::holds_alternative<std::monostate>(todo.fDue)) {
	output << L"DUE";
	
	// value data type property parameter
	if (std::holds_alternative<DynamicCalendar<wchar_t>::Date>(todo.fDue))
		output << ";VALUE=DATE";
	
	// time zone identifier property parameter
	if (!todo.fDueTimeZoneID.empty()) output << L";TZID=" << todo.fDueTimeZoneID;
	
	output << ':';
	
	// value
	std::visit([&output](const auto &start) { output << start; }, todo.fDue);
	output << '\n';
	}
// DURATION

// optional multiple
// ATTACH
// ATTENDEE
// CATEGORIES
// COMMENT
// CONTACT
// EXDATE
// REQUEST-STATUS
// RELATED
// RESOURCES
// RDATE

// extra properties
for (const auto &extra: todo.fExtra)
	output << extra.first << ':' << extra.second << '\n';

// alarms *** anything else?
for (const typename DynamicCalendar<Char>::Alarm &alarm: todo.fAlarms)
	output << alarm;

output << L"END:VTODO\n";

return output;
}


/*	DynamicCalendar::TimeZone::Division::<<
	Time zone division (ß3.6.5)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::TimeZone::Division &division
	)
{
const Char *const kind =
	division.fKind == DynamicCalendar<Char>::TimeZone::Division::kStandard ?
		L"STANDARD" :
		L"DAYLIGHT";

output << L"BEGIN:" << kind << '\n';

if (!std::holds_alternative<std::monostate>(division.fStart))
	std::visit([&output](const auto &start) { output << L"DTSTART:" << start << '\n'; }, division.fStart);
if (division.fOffsetFrom && *division.fOffsetFrom) output << L"TZOFFSETFROM:" << *division.fOffsetFrom << '\n';
if (division.fOffsetTo && *division.fOffsetTo) output << L"TZOFFSETTO:" << *division.fOffsetTo << '\n';
if (division.fRecurrenceRule) output << L"RRULE:" << *division.fRecurrenceRule << '\n';
// *** comment
for (const typename DynamicCalendar<Char>::RecurrenceDateTime &recurrenceVariant: division.fRecurrence)
	std::visit([&output](const auto &recurrence) { output << L"RDATE:" << recurrence << '\n'; }, recurrenceVariant);
if (!division.fName.empty()) output << L"TZNAME:" << division.fName << '\n';

output << L"END:" << kind << '\n';

return output;
}


/*	DynamicCalendar::TimeZone::<<
	Emit time zone component (ß3.6.5)
*/
template <typename Char>
static std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const typename DynamicCalendar<Char>::TimeZone &timeZone
	)
{
output << L"BEGIN:VTIMEZONE\n";

if (const std::basic_string<Char> &id = timeZone.fID; !id.empty()) output << L"TZID:" << id << '\n';
// *****

// extra properties
for (const auto &extra: timeZone.fExtra)
	output << extra.first << ':' << extra.second << '\n';

// for each time zone division
for (const typename DynamicCalendar<Char>::TimeZone::Division &division: timeZone.fDivisions)
	output << division;

output << L"END:VTIMEZONE\n";

return output;
}


/*	DynamicCalendar::<<
	Emit iCalendar object (ß3.4)
*/
template <typename Char>
std::basic_ostream<Char> &operator<<(
	std::basic_ostream<Char> &output,
	const DynamicCalendar<Char> &calendar
	)
{
// object must include at least one calendar component
if (calendar.fComponents.size() == 0) return output;

output << L"BEGIN:VCALENDAR\n";

// required properties
if (!calendar.fProductID.empty()) output << L"PRODID:" << calendar.fProductID << '\n';
output << L"VERSION:2.0\n";

// optional properties
if (calendar.fScale != DynamicCalendar<Char>::Scale::kNone) output << L"CALSCALE:" << calendar.fScale << '\n';
// *** method

// extra properties
for (const auto &extra: calendar.fExtra)
	output << extra.first << ':' << extra.second << '\n';

// for each calendar component
typename DynamicCalendar<Char>::Event blah;
for (const auto &component: calendar.fComponents)
	std::visit(
		[&output](const auto &type) { output << type; },
		component
		);

output << L"END:VCALENDAR\n";

return output;
}


// explicit instantiation
/* Unfortunately we can't instantiate both at the same time; not until I find a solution to string literals
   that doesn't depend on the preprocessor ("CFSTR"). */
#if 0
template struct DynamicCalendar<char>;
template std::basic_ostream<char> &operator<<(std::basic_ostream<char>&, const DynamicCalendar<char>&);

#else
template struct DynamicCalendar<wchar_t>;
template std::basic_ostream<wchar_t> &operator<<(std::basic_ostream<wchar_t>&, const DynamicCalendar<wchar_t>&);
#endif
