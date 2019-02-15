#pragma once
#include <memory>
template<typename T>
class list
{
	T* elements;
	int count;
	int capacity;
public:
	list() {
		capacity = 32;
		count = 0;
		elements = new T[capacity];
	}
	void reset() {
		count = 0;
	}
	list(std::initializer_list<T> initialElements) {
		capacity = max(32, initialElements.size());
		count = initialElements.size();
		elements = new T[capacity];
		for (int i = 0; i < initialElements.size(); i++)
			elements[i] = initialElements[i];
	}
	~list() {
		delete[] elements;
	}
	void add(T item) {
		if (count == capacity)
		{
			capacity = (int)(capacity*1.3);
			T* newelements = new T[capacity];
			for (int i = 0; i < count; i++)
				newelements[i] = elements[i];
			delete[] elements;
			elements = newelements;
		}
		elements[count++] = item;
	}
	inline T& operator[](int index) const {
		return elements[index];
	}
	inline T& first() const {
		return elements[0];
	}
	inline T& last() const {
		return elements[count - 1];
	}
	inline int size() const {
		return count;
	}
};

template<class T>
using gList = ManagedObject<list<T>>;

//class MemoryLinkedList {
//	int* queueStarts;
//	UINT64* queueSignals;
//	int beg, end;
//	int count;
//	int freeStart = 0;
//	int capacity;
//	int maxSize;
//public:
//	MemoryLinkedList(int maxSize, int capacity):capacity(capacity), maxSize(maxSize) {
//		queueStarts = new int[capacity];
//		queueSignals = new UINT64[capacity];
//		count = 0;
//		beg = 0;
//		end = -1;
//	}
//
//	int malloc(int size) {
//
//		if (freeStart + size > maxSize)
//			freeStart = 0;
//
//		int ptr = freeStart;
//
//		freeStart += size;
//		
//		return ptr;
//	}
//
//	void signal(UINT64 value) {
//
//	}
//
//	void freeUntil(UINT64 value) {
//	}
//};