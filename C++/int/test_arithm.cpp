#include <iostream>
using namespace std;

int main() {
	short x = -32768; // -32768
	short y = 32767; // 32767
	short sum = x+y; // -1
	cout << x << y << sum << endl;
	cout << short(sum)+short(-x) << endl; // should be sum-x = 32767
}