// The classic: now as state machine!
//
// Compile like this:
// g++ Macho.cpp HelloWorld.cpp

#include "Macho.hpp"

#include <iostream>
using namespace std;


namespace HelloWorld {
	TOPSTATE(Top) {
		STATE(Top)

	private:
		void entry() { cout << "Hello World!" << endl; }
		void exit() { cout << "Goodbye world!" << endl; }
	};
}

int main() {
	Macho::Machine<HelloWorld::Top> m;

	return 0;
}
