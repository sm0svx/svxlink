// Just a small state machine (3 states) as demonstration.
//
// Compile like this:
// g++ Macho.cpp Example.cpp

#include "Macho.hpp"

#include <iostream>
using namespace std;


namespace Example {

	////////////////////////////////////////////////////////
	// State declarations

	// Machine's top state
	TOPSTATE(Top) {
		// Top state variables (visible to all substates)
		struct Box {
			Box() : data(0) {}
			long data;
		};

		STATE(Top)

		// Machine's event protocol
		virtual void event1(int i) {}
		virtual void event2(long l) {}

	private:
		// special actions
		void entry();
		void exit();
		void init();
		void init(int);
	};

	// A superstate
	SUBSTATE(Super, Top) {
		STATE(Super)

		// This state has history
		HISTORY()

	private:
		// Entry and exit actions of state
		void entry();
		void exit();
	};

	// A substate
	SUBSTATE(StateA, Super) {
		// State variables
		struct Box {
			Box() : data(0) {}
			int data;
		};

		STATE(StateA)

		// Event handler
		void event1(int i);

	private:
		void entry();
		void init(int);
		void exit();
	};

	// A substate
	SUBSTATE(StateB, Super) {
		STATE(StateB)

		void event2(long l);

	private:
		void entry();
		void exit();
	};

	////////////////////////////////////////////////////////
	// Event handler implementations

	// Top state
	void Top::entry() { cout << "Top::entry" << endl; }
	void Top::exit() { cout << "Top::exit" << endl; }
	void Top::init() {
		// Initialize state with box
		setState<StateA>(44);
	}
	void Top::init(int i) {
		box().data = i;
		init();
	}

	// Super state
	void Super::entry() { cout << "Super::entry" << endl; }
	void Super::exit() { cout << "Super::exit" << endl; }

	// StateA state
	void StateA::entry() { cout << "StateA::entry" << endl; }
	void StateA::init(int i) { cout << "StateA::init " << i << endl; }
	void StateA::exit() { cout << "StateA::exit" << endl; }
	void StateA::event1(int i) {
		box().data = i;
		cout << "StateA::box().data: " << box().data << endl;
		setState<StateB>();
	}

	// StateB state
	void StateB::entry() { cout << "StateB::entry" << endl; }
	void StateB::exit() { cout << "StateB::exit" << endl; }
	void StateB::event2(long l) {
		Top::box().data = l;
		cout << "Top::box().data: " << Top::box().data << endl;
		setState<StateA>(42);
	}

} // namespace Example


//////////////////////////////////////////////////////////////////////
// Test run
int main() {
	using namespace Macho;

	// Initialize state machine with some data
	Machine<Example::Top> m(State<Example::Top>(11));

	// Dispatch some events
	m->event1(42);
	m->event2(43);

	// Inspect state machine
	cout << "m.box().data: " << m.box().data << endl;

	return 0;
}

/*
   Output is:

   Top::entry
   Super::entry
   StateA::entry
   StateA::init 44
   StateA::box().data: 42
   StateA::exit
   StateB::entry
   Top::box().data: 43
   StateB::exit
   StateA::entry
   StateA::init 42
   m.box().data: 43
   StateA::exit
   Super::exit
   Top::exit
*/
