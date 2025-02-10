/*
	Calendar
	
	iCalendar items
	[RFC5545 Internet Calendaring and Scheduling Core Object Specification (iCalendar)]
	
	2019/05/19	Originated
	
	Copyright © 2019-2023 by: Ben Hekster
	
	We've basically implemented based on what we've experienced.  The entire spec ought to be
	gone through, top to bottom, and conformance checked with unit tests.
*/

#include <assert.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstring>
#include <ctime>
#include <string>

#include "Calendar.h"



/*	DateTime::MakeForNowUTC
	Create a date-time-zone for the current date-time in UTC
*/
template <typename Char>
typename Calendar<Char>::DateTime Calendar<Char>::DateTime::MakeForNowUTC()
{
DateTime result;

// get current time
std::time_t now;
if (std::time(&now) == static_cast<std::time_t>(-1)) throw errno;

// convert to UTC
std::tm nowComponents;
if (const errno_t error = gmtime_s(&nowComponents, &now)) throw error;

// create structure
result.fDate.fYear = 1900 + nowComponents.tm_year;
result.fDate.fMonth0 = nowComponents.tm_mon;
result.fDate.fDay0 = nowComponents.tm_mday - 1;
result.fTime.fHour = nowComponents.tm_hour;
result.fTime.fMinute = nowComponents.tm_min;
result.fTime.fSecond = nowComponents.tm_sec;
result.fTime.fZone = Time::Zone::kUTC;

return result;
}


/*	ParseAction

*/
template <typename Char>
typename Calendar<Char>::Action Calendar<Char>::Parser::ParseAction(
	string_view	status
	)
{
// list of action parsers
const static struct Parser {
	const Char	*key;
	Calendar::Action action;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"AUDIO", Action::kAudio },
	{ L"DISPLAY", Action::kDisplay },
	{ L"EMAIL", Action::kEMail },
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), status);
return parser != std::end(parsers) ? parser->action : Action::kOther;
}


/*	ParseClassification

*/
template <typename Char>
typename Calendar<Char>::Classification Calendar<Char>::Parser::ParseClassification(
	string_view	classification
	)
{
// list of status parsers
const static struct Parser {
	const Char	*key;
	Calendar::Classification classification;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"CONFIDENTIAL", Classification::kConfidential },
	{ L"PRIVATE", Classification::kPrivate },
	{ L"PUBLIC", Classification::kPublic },
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), classification);
return parser != std::end(parsers) ? parser->classification : Classification::kOther;
}


/*	ParseDate
	Parse "date" (§3.3.4)
*/
template <typename Char>
typename Calendar<Char>::Date Calendar<Char>::Parser::ParseDate(
	string_view	date
	)
{
if (date.size() != 8) throw "unexpected date format";

/* I used to have this built on std::from_chars, which operates directly on a string_view;
   unfortunately, only the narrow-character string_view is available.  sscanf() actually seems
   a better solution; but then it's a little harder to transparently support the narrow and wide
   versions.  This way is transparent; rely on short string optimization. */
Date result;
result.fYear = static_cast<unsigned short>(stoul(string(string_view(date.data() + 0, 4))));
result.fMonth0 = static_cast<unsigned char>(stoul(string(string_view(date.data() + 4, 2))) - 1);
result.fDay0 = static_cast<unsigned char>(stoul(string(string_view(date.data() + 6, 2))) - 1);

return result;
}


/*	ParseDateTime
	Parse Date-Time (§3.3.5)
	Implemented:
		FORM #1: DATE WITH LOCAL TIME ("19980118T230000", 15 characters)
		FORM #2: DATE WITH UTC TIME ("19980119T070000Z", 16 characters)
		*** FORM #3
*/
template <typename Char>
typename Calendar<Char>::DateTime Calendar<Char>::Parser::ParseDateTime(
	string_view	created
	)
{
DateTime result;

// parse date and time components
if (created.size() < 8 + 1 + 6) throw "unexpected date/time format";
result.fDate = ParseDate(created.substr(0, 8));
if (created[8] != 'T') throw "no date-time separator";
result.fTime.fHour = static_cast<unsigned char>(stoul(string(string_view(created.data() + 9, 2))));
result.fTime.fMinute = static_cast<unsigned char>(stoul(string(string_view(created.data() + 11, 2))));
result.fTime.fSecond = static_cast<unsigned char>(stoul(string(string_view(created.data() + 13, 2))));

// local time zone?
if (created.size() == 15)
	result.fTime.fZone = Time::Zone::kNone;

else if (created.size() == 16) {
	if (created[15] != 'Z') throw "unexpected Zulu indicator";
	result.fTime.fZone = Time::Zone::kUTC;
	}

else
	throw "unexpected time zone suffix";

return result;
}


/*	ParseDuration
	Parse Duration (§3.3.6)
*/
template <typename Char>
typename Calendar<Char>::Duration Calendar<Char>::Parser::ParseDuration(
	string_view	duration
	)
{
Duration result;
result.fStyle = Duration::Style::kNone;

// sign
switch (duration.at(0)) {
	case '+':	result.fNegative = false; duration.remove_prefix(1); break;
	case '-':	result.fNegative = true; duration.remove_prefix(1); break;

	default:	result.fNegative = false;
	}

// duration indicator
if (duration.at(0) != 'P') throw "invalid duration";
duration.remove_prefix(1);

while (!duration.empty()) {
	if (duration.at(0) == 'T') {
		duration.remove_prefix(1);

		switch (result.fStyle) {
			case Duration::Style::kNone:	result.fStyle = Duration::Style::kTime; break;
			case Duration::Style::kDate:	result.fStyle = Duration::Style::kDateTime; break;
			default:			throw "invalid duration";
			}

		result.fFrom = Duration::Unit::kNone;
		}

	// parse number
	typename Duration::Value value; {
		size_t length;
		value = static_cast<Duration::Value>(stoul(string(string_view(duration.data(), duration.size())), &length));
		duration.remove_prefix(length);
		}

	// parse signifier
	switch (duration.at(0)) {
		case 'W':
			switch (result.fStyle) {
				case Duration::Style::kNone:	result.fStyle = Duration::Style::kWeek; break;
				default:			throw "invalid duration";
				}

			result.fWeek = value;
			break;

		case 'D':
			switch (result.fStyle) {
				case Duration::Style::kNone:	result.fStyle = Duration::Style::kDate; break;
				default:			throw "invalid duration";
				}

			result.fDay = value;
			break;

		case 'H':
			switch (result.fStyle) {
				case Duration::Style::kDateTime:
				case Duration::Style::kTime:
					switch (result.fFrom) {
						case Duration::Unit::kNone:	result.fFrom = Duration::Unit::kHour; break;
						default:			throw "invalid duration";
						}
					break;
				default:			throw "invalid duration";
				}

			result.fTo = Duration::Unit::kMinute;
			result.fHours = value;
			break;

		case 'M':
			switch (result.fStyle) {
				case Duration::Style::kDateTime:
				case Duration::Style::kTime:
					switch (result.fFrom) {
						case Duration::Unit::kNone:	result.fFrom = Duration::Unit::kMinute; break;
						case Duration::Unit::kHour:	if (result.fTo != Duration::Unit::kMinute) throw "invalid duration";
						default:			throw "invalid duration";
						}
					break;
				default:			throw "invalid duration";
				}

			result.fTo = Duration::Unit::kSecond;
			result.fMinutes = value;
			break;

		case 'S':
			switch (result.fStyle) {
				case Duration::Style::kDateTime:
				case Duration::Style::kTime:
					switch (result.fFrom) {
						case Duration::Unit::kNone:	result.fFrom = Duration::Unit::kSecond; break;
						case Duration::Unit::kHour:
						case Duration::Unit::kMinute:	if (result.fTo != Duration::Unit::kSecond) throw "invalid duration";
						default:			throw "invalid duration";
						}
					break;
				default:			throw "invalid duration";
				}

			result.fTo = Duration::Unit::kNone;
			result.fMinutes = value;
			break;
		}

	duration.remove_prefix(1);
	}

return result;
}


/*	RecurrenceRule::Parser::ParseFrequency

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseFrequency(
	string_view	rule
	)
{
static const struct {
	const Char	*key;
	Unit		value;

	operator	const string_view() const { return key; }
	} frequencies[] = {
	{ L"DAILY", Unit::kDaily },
	{ L"HOURLY", Unit::kHourly },
	{ L"MINUTELY", Unit::kMinutely },
	{ L"MONTHLY", Unit::kMonthly },
	{ L"SECONDLY", Unit::kSecondly },
	{ L"WEEKLY", Unit::kWeekly },
	{ L"YEARLY", Unit::kYearly }
	};

// find the lower bound of key
const auto *const frequency = std::lower_bound(std::begin(frequencies), std::end(frequencies), rule);
if (frequency == std::end(frequencies) || frequency->key != rule) throw "unexpected recurrence frequency";

Frequency(frequency->value);
}


/*	RecurrenceRule::Parser::ParseUntil

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseUntil(
	string_view	until
	)
{
// DATE-TIME?
if (until.length() == 15 || until.length() == 16)
	Until(Calendar::Parser::ParseDateTime(until));

// DATE?
else if (until.length() == 8)
	Until(ParseDate(until));

else
	throw "improper recurrence until";
}


/*	RecurrenceRule::Parser::ParseCount

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseCount(
	string_view	rule
	)
{
}


/*	RecurrenceRule::Parser::ParseInterval
	Parse
*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseInterval(
	string_view	rule
	)
{
// convert to integer
size_t intervalL;
unsigned interval = stoul(string(string_view(rule.data(), rule.size())), &intervalL);
if (intervalL != rule.size()) throw "invalid recurrence rule interval";

Interval(interval);
}


/*	RecurrenceRule::Parser::ParseBySecond

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseBySecond(
	string_view	rule
	)
{
}


/*	RecurrenceRule::Parser::ParseByMinute

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByMinute(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseByHour

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByHour(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseByDay

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByDay(
	string_view	bywdaylist
	)
{
// for all weekdays
for (
	size_t separatorI;

	!bywdaylist.empty();
	
	bywdaylist.remove_prefix(separatorI != std::string_view::npos ? separatorI + 1 : bywdaylist.length())
	) {
	// find the weekday
	separatorI = bywdaylist.find_first_of(',');
	string_view weekdaynum = bywdaylist.substr(0, separatorI != std::string_view::npos ? separatorI : bywdaylist.length());
	
	// ordinal sign
	bool negative;
	switch (weekdaynum.at(0)) {
		case '+':	negative = false; weekdaynum.remove_prefix(1); break;
		case '-':	negative = true; weekdaynum.remove_prefix(1); break;

		default:	negative = false;
		}

	// ordinal value
	signed char ordinal;
	if (std::isdigit(weekdaynum.at(0))) {
		size_t ordinalL;
		ordinal = stoi(string(string_view(weekdaynum.data(), weekdaynum.length())), &ordinalL);
		if (ordinalL == 0) throw "invalid recurrence by-day ordinal";
		
		weekdaynum.remove_prefix(ordinalL);
		if (negative) ordinal = -ordinal;
		}

	else
		ordinal = 0;
	
	// parse weekday
	static const Char *const weekdays[] = {
		L"SU",
		L"MO",
		L"TU",
		L"WE",
		L"TH",
		L"FR",
		L"SA"
		};
	const Char *const *weekday;
	for (weekday = weekdays; weekday != std::end(weekdays); ++weekday)
		if (weekdaynum == *weekday) break;
	if (weekday == std::end(weekdays)) throw "invalud recurrence by-day weekday";
	const Weekday w = static_cast<Weekday>(1 + (weekday - weekdays));

	ByDay(w, ordinal);
	}
}


/*	Parser::RecurrenceRule::ParseByMonthDay

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByMonthDay(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseByYearDay

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByYearDay(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseByWeekNumber

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByWeekNumber(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseByMonth

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseByMonth(
	string_view	months
	)
{
// for all months
for (
	size_t separatorI;
	
	!months.empty();
	
	months.remove_prefix(separatorI != std::string_view::npos ? separatorI + 1 : months.length())
	) {
	// find the end of this month
	separatorI = months.find_first_of(',');
	const unsigned nextL = separatorI != std::string_view::npos ? separatorI : months.length();
	
	// parse month
	unsigned monthL;
	unsigned char month0 = static_cast<unsigned char>(stoul(string(string_view(months.data(), nextL)), &monthL)) - 1;
	if (monthL != nextL) throw "invalid recurrence month";
	
	ByMonth0(month0);
	}
}


/*	Parser::RecurrenceRule::ParseBySetPos

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseBySetPos(
	string_view	rule
	)
{
}


/*	Parser::RecurrenceRule::ParseWkst

*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::ParseWkst(
	string_view	rule
	)
{
}


/*	ParseRecurrenceRule
	Parse Recurrence Rule (§3.8.5.3)
*/
template <typename Char>
void Calendar<Char>::Parser::RecurrenceRule::Parse(
	string_view	rules
	)
{
const static struct {
	const Char	*key;
	void		(RecurrenceRule::*Parser)(string_view);

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"BYDAY", &RecurrenceRule::ParseByDay },
	{ L"BYHOUR", &RecurrenceRule::ParseByHour },
	{ L"BYMINUTE", &RecurrenceRule::ParseByMinute },
	{ L"BYMONTH", &RecurrenceRule::ParseByMonth },
	{ L"BYMONTHDAY", &RecurrenceRule::ParseByMonthDay },
	{ L"BYSECOND", &RecurrenceRule::ParseBySecond },
	{ L"BYSETPOS", &RecurrenceRule::ParseBySetPos },
	{ L"BYWEEKNO", &RecurrenceRule::ParseByWeekNumber },
	{ L"BYYEARDAY", &RecurrenceRule::ParseByYearDay },
	{ L"COUNT", &RecurrenceRule::ParseCount },
	{ L"FREQ", &RecurrenceRule::ParseFrequency },
	{ L"INTERVAL", &RecurrenceRule::ParseInterval },
	{ L"UNTIL", &RecurrenceRule::ParseUntil },
	{ L"WKST", &RecurrenceRule::ParseWkst }
	};

// for all recurrence rule parts
for (
	size_t separatorI;

	!rules.empty();
	
	rules.remove_prefix(separatorI != string_view::npos ? separatorI + 1 : rules.length())
	) {
	// find the end of this rule part
	separatorI = rules.find_first_of(';');
	const string_view rule(rules.data(), separatorI != string_view::npos ? separatorI : rules.length());

	// find the name/value separator
	const size_t valueSeparatorI = rule.find_first_of('=');
		if (valueSeparatorI == string_view::npos) throw "couldn't find name/value separator";

	// define the rule part key and value
	const string_view
		key = rule.substr(0, valueSeparatorI),
		value = rule.substr(valueSeparatorI + 1);

	// find the parser
	const auto *const p = std::find(std::begin(parsers), std::end(parsers), key);
		if (!p) throw "unexpected recurrence rule part key";

	// invoke
	(this->*p->Parser)(value);
	}
}


/*	ParseScale

*/
template <typename Char>
typename Calendar<Char>::Scale Calendar<Char>::Parser::ParseScale(
	string_view	scale
	)
{
// list of scale parsers
const static struct Parser {
	const Char	*key;
	Calendar::Scale status;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"GREGORIAN", Scale::kGregorian},
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), scale);

return parser != std::end(parsers) ? parser->status : Scale::kOther;
}


/*	ParseStatusEvent

*/
template <typename Char>
typename Calendar<Char>::StatusEvent Calendar<Char>::Parser::ParseStatusEvent(
	string_view	status
	)
{
// list of status parsers
const static struct Parser {
	const Char	*key;
	Calendar::StatusEvent status;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"CANCELLED", StatusEvent::kCancelled },
	{ L"CONFIRMED", StatusEvent::kConfirmed},
	{ L"TENTATIVE", StatusEvent::kTentative },
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), status);
if (parser == std::end(parsers)) throw "unexpected event status";

return parser->status;
}


/*	ParseStatusToDo

*/
template <typename Char>
typename Calendar<Char>::StatusToDo Calendar<Char>::Parser::ParseStatusToDo(
	string_view	status
	)
{
// list of status parsers
const static struct Parser {
	const Char	*key;
	Calendar::StatusToDo status;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"CANCELLED", StatusToDo::kCancelled },
	{ L"COMPLETED", StatusToDo::kCompleted },
	{ L"IN-PROCESS", StatusToDo::kInProcess },
	{ L"NEEDS-ACTION", StatusToDo::kNeedsAction }
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), status);
if (parser == std::end(parsers)) throw "unexpected todo status";

return parser->status;
}



/*	ParseTransparency

*/
template <typename Char>
typename Calendar<Char>::Transparency Calendar<Char>::Parser::ParseTransparency(
	string_view	transparency
	)
{
// list of parsers
const static struct Parser {
	const Char	*key;
	Calendar::Transparency transparency;

	operator	const string_view() const { return key; }
	} parsers[] = {
	{ L"OPAQUE", Transparency::kOpaque },
	{ L"TRANSPARENT", Transparency::kTransparent },
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), transparency);
if (parser == std::end(parsers)) throw "unexpected transparency";

return parser->transparency;
}


/*	ParseUTCOffset
	Parse UTC Offset (§3.3.14)
*/
template <typename Char>
typename Calendar<Char>::UTCOffset Calendar<Char>::Parser::ParseUTCOffset(
	string_view	offset
	)
{
UTCOffset result;

// optional sign
bool negative = false;
if (offset.length() < 1) throw "invalid UTC Offset";
switch (offset[0]) {
	case '-':	negative = true; [[fallthrough]];
	case '+':	offset.remove_prefix(1); [[fallthrough]];
	default:	break;
	}

// parse hours and minutes
if (offset.length() < 4) throw "invalid UTC Offset";
result.fHour = static_cast<unsigned char>(stoul(string(string_view(offset.data(), 2))));
if (negative) result.fHour = -result.fHour;
offset.remove_prefix(2);
result.fMinute = static_cast<unsigned char>(stoul(string(string_view(offset.data(), 2))));
offset.remove_prefix(2);

// parse optional seconds
if (offset.length() >= 2) {
	result.fSecond = static_cast<unsigned char>(stoul(string(string_view(offset.data(), 2))));
	offset.remove_prefix(2);
	}

else
	result.fSecond = 0;

if (!offset.empty()) throw "invalid UTC Offset";

return result;
}


/*	ParseValueType
	Parse value type (§3.2.20)
*/
template <typename Char>
typename Calendar<Char>::ValueType Calendar<Char>::Parser::ParseValueType(
	string_view	value
	)
{
static const struct Parser {
	const Char	*name;
	ValueType	type;

	operator	const string_view() const { return name; }
	} parsers[] = {
	{ L"BINARY", ValueType::kBinary },
	{ L"BOOLEAN", ValueType::kBoolean },
	{ L"CAL-ADDRESS", ValueType::kCalendarAddress },
	{ L"DATE", ValueType::kDate },
	{ L"DATE-TIME", ValueType::kDateTime },
	{ L"DURATION", ValueType::kDuration },
	{ L"FLOAT", ValueType::kFloat },
	{ L"INTEGER", ValueType::kInteger },
	{ L"PERIOD", ValueType::kPeriod },
	{ L"RECUR", ValueType::kRecurrence },
	{ L"TEXT", ValueType::kText },
	{ L"URI", ValueType::kURI },
	{ L"UTC-OFFSET", ValueType::kUTCOffset }
	};

// look for the key
const Parser *const parser = std::find(std::begin(parsers), std::end(parsers), value);
if (parser == std::end(parsers)) throw "unexpected property value type";

return parser->type;
}



/*	Calendar::Parser

*/
template <typename Char>
Calendar<Char>::Parser::Parser(
	const Context	&context,
	istream		&input
	) :
	fInput(input),
	fContext(&context)
{
}


/*	BeginAlarm
	Start parsing an ALARM
*/
template <typename Char>
void Calendar<Char>::Parser::BeginAlarm()
{
ParseAlarm();
}


/*	EndTimeAlarm
	Complete parsing an ALARM
*/
template <typename Char>
bool Calendar<Char>::Parser::EndAlarm()
{
return true;
}


/*	Begin
	Parse BEGIN lines
*/
template <typename Char>
void Calendar<Char>::Parser::Begin(
	string_view	parameters,
	string_view	value
	)
{
// look for the key
const Context *const inner = std::find(fContext->fInner, fContext->fInnerE, value);
if (inner == fContext->fInnerE) throw "unexpected BEGIN";

// set inner context
assert(inner->fOuter == fContext);
fContext = inner;

// notify
if (void (Parser::*InnerBegin)() = fContext->FBegin) (this->*InnerBegin)();
}


/*	End
	Parse END lines
*/
template <typename Char>
bool Calendar<Char>::Parser::End(
	string_view	parameters,
	string_view	value
	)
{
// ensure we're ending the current context
if (value != fContext->fName) throw "unexpected END";

// notify
bool doReturn = false;
if (bool (Parser::*InnerEnd)() = fContext->FEnd) doReturn = (this->*InnerEnd)();

// set previous context
fContext = fContext->fOuter;

return doReturn;
}


/*	Property
	Parse property lines
*/
template <typename Char>
void Calendar<Char>::Parser::Property(
	const Context	&component,
	const typename Context::Line &line,
	string_view	parameters,
	string_view	value
	)
{
// for all property parameters
for (
	size_t separatorI;

	!parameters.empty();
	
	parameters.remove_prefix(separatorI != string_view::npos ? separatorI + 1 : parameters.length())
	) {
	// find the end of this parameter
	separatorI = parameters.find_first_of(';');
	const string_view parameter(parameters.data(), separatorI != string_view::npos ? separatorI : parameters.length());

	// find the parameter name/value-list separator
	const size_t nameValuesSeparatorI = parameter.find_first_of('=');
	if (nameValuesSeparatorI == std::string_view::npos) throw "no parameter name-value separator";
	const string_view
		name(parameter.substr(0, nameValuesSeparatorI)),
		value(parameter.substr(nameValuesSeparatorI + 1));

	// find a parser based on the parameter name
	if (
		// find the lower bound of key
		const Context::Line::Parameter *const parameter = std::lower_bound(line.fParameters, line.fParametersE, name);

		// is it the key we're looking for?
		parameter != line.fParametersE && *parameter == name
		)
		// invoke property parameter parser
		(this->*parameter->Callback)(value);

	// else invoke the default parser
	else if (void (Parser::*ExtraParameter)(string_view, string_view, string_view) = component.FExtraParameter)
		(this->*ExtraParameter)(line.key, name, value);
	}

// invoke property parser
(this->*line.Callback)(value);
}


/*	Calendar::Parser::()
	iCalendar is a line-oriented format; but with some twists:
	*	lines are key [; optional parameters ]: value
	*	content lines can have continuation lines (see our 'CalDAVIAdapter')
	
	FHackNextLine allows a property parser to invoke a specific property parser (possibly the same one)
	in case the next line can't be parsed as expected (see DynamicCalendar::XAppleStructuredLocation).
*/
template <typename Char>
void Calendar<Char>::Parser::operator()()
{
// for each line in the input stream
for (string line; std::getline(fInput, line); ) {
	const string_view view = line;
	
	// reset 'hack' next line parser
	void (Parser::*const hackNextLine)(string_view) = FHackNextLine;
	FHackNextLine = nullptr;
	
	// find the name/value separator
	const size_t colonI = view.find_first_of(L':');
		if (colonI == string_view::npos) {
			// invoke 'hack' next line parser if we have one
			if (hackNextLine) {
				(this->*hackNextLine)(view);
				
				continue;
				}
			
			throw "couldn't find name/value separator";
			}
	
	// find the optional name/param separator
	const size_t semicolonI = view.substr(0, colonI).find_first_of(';');
	const bool hasParam = semicolonI != string_view::npos;
	
	// define the key, parameters, and value
	const string_view
		key = view.substr(0, hasParam ? semicolonI : colonI),
		parameters = view.substr(semicolonI + 1, hasParam ? colonI - (semicolonI + 1) : 0),
		value = view.substr(colonI + 1);
	if (
		// find the lower bound of key
		const Context::Line *const line = std::lower_bound(fContext->fLines, fContext->fLinesE, key);

		// is it the key we're looking for?
		line != fContext->fLinesE && *line == key
		)
		Property(*fContext, *line, parameters, value);
	
	else if (key == L"BEGIN")
		Begin(parameters, value);
	
	else if (key == L"END")
		{ if (End(parameters, value)) break; }
	
	// unrecognized line for 'hack' parser?
	else if (hackNextLine)
		(this->*hackNextLine)(view);
	
	// unspecified keys
	else if (void (Parser::*ExtraLine)(string_view, string_view, string_view) = fContext->FExtraLine)
		(this->*ExtraLine)(key, parameters, value);
	}
}


// explicit instantiation
//template struct Calendar<char>;
template struct Calendar<wchar_t>;