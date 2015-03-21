#include <iostream>
using namespace std;

// Base class for Unpolymorphism
class UnpolyBase {
public:
	UnpolyBase() { }

	~UnpolyBase() {
		cout << "UnpolyBase gone" << endl;
	}
};

// Base class for Polymorphism
class PolyBase : public UnpolyBase {
public:
	PolyBase() { }

	virtual ~PolyBase() {
		cout << "PolyBase gone" << endl;
	}

	virtual void print() const {
		cout << "I'm PolyBase" << endl;
	}
};

class PolyDerived : public PolyBase {
public:
	PolyDerived() { }

	~PolyDerived() {
		cout << "PolyDerived gone" << endl;
	}

	void print() const {
		cout << "I'm PolyDerived" << endl;
	}
};

class UnpolyDerived : public UnpolyBase {
public:
	UnpolyDerived() { }

	~UnpolyDerived() {
		cout << "UnpolyDerived gone" << endl;
	}

	void print() const {
		cout << "I'm UnpolyDerived" << endl;
	}
};

/*
*	Test case to show that we can safely delete the pointer with the 
* type of a base class, which is designed to use polymorphically by
* making the d'tor virtual.
*/
void test1() {
	cout << "Test 1: polymorphical usage" << endl;
	PolyBase* ptr = new PolyDerived;
	ptr->print();
	delete ptr;
	cout << endl;
}

/*
* Test case to show that even the class is inherited from some class
* that is not used for polymorphism, we can still ensure that d'tor will
* be called correctly. This is because we are using the derived class
* 'unpolymorphically', which means that we don't use a pointer/ref w/ type
* of base class to points to an instance that is actually of derived type.
*/
void test2() {
	cout << "Test 2: non-polymorphical usage" << endl;
	UnpolyDerived* ptr = new UnpolyDerived;
	ptr->print();
	delete ptr;
	cout << endl;
}

/*
* Test case to generate runtime error. Here we can see that only Unploy's d'tor 
* is called, b/c its d'tor is non-virtual. C++ does not knowe about PolyBase or
* PolyDerived part. Memory Leak!
*/
void test3_rtmerror() {
	cout << "Test 3: runtime error" << endl;
	UnpolyBase* ptr = new PolyDerived;
	delete ptr;
	cout << endl;
}

int main() {
	test1();
	test2();
	#if 0
	test3_rtmerror();
	#endif
	return 0;
}