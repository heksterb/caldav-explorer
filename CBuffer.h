/*
	CBuffer
	
	Simple buffer
	
	2023/03/03	Originated
	
	Copyright © 2023 by: Ben Hekster
*/

#pragma once

#include <stdlib.h>

#include <new>


/*	CBuffer
	Simple buffer of nonconstructible (POD) data types
	
	malloc
	*	fine with me, but then we need a destructor
	*	have to deal with failure to allocate
	operator new[]
	*	also allocates (and keeps track of) the array length
	*	needlessly initializes the array
	*	can't be reallocated
	vector
	*	distinguishes between occupied and preallocated storage
	*	generally has too much semantics
*/
template <typename T>
struct CBuffer {
	using Value = T;

protected:
	unsigned	fLength;
	T		*fStorage;

public:
			CBuffer(size_t);
			~CBuffer();
	
	operator	T*() { return fStorage; }
	operator	const T*() const { return fStorage; }
	
	T		*Begin() { return fStorage; }
	const T		*Begin() const { return const_cast<CBuffer<T>*>(this)->Begin(); }
	T		*End() { return fStorage + fLength; }
	const T		*End() const { return const_cast<CBuffer<T>*>(this)->End(); }
	void		Reallocate(size_t);
	size_t		Length() const { return fLength; }
	};


template <typename T>
inline CBuffer<T>::CBuffer(
	size_t		length
	) :
	fLength(length),
	fStorage(static_cast<T*>(calloc(length, sizeof(T))))
	{
	if (!fStorage) throw std::bad_alloc();
	}


template <typename T>
inline CBuffer<T>::~CBuffer() {
	free(fStorage);
	}


template <typename T>
inline void CBuffer<T>::Reallocate(
	size_t		length
	) {
	// try to reallocate
	T *const storage = static_cast<T*>(realloc(fStorage, length * sizeof(T)));
	
	// if failed, old storage was not freed
	if (!storage) throw std::bad_alloc();
	
	fStorage = storage;
	fLength = length;
	}
