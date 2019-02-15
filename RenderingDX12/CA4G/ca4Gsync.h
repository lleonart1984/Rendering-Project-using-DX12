#pragma once

//#define SHOW_SYNC_DEBUG

#include <Windows.h>

class Semaphore {
	HANDLE handle;
#ifdef SHOW_SYNC_DEBUG
	int currentCounter;
	int maxCount;
	bool waiting = false;
#endif
public:

	Semaphore(int initialCount, int maxCount)
#ifdef SHOW_SYNC_DEBUG
		:currentCounter(initialCount), maxCount(maxCount) 
#endif
	{
		handle = CreateSemaphore(nullptr, initialCount, maxCount, nullptr);
	}
	~Semaphore() {
		CloseHandle(handle);
	}

	inline bool Wait() {
#ifdef SHOW_SYNC_DEBUG
		waiting = true;
#endif
		auto result = WaitForSingleObject(handle, INFINITE);
#ifdef SHOW_SYNC_DEBUG
		waiting = false;
		currentCounter--;
#endif
		return result == WAIT_OBJECT_0;
	}

	inline void Release() {
		ReleaseSemaphore(handle, 1, nullptr);
#ifdef SHOW_SYNC_DEBUG
		currentCounter++;
#endif
	}
};

class Mutex {
	HANDLE handle;
#ifdef SHOW_SYNC_DEBUG
	bool locked = false;
#endif
public:
	inline Mutex() {
		handle = CreateMutex(nullptr, false, nullptr);
	}
	~Mutex() {
		CloseHandle(handle);
	}

	inline bool Acquire() {
#ifdef SHOW_SYNC_DEBUG
		locked = true;
#endif
		return WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0;
	}

	inline void Release() {
		ReleaseMutex(handle);
#ifdef SHOW_SYNC_DEBUG
		locked = false;
#endif
	}
};

class CountEvent {
	int count = 0;
	Mutex mutex;
	Semaphore goSemaphore;
public:
	inline CountEvent() :mutex(Mutex()), goSemaphore(Semaphore(1, 1)) {
	}

	inline void Increment() {
		mutex.Acquire();
		if (this->count == 0)
			goSemaphore.Wait();
		this->count++;
		mutex.Release();
	}

	inline void Wait() {
		goSemaphore.Wait();
		goSemaphore.Release();
	}

	inline void Signal() {
		mutex.Acquire();
		this->count--;
		if (this->count == 0)
			goSemaphore.Release();
		mutex.Release();
	}
};

template<typename T>
class ProducerConsumerQueue {
	T* elements;
	int elementsLength;
	int start;
	int count;
	Semaphore productsSemaphore;
	Semaphore spacesSemaphore;
	Mutex mutex;
	bool closed;
public:
	ProducerConsumerQueue(int capacity) : 
		productsSemaphore(Semaphore(0, capacity)),
		spacesSemaphore(Semaphore(capacity, capacity)),
		mutex(Mutex()) {
		elementsLength = capacity;
		elements = new T[elementsLength];
		start = 0;
		count = 0;
		closed = false;
	}
	~ProducerConsumerQueue() {
		delete[] elements;
	}
	inline int getCount() { return count; }

	bool TryConsume(T& element)
	{
		if (productsSemaphore.Wait())
		{
			// ensured there is an element to be consumed
			mutex.Acquire();
			element = elements[start];
			start = (start + 1) % elementsLength;
			count--;
			spacesSemaphore.Release();
			mutex.Release();
			return true;
		}
		return false;
	}

	bool TryProduce(T element) {
		if (spacesSemaphore.Wait())
		{
			mutex.Acquire();
			int enqueuePos = (start + count) % elementsLength;
			elements[enqueuePos] = element;
			count++;
			productsSemaphore.Release();
			mutex.Release();
			return true;
		}
		return false;
	}
};
