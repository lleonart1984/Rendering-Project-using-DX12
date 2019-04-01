#pragma once

template<typename S>
class ManagedObject {
	friend S;
	template<typename T> friend class ManagedObject;

private:
	S *_this;
	volatile long* counter;

	void AddReference() {
		if (counter)
		{
			InterlockedAdd(counter, 1);
		}
	}

	void RemoveReference() {
		if (counter) {
			InterlockedAdd(counter, -1);
			if ((*counter) == 0) {
				delete _this;
				delete counter;
			}
		}
	}

public:
	ManagedObject() : _this(nullptr), counter(nullptr) {
	}
	ManagedObject(S* self) : _this(self), counter(self ? new long(1) : nullptr) {
	}

	inline bool isNull() const {
		return _this == nullptr;
	}

	// Copy constructor
	ManagedObject(const ManagedObject<S>& other) {
		this->counter = other.counter;
		this->_this = other._this;
		AddReference();
	}

	template <typename Subtype>
	ManagedObject(const ManagedObject<Subtype>& other) {
		this->counter = other.counter;
		this->_this = (S*)other._this;
		AddReference();
	}

	ManagedObject<S>& operator = (const ManagedObject<S>& other) {
		RemoveReference();
		this->counter = other.counter;
		this->_this = other._this;
		AddReference();
		return *this;
	}

	bool operator == (const ManagedObject<S> &other) {
		return other._this == _this;
	}

	bool operator != (const ManagedObject<S> &other) {
		return other._this != _this;
	}

	~ManagedObject() {
		RemoveReference();
	}

	//Dereference operator
	S& operator*()
	{
		return *_this;
	}
	//Member Access operator
	S* operator->()
	{
		return _this;
	}

	template<typename T>
	ManagedObject<T> Dynamic_Cast() {
		ManagedObject<T> obj;
		obj._this = dynamic_cast<T*>(_this);
		obj.counter = counter;
		obj.AddReference();
		return obj;
	}

	operator bool() const {
		return !isNull();
	}
};

template<typename S>
using gObj = ManagedObject<S>;