#include "SmartPtr.hpp"
#include <iostream>
using namespace std;

struct TestObj {
	TestObj(int i) : _data(i) { }

	~TestObj() {
		cout << "Obj: " << _data << " gone." << endl;
	}

	void print() {
		cout << "Obj has data: " << _data << endl;
	}

private:
	int _data;
};


int main() {
	TestObj obj(1);
	auto sp1 = make_smart_ptr(obj);
	auto sp2 = sp1;
	auto sp3(sp2);
	sp2 -> print();
	sp3 -> print();

	return 0;
}