#ifndef SMARTPTR_H
#define SMARTPTR_H

#include <iostream>
using namespace std;

template <typename T>
class SmartPointer {
public:
	// constructor
	SmartPointer(const T& obj) : _ptr(new T(obj)), _refCount(new int(0)) {
		increaseRef();
	
	}

	// copy constructor
	SmartPointer(const SmartPointer<T>& sp) : _ptr(sp._ptr), _refCount(sp._refCount) {
		increaseRef();
	}

	// destructor
	~SmartPointer() {
		decreaseRef();
	}

	// dereference * operator
	T& operator*() {
		return (*_ptr);
	}

	T* operator->() {
		return _ptr;
	}

	// copy assignment
	SmartPointer<T>& operator=(SmartPointer<T>& sp) {
		if (this != &sp) {
			decreaseRef();
			_ptr = sp._ptr;
			_refCount = sp._refCount;
			increaseRef();
		}
		return *this;
	}


private:
	T* _ptr;
	int* _refCount;

	void increaseRef() {
		(*_refCount)++;
		cout << "Smart Pointer reference cout: " << *_refCount << endl;
	}

	void decreaseRef() {
		(*_refCount)--;
		cout << "Smart Pointer reference cout: " << *_refCount << endl;
		if (*_refCount == 0) {
			cout << "Smart pointer release data" << endl;
			delete _refCount;
			delete _ptr;
		}
	}
};

template <typename T>
SmartPointer<T> make_smart_ptr(const T& obj) {
	return {obj};
}

#endif