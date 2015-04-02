#include <iostream>
#include <iomanip>

typedef unsigned char* byte_ptr;
// check little/big endian
void show_bytes(byte_ptr start, int len) {
	using namespace std;
	for (int i = 0; i < len; ++i) {
		cout << setw(2) << setfill('0') << hex << int(start[i]) << " ";
	}
	cout << endl;
}
// generic function for all built-in types
template <typename T> 
void show_type(T x) {
	show_bytes((byte_ptr)&x, sizeof x);
}

int func1(unsigned word) {
	return (int)((word << 24) >> 24);
}

int func2(unsigned word) {
	return ((int) word << 24) >> 24;
}

int main() {
	int x = 0x12345678;
	show_type(0x12345678);
	show_type(&x); // my computer is indeed 64bit!
	std::cout << &x << std::endl;

	x = 0x87654321;
	show_type(func1(x));
	show_type(func2(x));

	short y = 0x1234;
	x = y;
	show_type(y);
	show_type(x);

	return 0;
}