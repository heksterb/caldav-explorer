/*
	AdaptableStreamBuffer
	
	streambuf adapters
		
	2018/09/09	Originated
	2023/03/29	Refactored as generic adapter stream
	
	Copyright © 2018-2023 by: Ben Hekster
	
	Consider this an experiment in managing templates through explicit instantiation.
	Of course, this only works if you can predict at build time all the template instantiations
	that will be used; which we do.  The situation might be different if we were building
	library code; but even in that case, I'm wondering whether it wouldn't be possible
	to maintain a separate source file that dealt with explicit instantiation so that mere
	users of a class aren't bothered with implementation source code.
	
	I consider it a design deficiency in the I/O Streams library that it's impossible
	to chain together 'filtering' streams that share buffers without resorting to this kind
	of templatization hack.
*/

#pragma once

#include <assert.h>
#include <stdlib.h>

#include <streambuf>

#include "CBuffer.h"


/*	Splicer
	Helper for transforming a character buffer in place; by either:
	*	translating characters
	*	deleting characters
	*	inserting characters
	If more characters are produced by the transformation than were originally present,
	the overflow is maintained.
	
	NOT designed to work with range-for
		
	CASE (1) one character out, one character in
	
		operator*		operator<<		operator++		
		I0 I1 I2 I3  		 O0 I1 I2 I3		 O0 I1 I2 I3		
		+--+--+--+--+		+--+--+--+--+		+--+--+--+--+		
		^			^  ^			   ^			
		|			|  |			   |			
		fOut			|  fOut			   fOut			
		fIn			fIn			   fIn			
		
	CASE (2) two characters out, one character in
	
		operator*		operator<<		operator<<		operator++   
		I0 I1 I2 I3  		 I0 I1 I2 I3		 I0 I1 I2 I3		 O0 I1 I2 I3		
		+--+--+--+--+		+--+--+--+--+		+--+--+--+--+		+--+--+--+--+		
		^			^  ^			^     ^	        	   ^  ^      
		|			|  |			|     |	        	   |  |			
		fOut			|  fOut			|     fOut      	   |  fOut		
		fIn			fIn			fIn          		   fIn          		
	
		operator*		operator<<		operator++   
		O0 I1 I2 I3  		 O0 I1 I2 I3		 O0 O1 I2 I3		
		+--+--+--+--+		+--+--+--+--+		+--+--+--+--+		
		^  ^      		   ^     ^		      ^  ^	        
		|  |      		   |     |		      |  |	        
		|  fOut   		   |     fOut		      |  fOut      
		fIn       		   fIn			      fIn          		
*/
template <typename Char>
struct Splicer {
protected:
	Char		*const fBegin,			// begin of data to be spliced
			*fOut,				// end of processed data
			*fIn,				// start of unprocessed data
			*const fEnd,			// end of data to be spliced
			*fOver;				// beginning of overflow (data that logically belongs after 'fOut')
	const Char	*const fLimit;			// end of overflow buffer area

public:
			Splicer(Char *begin, Char *end, const Char *limit);
			~Splicer();
	
	// any more data needing to be filtered?
	operator	bool() const { return fIn < fEnd; }
	
	Char		operator*() { return *fIn; }
	Splicer		&operator++();			// note that this is preincrement and probably NOT what you want
	Char		read();
	Splicer		&operator<<(Char);
	
	// return pointer past end of processed output
	Char		*end() const { return fOut; }
	};



/*	aistreambuf
	Input stream buffer that supports user-defined associated character sequence and
	transformations on them
*/
template <typename Char, class Adapter>
class aistreambuf : public std::basic_streambuf<Char> {
protected:
	using Base = typename std::basic_streambuf<Char>;
	using int_type = typename std::basic_streambuf<Char>::int_type;
	
	static constexpr unsigned kBufferSizeIncrement = 0x1000;
	CBuffer<Char>	fBuffer;
	Adapter		fAdapter;
	
	int_type	underflow() override;
	int		sync() override;

public:
	template <typename... Args>
			aistreambuf(Args&&...);
			aistreambuf(const aistreambuf&) = delete;
			~aistreambuf() = default;
	
	void		claimed(size_t);
	const Char	*data() const { return Base::gptr(); }
	size_t		size() const { return Base::egptr() - Base::gptr(); }
	};


/*	aistreambuf
	Present input stream buffer interface
*/
template <typename Char, class Adapter>
template <typename... Args>
aistreambuf<Char, Adapter>::aistreambuf(
	Args		&&...args
	) :
	fBuffer(kBufferSizeIncrement),
	fAdapter(std::forward<Args>(args)...)	
{
// buffer empty; available for reading into
Base::setg(fBuffer.Begin(), fBuffer.Begin(), fBuffer.Begin());
}


/*	claimed
	Indicate that the given amount of occupied get area space was consumed
*/
template <typename Char, class Adapter>
void aistreambuf<Char, Adapter>::claimed(
	size_t		size
	)
{
// move gptr() forward by that amount
Char *const p = Base::gptr() + size;
assert(p <= Base::egptr());
Base::setg(Base::eback(), p, Base::egptr());
}


/*	underflow
	Read data into buffer and return next available character
*/
template <typename Char, class Adapter>
typename std::basic_streambuf<Char>::int_type aistreambuf<Char, Adapter>::underflow()
{
// some data consumed but some is still present?
const size_t consumedL = Base::gptr() - Base::eback();
const size_t presentL = Base::egptr() - Base::gptr();
if (consumedL > 0 && presentL > 0)
	// consolidate data in get area to beginning of buffer
	std::copy(fBuffer.Begin() + consumedL, fBuffer.Begin() + consumedL + presentL, fBuffer.Begin());

// is buffer large enough?
const size_t availableL = fAdapter.available();
if (
	const size_t neededL = presentL + availableL * Adapter::kOverflowNumerator / Adapter::kOverflowDenominator;
	neededL > fBuffer.Length()
	)
	// reallocate buffer to next increment
	fBuffer.Reallocate(
		(neededL + (kBufferSizeIncrement - 1)) / kBufferSizeIncrement * kBufferSizeIncrement
		);

// house new data after end of get area
const size_t housedL = fAdapter.house(fBuffer.Begin() + presentL, availableL);

// process new data for the associated character sequence
Char *const adjustedE = fAdapter.filter(
	fBuffer.Begin() + presentL,			// beginning of new, unprocessed data
	fBuffer.Begin() + presentL + housedL,		// end of new, unprocessed data
	fBuffer.End()					// end of buffer
	);

// account for change in length of new data
Base::setg(fBuffer.Begin(), fBuffer.Begin(), adjustedE);

// return first byte of available data, or EOF
return Base::gptr() < Base::egptr() ? Base::traits_type::to_int_type(*Base::gptr()) : Base::traits_type::eof();
}


/*	sync
	Synchronize get area with underlying external character sequence
*/
template <typename Char, class Adapter>
int aistreambuf<Char, Adapter>::sync()
{
// force the put area to be flooded
(void) underflow();

// success
return 0;
}



/*	aostreambuf
	Generically adapt native text stream to a text buffer that is suitable for submission through HTTP
	
	Using this implies text; which implies CR/LF conversion and character set conversion;
	which implies that it is practically impossible to know in advance (even knowing the size
	of the unconverted input) the content size of the converted output; which implies that
	the entire content body will have to be generated prior to issuing the HTTP request;
	which implies that the content body won't be streamed; which means that there is no
	advantage in giving this class knowledge of the HTTP client; which means that we can
	just write this as a generic class that generates into a buffer
	
	Thought about using stringbuf/wstringbuf as base class to provide the buffering, but
	we need fairly direct control over that buffer:
	*	when wostreambuf needs to evict to nostreambuf we need to (partially)
		consume from the 'base' end of the string; unsure whether stringbuf
		expects a subclass to call setp() on itself
	*	when evicting into the nostreambuf we need to preallocate a specific amount
		of space past pptr(); I don't see a documented way of doing that
*/
template <typename Char, class Adapter>
class aostreambuf : public std::basic_streambuf<Char> {
protected:
	using Base = typename std::basic_streambuf<Char>;
	using int_type = typename std::basic_streambuf<Char>::int_type;
	
	static constexpr unsigned kBufferSizeIncrement = 0x1000;
	CBuffer<Char>	fBuffer;
	Adapter		fAdapter;
	
	int_type	overflow(int_type) override;
	int		sync() override;
	
public:
	template <typename... Args>
			aostreambuf(Args&&...);
			aostreambuf(const aostreambuf&) = delete;
			~aostreambuf() = default;
	
	Adapter		&adapter() { return fAdapter; }
	
	Char		*reserve(size_t);
	void		used(size_t);
	const Char	*data() const { return fBuffer.Begin(); }
	size_t		size() const { return Base::pbase() - fBuffer.Begin(); }
	};



/*	aostreambuf
	Present stream buffer interface for consumption by HTTP
*/
template <typename Char, class Adapter>
template <typename... Args>
aostreambuf<Char, Adapter>::aostreambuf(
	Args		&&...args
	) :
	fBuffer(kBufferSizeIncrement),
	fAdapter(std::forward<Args>(args)...)
{
// buffer empty; available for writing into
Base::setp(fBuffer.Begin(), fBuffer.Begin(), fBuffer.End());
}


/*	reserve
	Ensure at least the given amount of put area space is available,
	and return a pointer to it
*/
template <typename Char, class Adapter>
Char *aostreambuf<Char, Adapter>::reserve(
	size_t		size
	)
{
// need additional space?
assert(Base::epptr() >= Base::pptr());
if (
	const size_t availableL = Base::epptr() - Base::pptr();
	size > availableL
	) {
	const size_t
		// amount of data in buffer, but filtered, and 'evicted' but nowhere to go
		usedL = Base::pbase() - fBuffer.Begin(),
		
		// same, and including data in 'put area'
		presentL = Base::pptr() - fBuffer.Begin(),
		
		// amount of buffer space needed to accomodate reservation request
		neededL = presentL + size;
	
	// reallocate
	/* *** probably a buffer chain would make more sense than a repeated monstrous reallocation */
	fBuffer.Reallocate(
		(neededL + (kBufferSizeIncrement - 1)) / kBufferSizeIncrement * kBufferSizeIncrement
		);
	
	Base::setp(fBuffer.Begin() + usedL, fBuffer.Begin() + presentL, fBuffer.End());
	}

// return available put area
return Base::pptr();
}


/*	used
	Indicate that the given amount of reserved put area space was populated
*/
template <typename Char, class Adapter>
void aostreambuf<Char, Adapter>::used(
	size_t		size
	)
{
// account
Char *const p = Base::pptr() + size;
assert(p <= Base::epptr());
Base::setp(Base::pbase(), p, Base::epptr());

// flush immediately
sync();
}


/*	overflow
	Make space available in the put area
	
	If filtered data isn't evicted, buffer space before the put area (ie, before pbase)
	is considered part of the external character sequence
*/
template <typename Char, class Adapter>
typename std::basic_streambuf<Char>::int_type aostreambuf<Char, Adapter>::overflow(
	int_type	c
	)
{
// process new data for the associated character sequence
const Char *adjustedE = fAdapter.filter(
	Base::pbase(),					// beginning of new, unprocessed data
	Base::pptr(),					// end of new, unprocessed data
	fBuffer.End()					// end of buffer
	);

// try to evict all prepared data
const size_t presentL = adjustedE - fBuffer.Begin();
const size_t evictedL = fAdapter.evict(fBuffer.Begin(), presentL);
assert(evictedL <= presentL);
const size_t remainingL = presentL - evictedL;

// some but not all data was evicted?
if (evictedL > 0 && remainingL > 0)
	// move data in put area to beginning of buffer
	std::copy(fBuffer.Begin() + evictedL, fBuffer.Begin() + presentL, fBuffer.Begin());

// resize the buffer if needed
const size_t neededL = remainingL + (c != Base::traits_type::eof());
if (neededL > fBuffer.Length()) {
	// reallocate buffer to next increment
	const size_t reallocatedL = (neededL + (kBufferSizeIncrement - 1)) / kBufferSizeIncrement * kBufferSizeIncrement;
	fBuffer.Reallocate(reallocatedL);
	
	// account after relocation and resizing
	Base::setp(fBuffer.Begin() + remainingL, fBuffer.Begin() + remainingL, fBuffer.Begin() + reallocatedL);
	}

else
	// account
	Base::setp(fBuffer.Begin() + remainingL, fBuffer.Begin() + remainingL, Base::epptr());

// buffer the character
if (c != Base::traits_type::eof())
	*Base::pptr() = c, Base::pbump(+1);

return c;
}


/*	sync
	Synchronize put area with underlying external character sequence
*/
template <typename Char, class Adapter>
int aostreambuf<Char, Adapter>::sync()
{
// force the put area to be evicted
(void) overflow(std::streambuf::traits_type::eof());

// success
return 0;
}
