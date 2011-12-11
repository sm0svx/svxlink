// Comprehensive tests of state machine features.
//
// Compile like this:
// (don't forget defining the MACHO_SNAPSHOTS symbol)
// g++ -D MACHO_SNAPSHOTS Macho.cpp Test.cpp

#include "Macho.hpp"

#include <map>
#include <vector>
#include <iostream>
#include <string>

using namespace std;


////////////////////////////////////////////////////////////////////////////////
// Tests for proper entry/exit, box and history handling.
enum {
	STATE_TOP = 1 << 0,
	STATE_A   = 1 << 1,
	STATE_AA  = 1 << 2,
	STATE_AB  = 1 << 3,
	STATE_AAA = 1 << 4,
	STATE_AAB = 1 << 5,
	STATE_ABA = 1 << 6,
	STATE_ABB = 1 << 7,
	STATE_B   = 1 << 8,
	STATE_BA  = 1 << 9,
	STATE_BB  = 1 << 10,
	STATE_BAA = 1 << 11,
	STATE_BAB = 1 << 12,
	STATE_BBA = 1 << 13,
	STATE_BBB = 1 << 14,
	STATE_C   = 1 << 15,
	STATE_CA  = 1 << 16,
	STATE_CB  = 1 << 17,
	STATE_CAA = 1 << 18,
	STATE_CAB = 1 << 19,
	STATE_CBA = 1 << 20,
	STATE_CBB = 1 << 21,
	STATE_X   = 1 << 22
};

namespace Transitions {

	long boxes;

	void box_created(long state) {
		boxes |= state;
	}
	void box_destroyed(long state) {
		boxes &= ~state;
	}
	bool has_box(long state) {
		return (boxes & state) == boxes;
	}

	bool at(unsigned pos, const map<long, size_t> & m, long state) {
		map<long, size_t>::const_iterator it = m.find(state);
		if (it == m.end())
			return false;

		return it->second == pos;
	}

	void add(long state, map<long, size_t> & m) {
		size_t c = m.size();
		m[state] = c;
	}


	TOPSTATE(Top) {
		struct Box {
			Box() : data(0) { clear(); box_created(STATE_TOP); }
			Box(int i) : data(i) { clear(); box_created(STATE_TOP); }
			~Box() { box_destroyed(STATE_TOP); }

			// Registering all exits and entries
			void entry(long state) { add(state, entries); }
			void exit(long state) { add(state, exits); }
			void init(long state) { add(state, inits); }

			void clear() const { entries.clear(); exits.clear(); inits.clear(); }

			mutable map<long, size_t> entries;
			mutable map<long, size_t> exits;
			mutable map<long, size_t> inits;

			mutable int data;
		};

		STATE(Top)

		virtual void event() {}

	private:
		void entry() { box().entry(STATE_TOP); }
		void exit() { box().exit(STATE_TOP); }
		void init() { box().init(STATE_TOP); }
	};


	SUBSTATE(StateA, Top) {
		struct Box {
			Box() : data(0) { box_created(STATE_A); }
			~Box() { box_destroyed(STATE_A); }
			int data;
		};

		STATE(StateA)

	private:
		// Testing proper box handling here
		void entry() { TOP::box().entry(STATE_A); box().data = 42; }
		void exit() { TOP::box().exit(STATE_A); assert(box().data == 42); }
		void init() { TOP::box().init(STATE_A); }
	};


	SUBSTATE(StateAA, StateA) {
		STATE(StateAA)

	private:
		// Testing proper box handling here
		void entry() { TOP::box().entry(STATE_AA); assert(StateA::box().data == 42); StateA::box().data = 43; }
		void exit() { TOP::box().exit(STATE_AA); assert(StateA::box().data == 43); StateA::box().data = 42; }
		void init() { TOP::box().init(STATE_AA); }
	};


	SUBSTATE(StateAAA, StateAA) {
		struct Box {
			Box() : data(0) {}
			~Box() {}
			int data;
		};
		PERSISTENT()
		STATE(StateAAA)

		void testInference();

	private:
		void entry() { TOP::box().entry(STATE_AAA); assert(StateA::box().data == 43); }
		void exit() { TOP::box().exit(STATE_AAA); assert(StateA::box().data == 43); }
		void init() { TOP::box().init(STATE_AAA); }

		// Trying to mess up type inference here
		void init(const long & i) { TOP::box().init(STATE_AAA); box().data = i; }
		void init(const void *) { TOP::box().init(STATE_AAA); box().data = 0; }
	};


	SUBSTATE(StateAAB, StateAA) {
		struct Box {
			Box() { box_created(STATE_AAB); }
			~Box() { box_destroyed(STATE_AAB); }
		};

		STATE(StateAAB)

	private:
		void entry() { TOP::box().entry(STATE_AAB); assert(StateA::box().data == 43); }
		void exit() { TOP::box().exit(STATE_AAB); assert(StateA::box().data == 43); }
		void init() { TOP::box().init(STATE_AAB); }
	};


	SUBSTATE(StateAB, StateA) {
		STATE(StateAB)

	private:
		void entry() { TOP::box().entry(STATE_AB); assert(StateA::box().data == 42); }
		void exit() { TOP::box().exit(STATE_AB); assert(StateA::box().data == 42); }
		void init() { TOP::box().init(STATE_AB); }
	};


	SUBSTATE(StateABA, StateAB) {
		STATE(StateABA)

	private:
		void entry() { TOP::box().entry(STATE_ABA); assert(StateA::box().data == 42); }
		void exit() { TOP::box().exit(STATE_ABA); assert(StateA::box().data == 42); }
		void init() { TOP::box().init(STATE_ABA); }
	};


	SUBSTATE(StateABB, StateAB) {
		STATE(StateABB)

	private:
		void entry() { TOP::box().entry(STATE_ABB); assert(StateA::box().data == 42); }
		void exit() { TOP::box().exit(STATE_ABB); assert(StateA::box().data == 42); }
		void init() { TOP::box().init(STATE_ABB); }
	};


	SUBSTATE(StateB, Top) {
		STATE(StateB)
		HISTORY()

	private:
		void entry() { TOP::box().entry(STATE_B); }
		void exit() { TOP::box().exit(STATE_B); }
		void init() { TOP::box().init(STATE_B); }
	};


	SUBSTATE(StateBA, StateB) {
		STATE(StateBA)

	private:
		void entry() { TOP::box().entry(STATE_BA); }
		void exit() { TOP::box().exit(STATE_BA); }
		void init() { TOP::box().init(STATE_BA); }
	};


	SUBSTATE(StateBAA, StateBA) {
		STATE(StateBAA)

	private:
		void entry() { TOP::box().entry(STATE_BAA); }
		void exit() { TOP::box().exit(STATE_BAA); }
		void init() { TOP::box().init(STATE_BAA); }
	};


	SUBSTATE(StateBAB, StateBA) {
		STATE(StateBAB)

	private:
		void entry() { TOP::box().entry(STATE_BAB); }
		void exit() { TOP::box().exit(STATE_BAB); }
		void init() { TOP::box().init(STATE_BAB); }
	};


	SUBSTATE(StateBB, StateB) {
		STATE(StateBB)

	private:
		void entry() { TOP::box().entry(STATE_BB); }
		void exit() { TOP::box().exit(STATE_BB); }
		void init() { TOP::box().init(STATE_BB); }
	};


	SUBSTATE(StateBBA, StateBB) {
		STATE(StateBBA)

	private:
		void entry() { TOP::box().entry(STATE_BBA); }
		void exit() { TOP::box().exit(STATE_BBA); }
		void init() { TOP::box().init(STATE_BBA); }
	};


	SUBSTATE(StateBBB, StateBB) {
		STATE(StateBBB)

	private:
		void entry() { TOP::box().entry(STATE_BBB); }
		void exit() { TOP::box().exit(STATE_BBB); }
		void init() { TOP::box().init(STATE_BBB); }
	};


	SUBSTATE(StateC, Top) {
		STATE(StateC)
		DEEPHISTORY()

	private:
		void init();
		void init(const bool &);
		void init(int, int, int, int);
		void entry() { TOP::box().entry(STATE_C); }
		void exit() { TOP::box().exit(STATE_C); }
	};


	SUBSTATE(StateCA, StateC) {
		STATE(StateCA)
		DEEPHISTORY()

	private:
		void init();
		void entry() { TOP::box().entry(STATE_CA); }
		void exit() { TOP::box().exit(STATE_CA); }
	};

	SUBSTATE(StateCAA, StateCA) {
		struct Box {
			Box() : data(0) { box_created(STATE_CAA); }
			~Box() { box_destroyed(STATE_CAA); }
			int data;
		};

		STATE(StateCAA)

	private:
		void entry() { TOP::box().entry(STATE_CAA); }
		void exit() { TOP::box().exit(STATE_CAA); }
		void init() { TOP::box().init(STATE_CAA); }

		// Trying to mess up type inference here
		void init(const unsigned long & i, int = 0) { TOP::box().init(STATE_CAA); box().data = i; }
		void init(const char *, int = 0) { TOP::box().init(STATE_CAA); box().data = 0; }
	};


	SUBSTATE(StateCAB, StateCA) {
		STATE(StateCAB)

	private:
		void entry() { TOP::box().entry(STATE_CAB); }
		void exit() { TOP::box().exit(STATE_CAB); }
		void init() { TOP::box().init(STATE_CAB); }
	};


	SUBSTATE(StateCB, StateC) {
		STATE(StateCB)

	private:
		void entry() { TOP::box().entry(STATE_CB); }
		void exit() { TOP::box().exit(STATE_CB); }
		void init() { TOP::box().init(STATE_CB); }
	};


	SUBSTATE(StateCBA, StateCB) {
		STATE(StateCBA)

	private:
		void entry() { TOP::box().entry(STATE_CBA); }
		void exit() { TOP::box().exit(STATE_CBA); }
		void init() { TOP::box().init(STATE_CBA); }
	};


	SUBSTATE(StateCBB, StateCB) {
		STATE(StateCBB)

	private:
		void entry() { TOP::box().entry(STATE_CBB); }
		void exit() { TOP::box().exit(STATE_CBB); }
		void init() { TOP::box().init(STATE_CBB); }
	};


	SUBSTATE(StateX, Top) {
		struct Box {
			Box() : state(StateX::alias()) { box_created(STATE_X); }
			~Box() { box_destroyed(STATE_X); }

			Macho::Alias state;
		};

		STATE(StateX)

		void event() { setState(box().state); }

	private:
		void entry() { TOP::box().entry(STATE_X); }
		void exit() { TOP::box().exit(STATE_X); }
		void init() { TOP::box().init(STATE_X); }
		void init(const Macho::Alias & s) { TOP::box().init(STATE_X); box().state = s; }
	};


	void StateC::init() { TOP::box().init(STATE_C); setState<StateCA>(); }
	void StateC::init(const bool &) {
		// Test access of static methods
		assert(StateC::isCurrent(machine()));
		assert(StateC::alias() == machine().currentState());

		TOP::box().init(STATE_C);
		Macho::Alias history = StateC::history(machine());

		if (history != StateC::alias())
			setState(history);
		else
			setState<StateCA>();
	}
	void StateC::init(int, int, int, int) {
	}

	void StateCA::init() { TOP::box().init(STATE_CA); setState<StateCAA>(); }

	// won't be executed: just to test compile time type inference
	void StateAAA::testInference() {
		setState<StateAAA>(42);
		setState<StateAAA>(0);
		setState<StateAAA>( (const char *) "test");

		setState<StateCAA>(42, 0);
		setState<StateCAA>(0, 0);
		setState<StateCAA>(0);
		setState<StateCAA>( (const char *) "test", 0);
	}

} // namespace Transitions


////////////////////////////////////////////////////////////////////////////////
// Testing dispatch mechanism.
enum {
	EVENT1,
	EVENT2,
	EVENT3,
	STATEA_ENTRY,
	STATEB_ENTRY
};


namespace Dispatch {

	TOPSTATE(Top) {
		typedef std::vector<int> Box;

		STATE(Top)

		virtual void clear() {
			box().clear();
		}
		virtual void event1(int i) {
			assert(i == 1);
			box().push_back(EVENT1);
		}
		virtual void event2(int i, bool b) {
			assert(i == 2); assert(!b);
			box().push_back(EVENT2);
		}
		virtual void event3(int i, bool b) {
			assert(i == 3); assert(b);
			box().push_back(EVENT3);
		}
	};


	SUBSTATE(StateA, Top) {
		STATE(StateA)

		virtual void event3(int i, bool b);

	private:
		void entry() {
			Top::box().push_back(STATEA_ENTRY);
		}
	};


	SUBSTATE(StateB, Top) {
		STATE(StateB)

		virtual void event3(int i, bool b);

	private:
		void entry() {
			Top::box().push_back(STATEB_ENTRY);
		}
	};


	void StateA::event3(int i, bool b) {
		SUPER::event3(i, b);
		setState(StateB::alias());	// Test setting state with alias
		dispatch(Event(&Top::event1, 1));
	}

	void StateB::event3(int i, bool b) {
		SUPER::event3(i, b);
		dispatch(Event(&Top::event1, 1));
		setState<StateA>();
	}

} // namespace Dispatch


////////////////////////////////////////////////////////////////////////////////
// Tests for template states.
namespace Templates {
	TOPSTATE(Top) {
		STATE(Top);

		virtual void event1() {}
		virtual void event2() {}

	private:
		void init();
	};

	SUBSTATE(StateA, Top) {
		STATE(StateA)
		DEEPHISTORY()

		typedef struct StateB Other;

		virtual void event1();
	};

	SUBSTATE(StateB, Top) {
		STATE(StateB)
		DEEPHISTORY()

		typedef struct StateA Other;

		virtual void event1();
	};

	template<class T>
	TSUBSTATE(TTop, T) {
		TSTATE(TTop)
		HISTORY()
	};

	template<class T>
	TSUBSTATE(TState1, TTop<T>) {
		TSTATE(TState1)

		virtual void event2();
	};

	template<class T>
	TSUBSTATE(TState2, TTop<T>) {
		TSTATE(TState2)

		virtual void event2();
	};

	template<class T>
	TSUBSTATE(TState3, TTop<T>) {
		TSTATE(TState3)

		virtual void event1();
		virtual void event2();
	};


	void Top::init() { setState<TState1<StateA> >(); }

	void StateA::event1() { setState(TState1<StateB>::alias()); }
	void StateB::event1() { setState(TState1<StateA>::alias()); }

	template<class T>
	void TState1<T>::event2() { setState<TState2<typename T::Other> >(); }

	template<class T>
	void TState2<T>::event2() { setState<TState1<typename T::Other> >(); }

	template<class T>
	void TState3<T>::event1() { setState<TState3<Macho::Anchor<StateA, 1> > >(); }

	template<class T>
	void TState3<T>::event2() { setState<TState3<Macho::Anchor<StateA, 0> > >(); }

} // namespace Templates


////////////////////////////////////////////////////////////////////////////////
// Helper functions to access protected members
class TestAccess {
public:
	template<typename T>
	static void setState(Macho::Machine<typename T::TOP> & m) {
		m.setState(T::_getInstance(m), &Macho::_theDefaultInitializer);
	}

	template<typename T, class P1>
	static void setState(Macho::Machine<typename T::TOP> & m, P1 p1) {
		m.setState(T::_getInstance(m), new Macho::_Initializer1<T, P1>(p1));
	}

	static void setState(Macho::_MachineBase & m, const Macho::Alias & state) {
		m.setState(state);
	}

	template<typename T>
	static void setStateHistory(Macho::Machine<typename T::TOP> & m) {
		m.setState(T::_getInstance(m), &Macho::_theHistoryInitializer);
	}

	template<typename T>
	static typename T::Box * getBox(Macho::Machine<typename T::Top> & m) {
		return & static_cast<T&>(m.myCurrentState->specification()).T::box();
	}
};


////////////////////////////////////////////////////////////////////////////////
// Tests for proper entry/exit, box and history handling.
void testTransitions() {
	using namespace Transitions;

	boxes = 0;

	Macho::Machine<Top> m(new Top::Box(42));

	assert(at(0, m.box().entries, STATE_TOP));
	assert(m.box().exits.empty());
	m.box().clear();

	// Parent -> Child
	TestAccess::setState<StateB>(m);
	assert(StateB::isCurrent(m));
	assert(StateB::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_B));
	assert(m.box().exits.empty());
	assert(at(0, m.box().inits, STATE_B));
	assert(has_box(STATE_TOP));
	m.box().clear();

	// Self transition with History
	TestAccess::setStateHistory<StateB>(m);
	assert(StateB::isCurrent(m));
	assert(StateB::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_B));
	assert(at(0, m.box().exits, STATE_B));
	assert(at(0, m.box().inits, STATE_B));
	assert(has_box(STATE_TOP));
	m.box().clear();

	// Sibling -> Sibling
	TestAccess::setState<StateA>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_A));
	assert(at(0, m.box().exits, STATE_B));
	assert(at(0, m.box().inits, STATE_A));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Self transition
	TestAccess::setState<StateA>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_A));
	assert(at(0, m.box().exits, STATE_A));
	assert(at(0, m.box().inits, STATE_A));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Parent -> Child
	TestAccess::setState<StateAA>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() != m.currentState());
	assert(StateAA::isCurrent(m));
	assert(StateAA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_AA));
	assert(m.box().exits.empty());
	assert(at(0, m.box().inits, STATE_AA));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Child -> Parent
	TestAccess::setState<StateA>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_A));
	assert(at(0, m.box().exits, STATE_AA));
	assert(at(1, m.box().exits, STATE_A));
	assert(at(0, m.box().inits, STATE_A));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Parent -> Child*
	TestAccess::setState(m, Macho::State<StateAAA>(42));
	assert(StateA::isCurrent(m));
	assert(StateA::alias() != m.currentState());
	assert(StateAA::isCurrent(m));
	assert(StateAA::alias() != m.currentState());
	assert(StateAAA::isCurrent(m));
	assert(StateAAA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_AA));
	assert(at(1, m.box().entries, STATE_AAA));
	assert(m.box().exits.empty());
	assert(at(0, m.box().inits, STATE_AAA));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Check persistent box
	TestAccess::setState<StateABB>(m);
	TestAccess::setState<StateAAA>(m);
	assert(TestAccess::getBox<StateAAA>(m)->data == 42);
	m.box().clear();

	// Sibling* -> Sibling*
	TestAccess::setState<StateABB>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() != m.currentState());
	assert(StateAB::isCurrent(m));
	assert(StateAB::alias() != m.currentState());
	assert(StateABB::isCurrent(m));
	assert(StateABB::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_AB));
	assert(at(1, m.box().entries, STATE_ABB));
	assert(at(0, m.box().exits, STATE_AAA));
	assert(at(1, m.box().exits, STATE_AA));
	assert(at(0, m.box().inits, STATE_ABB));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Child -> Parent*
	TestAccess::setState<StateA>(m);
	assert(StateA::isCurrent(m));
	assert(StateA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_A));
	assert(at(0, m.box().exits, STATE_ABB));
	assert(at(1, m.box().exits, STATE_AB));
	assert(at(2, m.box().exits, STATE_A));
	assert(at(0, m.box().inits, STATE_A));
	assert(has_box(STATE_TOP | STATE_A));
	m.box().clear();

	// Sibling -> Child* of Sibling
	TestAccess::setState<StateBAA>(m);
	assert(StateB::isCurrent(m));
	assert(StateB::alias() != m.currentState());
	assert(StateBA::isCurrent(m));
	assert(StateBA::alias() != m.currentState());
	assert(StateBAA::isCurrent(m));
	assert(StateBAA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_B));
	assert(at(1, m.box().entries, STATE_BA));
	assert(at(2, m.box().entries, STATE_BAA));
	assert(at(0, m.box().exits, STATE_A));
	assert(at(0, m.box().inits, STATE_BAA));
	assert(has_box(STATE_TOP));
	m.box().clear();

	// Child* of Sibling -> Sibling with Init
	TestAccess::setState<StateC>(m);
	assert(StateC::isCurrent(m));
	assert(StateC::alias() != m.currentState());
	assert(StateCA::isCurrent(m));
	assert(StateCA::alias() != m.currentState());
	assert(StateCAA::isCurrent(m));
	assert(StateCAA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAA));
	assert(at(0, m.box().exits, STATE_BAA));
	assert(at(1, m.box().exits, STATE_BA));
	assert(at(2, m.box().exits, STATE_B));
	assert(at(0, m.box().inits, STATE_C));
	assert(at(1, m.box().inits, STATE_CA));
	assert(at(2, m.box().inits, STATE_CAA));
	assert(has_box(STATE_TOP | STATE_CAA));
	m.box().clear();

	// Child* of Sibling -> Sibling with Shallow History
	TestAccess::setStateHistory<StateB>(m);
	assert(StateB::isCurrent(m));
	assert(StateB::alias() != m.currentState());
	assert(StateBA::isCurrent(m));
	assert(StateBA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_B));
	assert(at(1, m.box().entries, STATE_BA));
	assert(at(0, m.box().exits, STATE_CAA));
	assert(at(1, m.box().exits, STATE_CA));
	assert(at(2, m.box().exits, STATE_C));
	assert(at(0, m.box().inits, STATE_BA));
	assert(has_box(STATE_TOP));
	m.box().clear();

	// Child of Sibling -> Sibling with Deep History
	TestAccess::setStateHistory<StateC>(m);
	assert(StateC::isCurrent(m));
	assert(StateC::alias() != m.currentState());
	assert(StateCA::isCurrent(m));
	assert(StateCA::alias() != m.currentState());
	assert(StateCAA::isCurrent(m));
	assert(StateCAA::alias() == m.currentState());
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAA));
	assert(at(0, m.box().exits, STATE_BA));
	assert(at(1, m.box().exits, STATE_B));
	assert(at(0, m.box().inits, STATE_CAA));
	assert(has_box(STATE_TOP | STATE_CAA));
	TestAccess::getBox<StateCAA>(m)->data = 42;
	m.box().clear();

#ifdef MACHO_SNAPSHOTS
	// Test machine snapshots
	cout << endl << "Testing snapshots" << endl;
	long old_boxes = boxes;
	Macho::Snapshot<Top> s(m);

	for (int i = 0; i < 2; ++i) {
#endif
		// Child -> Parent with Deep History
		TestAccess::setStateHistory<StateC>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCA::isCurrent(m));
		assert(StateCA::alias() != m.currentState());
		assert(StateCAA::isCurrent(m));
		assert(StateCAA::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_C));
		assert(at(1, m.box().entries, STATE_CA));
		assert(at(2, m.box().entries, STATE_CAA));
		assert(at(0, m.box().exits, STATE_CAA));
		assert(at(1, m.box().exits, STATE_CA));
		assert(at(2, m.box().exits, STATE_C));
		assert(at(0, m.box().inits, STATE_CAA));
		assert(has_box(STATE_TOP | STATE_CAA));
		m.box().clear();

		// Sibling -> Sibling, then Child -> Parent with Deep History
		TestAccess::setState<StateCAB>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCA::isCurrent(m));
		assert(StateCA::alias() != m.currentState());
		assert(StateCAB::isCurrent(m));
		assert(StateCAB::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_CAB));
		assert(at(0, m.box().exits, STATE_CAA));
		assert(at(0, m.box().inits, STATE_CAB));
		assert(has_box(STATE_TOP));
		m.box().clear();

		TestAccess::setStateHistory<StateC>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCA::isCurrent(m));
		assert(StateCA::alias() != m.currentState());
		assert(StateCAB::isCurrent(m));
		assert(StateCAB::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_C));
		assert(at(1, m.box().entries, STATE_CA));
		assert(at(2, m.box().entries, STATE_CAB));
		assert(at(0, m.box().exits, STATE_CAB));
		assert(at(1, m.box().exits, STATE_CA));
		assert(at(2, m.box().exits, STATE_C));
		assert(at(0, m.box().inits, STATE_CAB));
		assert(has_box(STATE_TOP));
		m.box().clear();

		// Sibling* -> Sibling*
		TestAccess::setState<StateCBB>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCB::isCurrent(m));
		assert(StateCB::alias() != m.currentState());
		assert(StateCBB::isCurrent(m));
		assert(StateCBB::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_CB));
		assert(at(1, m.box().entries, STATE_CBB));
		assert(at(0, m.box().exits, STATE_CAB));
		assert(at(1, m.box().exits, STATE_CA));
		assert(at(0, m.box().inits, STATE_CBB));
		assert(has_box(STATE_TOP));
		m.box().clear();

		// Child* of Sibling -> Sibling
		TestAccess::setState<StateA>(m);
		assert(StateA::isCurrent(m));
		assert(StateA::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_A));
		assert(at(0, m.box().exits, STATE_CBB));
		assert(at(1, m.box().exits, STATE_CB));
		assert(at(2, m.box().exits, STATE_C));
		assert(at(0, m.box().inits, STATE_A));
		assert(has_box(STATE_TOP | STATE_A));
		m.box().clear();

		// Sibling -> Sibling with Deep History
		TestAccess::setStateHistory<StateC>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCB::isCurrent(m));
		assert(StateCB::alias() != m.currentState());
		assert(StateCBB::isCurrent(m));
		assert(StateCBB::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_C));
		assert(at(1, m.box().entries, STATE_CB));
		assert(at(2, m.box().entries, STATE_CBB));
		assert(at(0, m.box().exits, STATE_A));
		assert(at(0, m.box().inits, STATE_CBB));
		assert(has_box(STATE_TOP));
		m.box().clear();

		// Ignoring history
		TestAccess::setState<StateC>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCA::isCurrent(m));
		assert(StateCA::alias() != m.currentState());
		assert(StateCAA::isCurrent(m));
		assert(StateCAA::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_C));
		assert(at(1, m.box().entries, STATE_CA));
		assert(at(2, m.box().entries, STATE_CAA));
		assert(at(0, m.box().exits, STATE_CBB));
		assert(at(1, m.box().exits, STATE_CB));
		assert(at(2, m.box().exits, STATE_C));
		assert(at(0, m.box().inits, STATE_C));
		assert(at(1, m.box().inits, STATE_CA));
		assert(at(2, m.box().inits, STATE_CAA));
		assert(has_box(STATE_TOP | STATE_CAA));
		m.box().clear();

		// Clearing history
		StateB::clearHistory(m);
		TestAccess::setStateHistory<StateB>(m);
		assert(StateB::isCurrent(m));
		assert(StateB::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_B));
		assert(at(0, m.box().exits, STATE_CAA));
		assert(at(1, m.box().exits, STATE_CA));
		assert(at(2, m.box().exits, STATE_C));
		assert(at(0, m.box().inits, STATE_B));
		assert(has_box(STATE_TOP));

		// Clearing History deep
		TestAccess::setState<StateCAB>(m);
		TestAccess::setState<StateB>(m);
		m.box().clear();

		StateC::clearHistoryDeep(m);
		TestAccess::setState<StateC>(m);
		assert(StateC::isCurrent(m));
		assert(StateC::alias() != m.currentState());
		assert(StateCA::isCurrent(m));
		assert(StateCA::alias() != m.currentState());
		assert(StateCAA::isCurrent(m));
		assert(StateCAA::alias() == m.currentState());
		assert(at(0, m.box().entries, STATE_C));
		assert(at(1, m.box().entries, STATE_CA));
		assert(at(2, m.box().entries, STATE_CAA));
		assert(at(0, m.box().exits, STATE_B));
		assert(at(0, m.box().inits, STATE_C));
		assert(at(1, m.box().inits, STATE_CA));
		assert(at(2, m.box().inits, STATE_CAA));
		assert(has_box(STATE_TOP | STATE_CAA));
		m.box().clear();

		// Testing persistent boxes
		TestAccess::setState<StateAAA>(m);
		// Increment data of persistent box: restoring the machine later should bring back old value
		assert(TestAccess::getBox<StateAAA>(m)->data++ == 42);

		// Top box data
		assert(m.box().data == 42);

		m.box().clear();

#ifdef MACHO_SNAPSHOTS
		// Testing snapshots
		if (i == 0) {
			m = s;

			assert(m.box().entries.empty());
			assert(m.box().exits.empty());
			assert(m.box().inits.empty());
			// Test box content after snapshot restore
			assert(TestAccess::getBox<StateCAA>(m)->data == 42);

			boxes = old_boxes;
			m.box().clear();
		}
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Testing dispatch mechanism.
void testDispatch() {
	using namespace Dispatch;

	Macho::Machine<Top> m;

	// Test dispatching of events
	m.dispatch(Event(&Top::event1, 1));
	m.dispatch(Event(&Top::event2, 2, false));
	m.dispatch(Event(&Top::event3, 3, true));

	assert(m.box()[0] == EVENT1); assert(m.box()[1] == EVENT2); assert(m.box()[2] == EVENT3);
	m->clear();

	TestAccess::setState<StateA>(m);
	m->clear();
	// Test if internal dispatch happens after switching to new state
	m.dispatch(Event(&Top::event3, 3, true));
	assert(m.box()[0] == EVENT3); assert(m.box()[1] == STATEB_ENTRY); assert(m.box()[2] == EVENT1);

	m->clear();
	// Test if internal dispatch happens after switching to new state
	m.dispatch(Event(&Top::event3, 3, true));
	assert(m.box()[0] == EVENT3); assert(m.box()[1] == STATEA_ENTRY); assert(m.box()[2] == EVENT1);
}


////////////////////////////////////////////////////////////////////////////////
// Testing alias mechanism.
void testAliases() {
	using namespace Transitions;

	boxes = 0;

	// Test reference handling
	const int & i = 42;

	Macho::Alias state1 = Macho::State<StateCAA>(i);
	Macho::Alias state2 = Macho::State<StateAAB>();

	assert(string("StateCAA") == string(state1.name()));
	assert(string("StateAAB") == string(state2.name()));

	// Test reflection
	assert(StateCAA::isChild(StateCAA::alias()));
	assert(StateCAA::isChild(StateCA::alias()));
	assert(StateCAA::isChild(StateC::alias()));
	assert(!StateCAA::isChild(StateAAA::alias()));
	assert(!StateCAA::isChild(StateAA::alias()));
	assert(!StateCAA::isChild(StateA::alias()));

	assert(Macho::State<StateCAA>().isChild(Macho::Alias(StateCAA::alias())));
	assert(Macho::Alias(StateCAA::alias()).isChild(Macho::State<StateCA>()));
	assert(StateCAA::isChild(StateC::alias()));
	assert(!Macho::State<StateCAA>().isChild(Macho::Alias(StateAAA::alias())));
	assert(!Macho::Alias(StateCAA::alias()).isChild(Macho::State<StateAA>()));
	assert(!StateCAA::isChild(StateA::alias()));

	assert(StateCAA::isParent(StateCAA::alias()));
	assert(StateCA::isParent(StateCAA::alias()));
	assert(StateC::isParent(StateCAA::alias()));
	assert(!StateAAA::isParent(StateCAA::alias()));
	assert(!StateAA::isParent(StateCAA::alias()));
	assert(!StateA::isParent(StateCAA::alias()));

	assert(Macho::State<StateCAA>().isParent(Macho::Alias(StateCAA::alias())));
	assert(Macho::Alias(StateCA::alias()).isParent(Macho::State<StateCAA>()));
	assert(StateC::isParent(StateCAA::alias()));
	assert(!Macho::State<StateAAA>().isParent(Macho::Alias(StateCAA::alias())));
	assert(!Macho::Alias(StateAA::alias()).isParent(Macho::State<StateCAA>()));
	assert(!StateA::isParent(StateCAA::alias()));

	assert(Macho::State<StateA>() == Macho::State<StateA>());
	assert(! (Macho::State<StateA>() == Macho::State<StateB>()) );
	assert(Macho::State<StateA>() != Macho::State<StateB>());

	assert(StateA::alias() == StateA::alias());
	assert(! (StateA::alias() == StateB::alias()) );

	assert(Macho::Alias(StateA::alias()) == StateA::alias());
	assert(Macho::Alias(StateA::alias()) != StateB::alias());

	assert(Macho::State<StateA>() == StateA::alias());
	assert(Macho::State<StateA>() != StateB::alias());
	assert(Macho::State<StateA>() != Macho::Alias(StateB::alias()));

	// Test machine initialization with aliases.
	Macho::Machine<Top> m(state1);

	Macho::Alias history = Macho::StateHistory<StateC>(m);

	assert(history == StateC::alias());
	assert(string("StateC") == history.name());

	assert(StateCAA::alias() == m.currentState());
	assert(state1 == m.currentState());

	assert(at(0, m.box().entries, STATE_TOP));
	assert(at(1, m.box().entries, STATE_C));
	assert(at(2, m.box().entries, STATE_CA));
	assert(at(3, m.box().entries, STATE_CAA));
	assert(at(0, m.box().inits, STATE_CAA));
	assert(has_box(STATE_TOP | STATE_CAA));
	assert(TestAccess::getBox<StateCAA>(m)->data == 42);

	TestAccess::getBox<StateCAA>(m)->data = 0;
	m.box().clear();

	// State history is updated when state is left
	assert(history == StateC::alias());

	// Test state transitions with aliases.
	TestAccess::setState(m, state2);

	assert(StateAAB::alias() == m.currentState());
	assert(state2 == m.currentState());

	assert(at(0, m.box().exits, STATE_CAA));
	assert(at(1, m.box().exits, STATE_CA));
	assert(at(2, m.box().exits, STATE_C));
	assert(at(0, m.box().entries, STATE_A));
	assert(at(1, m.box().entries, STATE_AA));
	assert(at(2, m.box().entries, STATE_AAB));
	assert(at(0, m.box().inits, STATE_AAB));
	assert(has_box(STATE_TOP | STATE_A | STATE_AAB));
	m.box().clear();

	assert(history == StateCAA::alias());
	assert(string("StateCAA") == history.name());

	// Test reuse of state alias.
	TestAccess::setState(m, state1);

	assert(StateCAA::alias() == m.currentState());
	assert(state1 == m.currentState());

	assert(at(0, m.box().exits, STATE_AAB));
	assert(at(1, m.box().exits, STATE_AA));
	assert(at(2, m.box().exits, STATE_A));
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAA));
	assert(at(0, m.box().inits, STATE_CAA));
	assert(has_box(STATE_TOP | STATE_CAA));
	assert(TestAccess::getBox<StateCAA>(m)->data == 42);
	m.box().clear();

	assert(history == StateCAA::alias());

	// Test parametrized state transitions.
	TestAccess::setState<StateX>(m, Macho::State<StateCAB>());

	assert(StateX::alias() == m.currentState());

	assert(at(0, m.box().exits, STATE_CAA));
	assert(at(1, m.box().exits, STATE_CA));
	assert(at(2, m.box().exits, STATE_C));
	assert(at(0, m.box().entries, STATE_X));
	assert(at(0, m.box().inits, STATE_X));
	assert(has_box(STATE_TOP | STATE_X));
	m.box().clear();

	m->event(); // Will go to StateCAB.

	assert(StateCAB::alias() == m.currentState());
	assert(at(0, m.box().exits, STATE_X));
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAB));
	assert(at(0, m.box().inits, STATE_CAB));
	assert(has_box(STATE_TOP));
	m.box().clear();

	// State history is updated when state is left
	assert(history == StateCAA::alias());

	// Test history alias.
	TestAccess::setState<StateX>(m);
	m.box().clear();

	TestAccess::setState(m, history);
	assert(StateCAB::alias() == m.currentState());
	assert(at(0, m.box().exits, STATE_X));
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAB));
	assert(at(0, m.box().inits, STATE_CAB));
	assert(has_box(STATE_TOP));
	m.box().clear();

	assert(history == StateCAB::alias());
	assert(string("StateCAB") == history.name());

	// Test history alias.
	TestAccess::setState<StateX>(m);
	m.box().clear();

	TestAccess::setState(m, Macho::State<StateC>(true));
	assert(StateCAB::alias() == m.currentState());
	assert(at(0, m.box().exits, STATE_X));
	assert(at(0, m.box().entries, STATE_C));
	assert(at(1, m.box().entries, STATE_CA));
	assert(at(2, m.box().entries, STATE_CAB));
	assert(at(0, m.box().inits, STATE_C));
	assert(at(1, m.box().inits, STATE_CAB));
	assert(has_box(STATE_TOP));
	m.box().clear();

	assert(history == StateCAB::alias());

	// Testing multiple parameters to state init method.
	TestAccess::setState(m, Macho::State<StateC>(1, 2, 3, 4));
	assert(StateC::alias() == m.currentState());
}


////////////////////////////////////////////////////////////////////////////////
// Testing template states.
void testTemplates() {
	using namespace Templates;

	Macho::Machine<Top> m;

	assert(TState1<StateA>::alias() == m.currentState());

	m->event1();
	assert(TState1<StateB>::alias() == m.currentState());

	m->event1();
	assert(TState1<StateA>::alias() == m.currentState());

	m->event2();
	assert(TState2<StateB>::alias() == m.currentState());

	m->event2();
	assert(TState1<StateA>::alias() == m.currentState());

	TestAccess::setStateHistory<StateB>(m);
	assert(TState2<StateB>::alias() == m.currentState());

	TestAccess::setState<StateB>(m);
	assert(StateB::alias() == m.currentState());

	TestAccess::setStateHistory<TTop<StateB> >(m);
	assert(TState2<StateB>::alias() == m.currentState());

	TestAccess::setStateHistory<TTop<StateA> >(m);
	assert(TState1<StateA>::alias() == m.currentState());

	typedef TState3<Macho::Anchor<StateA, 0> > No0;
	typedef TState3<Macho::Anchor<StateA, 1> > No1;
	typedef TState2<Macho::Anchor<StateA, 1> > No1b;

	TestAccess::setState<No0>(m);
	assert(No0::alias() == m.currentState());

	m->event1();
	assert(No1::alias() == m.currentState());

	m->event2();
	assert(No0::alias() == m.currentState());

	TestAccess::setState<No1b>(m);
	assert(No1b::alias() == m.currentState());

	m->event1();
	assert(TState1<StateB>::alias() == m.currentState());

	TestAccess::setStateHistory<StateA>(m);
	assert(No1b::alias() == m.currentState());

	TestAccess::setStateHistory<TTop<Macho::Anchor<StateA, 0> > >(m);
	assert(No0::alias() == m.currentState());
}


////////////////////////////////////////////////////////////////////////////////
// Main
int main() {
	cout << "Testing transitions" << endl;
	testTransitions();

	cout << endl << "Testing dispatch mechanism" << endl;
	testDispatch();

	cout << endl << "Testing state aliases" << endl;
	testAliases();

	cout << endl << "Testing template states" << endl;
	testTemplates();

	cout << endl << "-- Test complete ---" << endl;
	return 0;
}
