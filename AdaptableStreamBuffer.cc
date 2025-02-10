/*
	AdaptableStreamBuffer
	
	streambuf adapters
	
	2018/09/09	Originated
	2023/03/29	Refactored as generic adapter stream
	
	Copyright © 2018-2023 by: Ben Hekster

REFERENCES:
	https://en.cppreference.com/w/cpp/io/basic_streambuf/
	http://www.angelikalanger.com/IOStreams/Excerpt/excerpt.htm
*/

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <fstream>

#include "AdaptableStreamBuffer.h"


/*

	Splicer

*/

/*	Splicer
	Represent a spliceable area of memory
	
	'begin' to 'end' represents the memory to be spliced;
	'end' to 'limit' is available for overflow if the spliced data
	becomes longer than the original input
*/
template <typename Char>
Splicer<Char>::Splicer(
	Char		*begin,
	Char		*end,
	const Char	*limit
	) :
	fBegin(begin),
	fOut(begin),
	fIn(begin),
	fEnd(end),
	fOver(end),
	fLimit(limit)
{
assert(end >= begin);
assert(limit >= end);
}


/*	~Splicer
	Stop splicing
*/
template <typename Char>
Splicer<Char>::~Splicer()
{
// assert that we processed all of the input
/* It is possible to handle the case where this is not so; just not sure that it is useful. */
assert(fIn == fEnd);

// is there any overflow?
if (
	signed overflow = fOut - fIn;
	overflow > 0
	)
	// is the overflow disconnected from the end of processed data?
	if (fOver > fOut)
		// move to connect overflow to end
		std::copy(fOver, fOver + overflow, fEnd);
}


/*	<<
	Insert a character to output
*/
template <typename Char>
Splicer<Char> &Splicer<Char>::operator<<(
	Char		c
	)
{
// will overflow?
if (
	signed overflow = fOut - fIn;
	overflow >= 0
	) {
	// ensure there is overflow buffer available
	/* *** note that if fOver > fEnd we can move the overflow area */
	assert(fOver + overflow < fLimit);
	
	fOver[overflow] = c;
	}

else
	*fOut = c;

/* note that we account for the output character even if we didn't actually store it there */
fOut++;

return *this;
}


/*	read
	Postincrement consume another character from input
*/
template <typename Char>
Char Splicer<Char>::read()
{
const Char result = *fIn;

// is overflowed?
if (fIn < fOut) {
	// move one character from overflow to where the input was consumed
	*fIn++ = *fOver++;
	
	// overflow is now empty?
	if (fIn == fOut)
		// restart overflow at the beginning of the overflow area
		fOver = fEnd;
	}

else
	fIn++;

return result;
}


/*	++
	Consumed another character from input
	
	Note that this preincrement is probably NOT what you want.  You might think to use it
	in the following way:
	
		for (; ci; ++ci) ci << transform(*ci);
	
	The problem here is that this will want to write the transformed 'c' before the
	input 'c' is consumed; therefore requiring one character of overflow.  If this can be
	guaranteed, then there is no problem; otherwise, it may fail at run time.
	
	Better would be
	
		while (ci) ci << transform(*ci++);
	
	but postincrement is costly to implement (and so it isn't).  Instead, use read():
	
		while (ci) ci << transform(ci.read());
*/
template <typename Char>
Splicer<Char> &Splicer<Char>::operator++()
{
// is overflowed?
if (
	signed overflow = fOut - fIn;
	overflow > 0
	) {
	// move one character from overflow to where the input was consumed
	*fIn++ = *fOver++;
	
	// overflow is now empty?
	if (fIn == fOut)
		// restart overflow at the beginning of the overflow area
		fOver = fEnd;
	}

else
	fIn++;

return *this;
}


// explicit instantiation
template struct Splicer<char>;
template struct Splicer<wchar_t>;
