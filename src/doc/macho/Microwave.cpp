// A simple simulation of an microwave oven:
// Timer can be programmed in minutes, last mode will be resumed if opened.
//
// Compile like this:
// g++ Macho.cpp Microwave.cpp

#include <iostream>
using namespace std;

#include "Macho.hpp"


// Simple microwave simulation
namespace Microwave {

	//////////////////////////////////////////////////////////////////////
	// State declarations

	// Machine's top state
	TOPSTATE(Top) {

		// Top state data (visible to all substates)
		struct Box {
			Box() : myCookingTime(0) {}
			void printTimer() { cout << "  Timer set to " << myCookingTime << " minutes" << endl; }
			void incrementTimer() { ++myCookingTime; }
			void decrementTimer() { -- myCookingTime; }
			void resetTimer() { myCookingTime = 0; }
			int getRemainingTime() { return myCookingTime; }
		private:
			int myCookingTime;
		};

		STATE(Top)

		// Machine's event protocol
		virtual void open() {}
		virtual void close() {}
		virtual void minute() {}	// Increment timer by a minute
		virtual void start() {}		// Start cooking
		virtual void stop() {}		// Stop cooking
		virtual void tick() {}		// Minute has passed

	private:
		// Initial entry action
		void init();
	};

	// Microwave has been opened
	SUBSTATE(Disabled, Top) {
		STATE(Disabled)

		// Event handler
		void close();

	private:
		// Entry and exit actions of state
		void entry();
		void exit();
	};

	// Microwave is ready
	SUBSTATE(Operational, Top) {
		STATE(Operational)

		// State has history enabled
		DEEPHISTORY()

		void open();
		void stop();

	private:
		void init();
	};

	// Microwave is idling
	SUBSTATE(Idle, Operational) {
		STATE(Idle)

		void minute();

	private:
		void entry();
	};

	// Microwave is being programmed
	SUBSTATE(Programmed, Operational) {
		STATE(Programmed)

		void minute();
		void start();
	};

	// Microwave is heating
	SUBSTATE(Cooking, Programmed) {
		STATE(Cooking)

		void tick();

	private:
		void entry();
		void exit();
	};


	//////////////////////////////////////////////////////////////////////
	// Event handler and box implementations

	// Top state
	void Top::init() {
		setState<Operational>();
	}

	// State Disabled
	void Disabled::entry() {
		cout << "  Microwave opened" << endl;
	}
	void Disabled::exit() {
		cout << "  Microwave closed" << endl;
	}
	void Disabled::close() {
		setState<Operational>();
	}

	// State Operational
	void Operational::init() {
		setState<Idle>();
	}
	void Operational::open() {
		setState<Disabled>();
	}
	void Operational::stop() {
		setState<Idle>();
	}

	// State Idle
	void Idle::entry() {
		TOP::box().resetTimer();
		cout << "  Microwave ready" << endl;
	}
	void Idle::minute() {
		setState<Programmed>();
		dispatch(Event(&TOP::minute));
	}

	// State Programmed
	void Programmed::minute() {
		TOP::box().incrementTimer();
		TOP::box().printTimer();
	}
	void Programmed::start() {
		setState<Cooking>();
	}

	// State Cooking
	void Cooking::entry() {
		cout << "  Heating on" << endl;
	}
	void Cooking::exit() {
		cout << "  Heating off" << endl;
	}
	void Cooking::tick() {
		cout << "  Clock tick" << endl;

		TOP::Box & tb = TOP::box();
		tb.decrementTimer();
		if (tb.getRemainingTime() == 0) {
			cout << "  Finished" << endl;
			setState<Idle>();
		} else
			tb.printTimer();
	}

} // namespace Microwave


int main() {
	Macho::Machine<Microwave::Top> m;

	cout << "Lets cook ourself a TV dinner:" << endl;

	m->minute();
	m->minute();
	m->minute();
	m->start();
	m->tick();
	m->open();
	cout << "Adding a little spice..." << endl;
	m->close();
	m->tick();
	m->tick();

	cout << "Now there is the remote...?" << endl;

	return 0;
}
