#ifndef __MACHO_HPP__
#define __MACHO_HPP__

// Macho - C++ Machine Objects
//
// The Machine Objects class library (in short Macho) allows the creation of
// state machines based on the "State" design pattern in straight C++. It
// extends the pattern with the option to create hierarchical state machines,
// making it possible to convert the popular UML statechart notation to working
// code in a straightforward way. Other features are entry and exit actions,
// state histories and state variables.
//
// Copyright (c) 2005 by Eduard Hiti (feedback to macho@ehiti.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// You are encouraged to provide any changes, extensions and corrections for
// this software to the author at the above-mentioned email address for
// inclusion into future versions.
//
//
// Description:
//
// States are represented as C++ classes. The hierarchy of states follows the
// inheritance relation between these state classes. A set of state classes for
// a single state machine derives directly or indirectly from a top state class,
// making it the composite state holding all other states. Events are processed
// by calling virtual methods of the top state class. Substates redefine the
// behaviour of these event handler methods.
//
// Special methods "entry", "exit" and "init" are called on state entry, state
// exit and state initialization of super- and substates (in the order defined
// by statechart semantics and current machine state).
//
// An object of type "Machine" maintains the current state of a state machine
// and dispatches events to it. The "Machine" type is a template class
// parametrized with the top state class of the state machine to be run.
//
// State data is not kept in state classes (because state class instances are
// created just once and then reused, whereas state data should be instantiated
// or destroyed each time its state is entered or left). State data is put in
// "Box" types specific to each state class instead, which are managed by the
// Machine object. Boxes are retrieved by calling the "box" method.
// Superstate boxes are accessible by qualifiying the "box" method with the
// state class name (e.g. TOP::box()).
//
// A history of entered substates can be kept for superstates. With a special
// transition into the superstate the history substate can be reentered. History
// can be shallow (only direct substates) or deep (any substate).
//
//
// Example:
//
// #include "Macho.hpp"
// #include <iostream>
// using namespace std;
//
// namespace Example {
//	TOPSTATE(Top) {
//		struct Box {
//			Box() : data(0) {}
//			long data;
//		};
//
//		STATE(Top)
//
//		virtual void event1() {}
//		virtual void event2() {}
//
//	private:
//		void entry();
//		void exit();
//		void init();
//	};
//
//	SUBSTATE(Super, Top) {
//		STATE(Super)
//		HISTORY()
//
//	private:
//		void entry();
//		void exit();
//	};
//
//	SUBSTATE(StateA, Super) {
//		struct Box {
//			Box() : data(0) {}
//			int data;
//		};
//
//		STATE(StateA)
//
//		void event1();
//
//	 private:
//		void entry();
//		void exit();
//		void init(int i);
//	};
//
//	SUBSTATE(StateB, Super) {
//		STATE(StateB)
//
//		void event2();
//
//	private:
//		void entry();
//		void exit();
//	};
//
//	void Top::entry() { cout << "Top::entry" << endl; }
//	void Top::exit() { cout << "Top::exit" << endl; }
//	void Top::init() { setState<StateA>(42); }
//
//	void Super::entry() { cout << "Super::entry" << endl; }
//	void Super::exit() { cout << "Super::exit" << endl; }
//
//	void StateA::entry() { cout << "StateA::entry" << endl; }
//	void StateA::init(int i) { box().data = i; }
//	void StateA::exit() { cout << "StateA::exit" << endl; }
//	void StateA::event1() { setState<StateB>(); }
//
//	void StateB::entry() { cout << "StateB::entry" << endl; }
//	void StateB::exit() { cout << "StateB::exit" << endl; }
//	void StateB::event2() { setState<StateA>(); }
// }
//
// int main() {
//	Macho::Machine<Example::Top> m;
//	m->event1();
//	m->event2();
//
//	return 0;
// }
//
// Output is:
//
// Top::entry
// Super::entry
// StateA::entry
// StateA::exit
// StateB::entry
// StateB::exit
// StateA::entry
// StateA::exit
// Super::exit
// Top::exit
//
//
// Version History:
//
//	  0.9.7 (released 2007-12-1):
//		 - Introduction of template states
//		 - fixed rare memory leak
//
//	  0.9.6 (released 2007-09-01):
//		 - Changes to state transition semantics (see file "changes_0_9_6.txt")
//		 - New mechanism for state initialization
//		 - Runtime reflection on state relationships now possible
//
//	  0.9.5 (released 2007-05-01):
//		 - Introduction of parametrized state transitions
//
//	  0.9.4 (released 2006-06-01):
//		 - Snapshot functionality added
//
//	  0.9.3 (released 2006-04-20):
//		 - Code reorganization (file Macho.cpp added)
//
//	  0.9.2 (released 2006-04-10):
//		 - Memory leak plugged
//		 - MSVC6 version updated
//
//	  0.9.1 (released 2006-03-30):
//		 - Introduction of persistent boxes
//		 - Speed and size optimizations
//		 - Machine instance can be accessed in event handlers with method "machine"
//
//	  0.9 (released 2006-01-15):
//		 - Introduction of queuable event type
//
//	  0.8.2 (released 2005-12-15):
//		 - Code size reduction by minimizing use of template classes
//
//	  0.8.1 (released 2005-12-01):
//		 - Added MSVC6 variant (see directory "msvc6")
//		 - Added method "clearHistoryDeep"
//
//	  0.8 (released 2005-11-01):
//		 - Initial release
//

#include <new>
#include <cassert>

class TestAccess;


////////////////////////////////////////////////////////////////////////////////
// Various macros for state and history declaration

// Use this macro to define your top state class.
#define TOPSTATE(TOP) \
	struct TOP : public ::Macho::Link< TOP, ::Macho::TopBase< TOP > >

// Use this macro for all other state classes.
#define SUBSTATE(STATE, SUPERSTATE) \
	struct STATE : public ::Macho::Link< STATE, SUPERSTATE >

// Use this macro for template states that receive an anchor as template
// parameter.
#define TSUBSTATE(STATE, SUPERSTATE) \
	struct STATE : public ::Macho::Link< STATE<typename SUPERSTATE::ANCHOR>, typename SUPERSTATE::SELF >


// Use this macro in your class definition to give it state functionality
// (mandatory). If you have a state box declare it BEFORE macro invocation!
#define STATE(S) \
public: \
	typedef S SELF; \
	typedef S ANCHOR; /* Anchor is the first non-template state in the inheritance chain */ \
	/* Constructor and destructor already defined: you can't (and shouldn't) have your own! */ \
	/* For the user a state class "constructor" and "destructor" are its entry and exit method! */ \
	S(::Macho::_StateInstance & instance) : LINK(instance) {} \
	~S() {} \
	static const char * _state_name() { return #S; } \
	/* Get to your Box with this method: */ \
	Box & box() { return *static_cast<Box *>(_box()); } \
	const Box & box() const { return *static_cast<const Box *>(_box()); } \
	friend class ::_VS8_Bug_101615;

// Use this macro in your template class definition to give it state functionality
// (mandatory). If you have a state box declare it BEFORE macro invocation!
#define TSTATE(S) \
	typedef S SELF; \
	typedef typename S::SUPER SUPER; \
	typedef typename S::TOP TOP; \
	typedef typename S::ANCHOR ANCHOR; /* Anchor is the first non-template state in the inheritance chain */ \
	typedef ::Macho::Link<S, SUPER> LINK; \
	S(::Macho::_StateInstance & instance) : LINK(instance) {} \
	~S() {} \
	static const char * _state_name() { return #S; } \
	typename S::Box & box() { return *static_cast<typename S::Box *>(this->_box()); } \
	friend class ::_VS8_Bug_101615; \
	using LINK::dispatch; \
	using LINK::machine; \
	/* must have these methods to quieten gcc */ \
	template<class U> void setState() { LINK::template setState<U>(); } \
	template<class U, class P1> void setState(const P1 & p1) { LINK::template setState<U, P1>(p1); } \
	template<class U, class P1, class P2> void setState(const P1 & p1, const P2 & p2) { LINK::template setState<U, P1, P2>(p1, p2); } \
	template<class U, class P1, class P2, class P3> void setState(const P1 & p1, const P2 & p2, const P3 & p3) { LINK::template setState<U, P1, P2>(p1, p2, p3); } \
	template<class U, class P1, class P2, class P3, class P4> void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4) { LINK::template setState<U, P1, P2>(p1, p2, p3, p4); } \
	template<class U, class P1, class P2, class P3, class P4, class P5> void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5) { LINK::template setState<U, P1, P2>(p1, p2, p3, p4, p5); } \
	template<class U, class P1, class P2, class P3, class P4, class P5, class P6> void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6) { LINK::template setState<U, P1, P2>(p1, p2, p3, p4, p5, p6); } \
	template<class U> void setStateHistory() { LINK::template setStateHistory<U>(); } \
	void setState(const class Alias & state) { LINK::setState(state); }


// Use this macro to select deep history strategy.
#define DEEPHISTORY() \
private: \
	/* If no superstate has history, SUPER::_setHistorySuper is a NOOP */ \
	virtual void _saveHistory(::Macho::_StateInstance & self, ::Macho::_StateInstance & shallow, ::Macho::_StateInstance & deep) \
	{ self.setHistory(&deep); SELF::SUPER::_setHistorySuper(self, deep); } \
protected: \
	/* Substates may use _setHistorySuper to bubble up history */ \
	virtual void _setHistorySuper(::Macho::_StateInstance & self, ::Macho::_StateInstance & deep) \
	{ self.setHistorySuper(deep); } \
public:

// Use this macro to select shallow history strategy.
#define HISTORY() \
private: \
	/* If no superstate has history, SUPER::_setHistorySuper is a NOOP */ \
	virtual void _saveHistory(::Macho::_StateInstance & self, ::Macho::_StateInstance & shallow, ::Macho::_StateInstance & deep) \
	{ self.setHistory(&shallow); SELF::SUPER::_setHistorySuper(self, deep); } \
protected: \
	/* Substates may use _setHistorySuper to bubble up history */ \
	virtual void _setHistorySuper(::Macho::_StateInstance & self, ::Macho::_StateInstance & deep) \
	{ self.setHistorySuper(deep); } \
public:


// Use this macro to have boxes survive state transitions
#define PERSISTENT() \
private: \
	virtual void _deleteBox(::Macho::_StateInstance & instance) {} \
public:


////////////////////////////////////////////////////////////////////////////////
// Everything else is put into namespace 'Macho'.
// Some identifiers are prefixed with an underscore to prevent name clashes with
// deriving classes or to mark things as library internal. Don't touch things
// with an underscore prefix!
namespace Macho {

	class _MachineBase;

	template<class T>
	class Machine;

	template<class T>
	class IEvent;

	class _StateInstance;

	// Unique identifier of states, build from consecutive integers.
	// Use Alias to get to ID.
	typedef unsigned int ID;

	// Key is used to build Alias object (points to _KeyData).
	typedef void * Key;


	////////////////////////////////////////////////////////////////////////////////
	// Metaprogramming tools

	// Check type equality at compile time.
	template<class T, class U>
	struct _SameType {
	};

	template<class T>
	struct _SameType<T, T> {
		typedef bool Check;
	};

	// Remove reference modifier from type.
	template<class R>
	struct DR {
		typedef R T;
	};

	template<class R>
	struct DR<R &> {
		typedef R T;
	};

	template<class R>
	struct DR<const R &> {
		typedef R T;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Superstate for template states: allows multiple numbered instances of the same
	// template state for a single anchor state.
	template<class T, int I>
	struct Anchor : public T {
		typedef Anchor<T, I> SELF;
		typedef SELF ANCHOR;
		enum { NUMBER = I };

	protected:
		Anchor(_StateInstance & instance) : T(instance) {}
	};


	////////////////////////////////////////////////////////////////////////////////
	// Box for states which don't declare own Box class.
	class _EmptyBox {
		_EmptyBox() {}
	public:
		static _EmptyBox theEmptyBox;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Helper functions for box creation
	template<class B>
	void * _createBox(void * & place) {
		if (!place)
			place = ::operator new(sizeof(B));

		new (place) B;

		void * box = place;
		place = 0;

		return box;
	}

	template<class B>
	void _deleteBox(void * & box, void * & place) {
		assert(box);
		assert(!place);

		static_cast<B *>(box)->~B();
		place = box;
		box = 0;
	}

#ifdef MACHO_SNAPSHOTS
	template<class B>
	void * _cloneBox(void * other) {
		assert(other);
		return new B(*static_cast<B *>(other));
	}
#endif

	// Specializations for EmptyBox:
	// EmptyBox object gets reused over and over and never is deleted.
	template<>
	void * _createBox<_EmptyBox>(void * & place);

	template<>
	void _deleteBox<_EmptyBox>(void * & box, void * & place);

#ifdef MACHO_SNAPSHOTS
	template<>
	void * _cloneBox<_EmptyBox>(void * other);
#endif


	////////////////////////////////////////////////////////////////////////////////
	// Essential information pointed at by state key.
	struct _KeyData {
		typedef _StateInstance & (*Generator)(_MachineBase & machine);
		typedef bool (*Predicate)(Key);
		typedef const char * (*NameFn)();

		// Get StateInstance object from key.
		const Generator instanceGenerator;

		// Is state of given key a child state?
		const Predicate childPredicate;

		const NameFn name;
		const ID id;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Base class for all state classes.
	// Also serves as 'Root' state. By entering this state we trigger entry
	// and exit actions of user's top state.
	class _StateSpecification {
	public:
		virtual ~_StateSpecification() {}

		static bool isChild(Key key) {
			return false;
		}

	protected:
		_StateSpecification(_StateInstance & instance)
			: _myStateInstance(instance)
		{}

		// Initiate transition to a new state.
		// Template parameter S is the new state to enter.
		// Transition is performed AFTER control flow returns to the Machine object.
		// Initiating more than one transition is considered an error!
		// The new state may receive parameters for its 'init' methods:
		// setState<StateA>("someData");
		template<class S>
		void setState();

		template<class S, class P1>
		void setState(const P1 & p1);

		template<class S, class P1, class P2>
		void setState(const P1 & p1, const P2 & p2);

		template<class S, class P1, class P2, class P3>
		void setState(const P1 & p1, const P2 & p2, const P3 & p3);

		template<class S, class P1, class P2, class P3, class P4>
		void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4);

		template<class S, class P1, class P2, class P3, class P4, class P5>
		void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5);

		template<class S, class P1, class P2, class P3, class P4, class P5, class P6>
		void setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6);

		// Initiate transition to a state's history.
		// If state has no history, transition is to the state itself.
		template<class S>
		void setStateHistory();

		// Initiate transition to a new state.
		// Parameter 'state' is the new state to enter.
		// See above and class 'Alias' for more information.
		void setState(const class Alias & state);

		// Deprectated!
		template<class S>
		void setStateBox(typename S::Box * box = 0);

		// Deprectated!
		template<class S>
		void setStateDirect(typename S::Box * box = 0);

		// 'Restore from snapshot' event: set current state.
		// Default implementation: Does not trigger entry actions!
		virtual void _restore(_StateInstance & current);

		// only to be used in _restore
		void setState(_StateInstance & current);

		// 'Shutdown machine' event: exit all states.
		// Default implementation: Triggers exit actions!
		// Override empty to omit calling exit actions.
		virtual void _shutdown();

		// This is the method to bubble up history information
		// for states whose superstates have no history (so does nothing).
		virtual void _setHistorySuper(_StateInstance & self, _StateInstance & deep) {}

	private:
		// State exit. Not allowed to initiate state change.
		virtual void exit() {}

		// State entry. Not allowed to initiate state change.
		virtual void entry() {}

		// Special kind of state entry: Upon transition to a new state,
		// entry methods of that state and its superstates are called;
		// 'init' however is called only on the one state the transition
		// actually goes to.
		// Is allowed to change state (to child states).
		virtual void init() {}

	private:
		// C++ needs something like package visibility

		// for _myStateInstance
		template<class T>
		friend class TopBase;

		// for _getInstance
		template<class C, class P>
		friend class Link;

		friend class _StateInstance;
		friend class _RootInstance;

		friend class _MachineBase;

		// Create StateInstance object of state.
		static _StateInstance & _getInstance(_MachineBase & machine);

		virtual void _deleteBox(_StateInstance & instance) {}

		// Default history strategy (no history).
		virtual void _saveHistory(_StateInstance & self, _StateInstance & shallow, _StateInstance & deep) {}

	private:
		_StateInstance & _myStateInstance;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Base class for user defined top state (and indirectly all other states).
	template<class T>
	class TopBase : public _StateSpecification {
	public:
		// This typedef is an alias for user defined top state in all (sub)states.
		typedef T TOP;

	protected:
		TopBase(_StateInstance & instance)
			: _StateSpecification(instance)
		{}

		void dispatch(IEvent<TOP> * event);

		_MachineBase & machine();
	};


	////////////////////////////////////////////////////////////////////////////////
	// This class links substate specifications to superstate specifications by
	// deriving from the superstate and being derived from by the substate.
	// Substates inherit event handlers from superstates for reuse or redefinition
	// this way.
	template<class C, class P>
	class Link : public P {
	public:
		// Alias for superstate.
		typedef P SUPER;

		// Alias for topstate.
		typedef typename P::TOP TOP;

		// Default box type.
		typedef _EmptyBox Box;

		// Key is used to build Alias.
		static Key key();

		// Alias represents state.
		static Alias alias();

		static bool isChild(Key other) {
			return key() == other || SUPER::isChild(other);
		}

		static bool isParent(Key other) {
			return static_cast<_KeyData *>(other)->childPredicate(key());
		}

		// Is machine m in this state?
		static bool isCurrent(const _MachineBase & m);

		// Deprecated!
		// Is machine m in exactly this state?
		static bool isCurrentDirect(const _MachineBase & m);

		static void clearHistory(_MachineBase & m);

		static void clearHistoryDeep(_MachineBase & m);

		static Alias history(const _MachineBase & m);

	protected:
		// Needed to perform compile time checks.
		typedef Link<C, P> LINK;

		Link(_StateInstance & instance);

		// These definitions seem redundant but they are not!
		// They override parent definitions so each substate gets either
		// this default or their own, but never its parents definitions.
		virtual void entry() {}
		virtual void init() {}
		virtual void exit() {}

		// This method keeps '_myStateInstance' attribute private.
		void * _box();
		const void * _box() const;

	private:
		// for _getInstance
		template<class U, class V>
		friend class Link;

		// for _getInstance
		friend class _StateSpecification;

		// for _getInstance
		friend class Machine<TOP>;

		// for _getInstance
		friend class Alias;

		// for Tests
		friend class ::TestAccess;

		// Create StateInstance object of state.
		static _StateInstance & _getInstance(_MachineBase & machine);

		// Box is by default not persistent. Not redundant!
		virtual void _deleteBox(_StateInstance & instance);

		// Default history strategy (no history). Not redundant!
		virtual void _saveHistory(_StateInstance & self, _StateInstance & shallow, _StateInstance & deep) {
			// Bubble up history. If no superstate has history, _setHistorySuper will do nothing.
			this->_setHistorySuper(self, deep);
		}

	private:
		_StateInstance & _myStateInstance;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Unique identifier for state S.
	template<class S>
	class StateID {
	public:
		static const ID value;
	};


	////////////////////////////////////////////////////////////////////////////////
	// StateInstance maintains machine specific data about a state. Keeps history, box
	// and state object for state. StateInstance object is created the first time state
	// is entered. There is at most one StateInstance object per state per machine
	// instance.
	class _StateInstance {
	protected:
		_StateInstance(_MachineBase & machine, _StateInstance * parent);

	public:
		virtual ~_StateInstance();

		// Perform entry actions.
		// 'first' is true on very first call.
		void entry(_StateInstance & previous, bool first = true);

		// Perform exit actions.
		void exit(_StateInstance & next);

		// Perform init action.
		void init(bool history);

		void saveHistory(_StateInstance & shallow, _StateInstance & deep) {
			// Check state's history strategy.
			mySpecification->_saveHistory(*this, shallow, deep);
		}

		// Update superstate's history information:
		void setHistorySuper(_StateInstance & deep) {
			if (myParent)
				// Let it choose between deep or shallow history.
				myParent->saveHistory(*this, deep);
		}

#ifdef MACHO_SNAPSHOTS
		// Copy state of another StateInstance object.
		void copy(_StateInstance & original);

		// Create a clone of StateInstance object for another machine.
		_StateInstance * clone(_MachineBase & newMachine);
#endif

		void shutdown() {
			mySpecification->_shutdown();
		}

		void restore(_StateInstance & instance) {
			mySpecification->_restore(instance);
		}

		virtual ID id() = 0;

		virtual Key key() = 0;

		virtual const char * name() = 0;

		// 'Virtual constructor' needed for cloning.
		virtual _StateInstance * create(_MachineBase & machine, _StateInstance * parent) = 0;

		virtual void createBox() = 0;
		virtual void deleteBox() = 0;
#ifdef MACHO_SNAPSHOTS
		virtual void cloneBox(void * box) = 0;
#endif

		// Only needed for top state (constructor of Machine calls this)
		void setBox(void * box) {
			assert(!myBox);

			if (myBoxPlace) {
				// Free cached memory of previously used box.
				::operator delete(myBoxPlace);
				myBoxPlace = 0;
			}

			myBox = box;
		}

		// Is 'instance' a superstate?
		bool isChild(const _StateInstance & instance) {
			return this == &instance || (myParent && myParent->isChild(instance));
		}

		_StateSpecification & specification() {
			assert(mySpecification);
			return *mySpecification;
		}

		void * box() {
			assert(myBox);
			return myBox;
		}

		_MachineBase & machine() {
			return myMachine;
		}

		// const: History can be manipulated even on a const object.
		void setHistory(_StateInstance * history) const {
			myHistory = history;
		}

		_StateInstance * history() const {
			return myHistory;
		}

	protected:
		_MachineBase & myMachine;
		_StateSpecification * mySpecification;   // Instance of state class
		mutable _StateInstance * myHistory;
		_StateInstance * myParent;
		void * myBox;
		void * myBoxPlace;	// Reused box heap memory
	};


	////////////////////////////////////////////////////////////////////////////////
	// StateInstance for Root state (the real top state).
	class _RootInstance : public _StateInstance {
	protected:
		friend class _StateSpecification;

		_RootInstance(_MachineBase & machine, _StateInstance * parent)
			: _StateInstance(machine, parent)
		{
			mySpecification = new _StateSpecification(*this);
		}

	public:
		virtual ID id() {
			return 0;
		}

		virtual Key key() {
			// Can't happen: key is only called by users, and they don't know about Root.
			assert(false); return 0;
		}

		virtual void createBox() {}
		virtual void deleteBox() {}
#ifdef MACHO_SNAPSHOTS
		virtual void cloneBox(void * box) {}
#endif

		virtual const char * name() { return "Root"; }

		// 'Virtual constructor' needed for cloning.
		virtual _StateInstance * create(_MachineBase & machine, _StateInstance * parent) {
			return new _RootInstance(machine, parent);
		}

	};


	////////////////////////////////////////////////////////////////////////////////
	// StateInstance for substates (including Top ;-)
	// Has methods to create state specific objects.
	template<class S>
	class _SubstateInstance : public _StateInstance {
	protected:
		template<class C, class P>
		friend class Link;

		_SubstateInstance(_MachineBase & machine, _StateInstance * parent)
			: _StateInstance(machine, parent)
		{
			assert(parent);
			this->mySpecification = new S(*this);
		}

	public:
		typedef typename S::Box Box;

		virtual ~_SubstateInstance() {
			if (this->myBox)
				Macho::_deleteBox<Box>(myBox, myBoxPlace);
		}

		virtual const char * name() { return S::_state_name(); }

		virtual ID id() {
			return StateID<S>::value;
		}

		virtual Key key() {
			return S::key();
		}

		// 'Virtual constructor' needed for cloning.
		virtual _StateInstance * create(_MachineBase & machine, _StateInstance * parent) {
			return new _SubstateInstance<S>(machine, parent);
		}

		virtual void createBox() {
			if (!this->myBox)
				this->myBox = Macho::_createBox<Box>(myBoxPlace);
		}

		virtual void deleteBox() {
			assert(myBox);
			Macho::_deleteBox<Box>(myBox, myBoxPlace);
		}

#ifdef MACHO_SNAPSHOTS
		virtual void cloneBox(void * box) {
			assert(!myBox);
			assert(!myBoxPlace);
			// Needs copy constructor in ALL box types.
			myBox = Macho::_cloneBox<Box>(box);
		}
#endif

	};


	////////////////////////////////////////////////////////////////////////////////
	// Definitions for queuable event types

	// Generic interface for event objects (available only to MachineBase)
	class _IEventBase {
	public:
		virtual ~_IEventBase() {}
		virtual void dispatch(_StateInstance &) = 0;
	};


	// Interface for event objects (bound to a top state)
	template<class TOP>
	class IEvent : protected _IEventBase {
		friend class Machine<TOP>;
		friend class TopBase<TOP>;
	};


	// Event with four parameters
	template<class TOP, class R, class P1, class P2, class P3, class P4, class P5, class P6>
	class _Event6 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1, P2, P3, P4, P5, P6);

	public:
		_Event6(Signature handler, const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4, const typename DR<P5>::T & p5, const typename DR<P6>::T & p6)
			: myHandler(handler)
			, myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
			, myParam5(p5)
			, myParam6(p6)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1, myParam2, myParam3, myParam4, myParam5, myParam6);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
		typename DR<P2>::T myParam2;
		typename DR<P3>::T myParam3;
		typename DR<P4>::T myParam4;
		typename DR<P5>::T myParam5;
		typename DR<P6>::T myParam6;
	};


	// Event with four parameters
	template<class TOP, class R, class P1, class P2, class P3, class P4, class P5>
	class _Event5 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1, P2, P3, P4, P5);

	public:
		_Event5(Signature handler, const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4, const typename DR<P5>::T & p5)
			: myHandler(handler)
			, myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
			, myParam5(p5)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1, myParam2, myParam3, myParam4, myParam5);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
		typename DR<P2>::T myParam2;
		typename DR<P3>::T myParam3;
		typename DR<P4>::T myParam4;
		typename DR<P5>::T myParam5;
	};


	// Event with four parameters
	template<class TOP, class R, class P1, class P2, class P3, class P4>
	class _Event4 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1, P2, P3, P4);

	public:
		_Event4(Signature handler, const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4)
			: myHandler(handler)
			, myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1, myParam2, myParam3, myParam4);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
		typename DR<P2>::T myParam2;
		typename DR<P3>::T myParam3;
		typename DR<P4>::T myParam4;
	};


	// Event with three parameters
	template<class TOP, class R, class P1, class P2, class P3>
	class _Event3 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1, P2, P3);

	public:
		_Event3(Signature handler, const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3)
			: myHandler(handler)
			, myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1, myParam2, myParam3);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
		typename DR<P2>::T myParam2;
		typename DR<P3>::T myParam3;
	};


	// Event with two parameters
	template<class TOP, class R, class P1, class P2>
	class _Event2 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1, P2);

	public:
		_Event2(Signature handler, const typename DR<P1>::T & p1, const typename DR<P2>::T & p2)
			: myHandler(handler)
			, myParam1(p1)
			, myParam2(p2)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1, myParam2);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
		typename DR<P2>::T myParam2;
	};


	// Event with one parameter
	template<class TOP, class R, class P1>
	class _Event1 : public IEvent<TOP> {
		typedef R (TOP::*Signature)(P1);

	public:
		_Event1(Signature handler, const typename DR<P1>::T & p1)
			: myHandler(handler)
			, myParam1(p1)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)(myParam1);
		}

		Signature myHandler;
		typename DR<P1>::T myParam1;
	};


	// Event with no parameters
	template<class TOP, class R>
	class _Event0 : public IEvent<TOP> {
		typedef R (TOP::*Signature)();

	public:
		_Event0(Signature handler)
			: myHandler(handler)
		{}

	protected:
		void dispatch(_StateInstance & instance) {
			TOP & behaviour = static_cast<TOP &>(instance.specification());
			(behaviour.*myHandler)();
		}

		Signature myHandler;
	};


	// Event creating functions using type inference
	template<class P1, class P2, class P3, class P4, class P5, class P6, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1, P2, P3, P4, P5, P6), const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4, const typename DR<P5>::T & p5, const typename DR<P6>::T & p6) {
		return new _Event6<TOP, R, P1, P2, P3, P4, P5, P6>(handler, p1, p2, p3, p4, p5, p6);
	}

	// Event creating functions using type inference
	template<class P1, class P2, class P3, class P4, class P5, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1, P2, P3, P4, P5), const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4, const typename DR<P5>::T & p5) {
		return new _Event5<TOP, R, P1, P2, P3, P4, P5>(handler, p1, p2, p3, p4, p5);
	}

	// Event creating functions using type inference
	template<class P1, class P2, class P3, class P4, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1, P2, P3, P4), const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3, const typename DR<P4>::T & p4) {
		return new _Event4<TOP, R, P1, P2, P3, P4>(handler, p1, p2, p3, p4);
	}

	template<class P1, class P2, class P3, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1, P2, P3), const typename DR<P1>::T & p1, const typename DR<P2>::T & p2, const typename DR<P3>::T & p3) {
		return new _Event3<TOP, R, P1, P2, P3>(handler, p1, p2, p3);
	}

	template<class P1, class P2, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1, P2), const typename DR<P1>::T & p1, const typename DR<P2>::T & p2) {
		return new _Event2<TOP, R, P1, P2>(handler, p1, p2);
	}

	template<class P1, class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)(P1), const typename DR<P1>::T & p1) {
		return new _Event1<TOP, R, P1>(handler, p1);
	}

	template<class R, class TOP>
	inline IEvent<TOP> * Event(R (TOP::*handler)()) {
		return new _Event0<TOP, R>(handler);
	}

} // namespace Macho


////////////////////////////////////////////////////////////////////////////////
// MSVC++ 8.0 does not handle qualified member template friends:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=101615
// Otherwise call could happen directly in Initializer classes.
class _VS8_Bug_101615 {
public:
	template<class S, class P1>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1);
	}

	template<class S, class P1, class P2>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1, const P2 & p2) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1, p2);
	}

	template<class S, class P1, class P2, class P3>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1, const P2 & p2, const P3 & p3) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1, p2, p3);
	}

	template<class S, class P1, class P2, class P3, class P4>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1, p2, p3, p4);
	}

	template<class S, class P1, class P2, class P3, class P4, class P5>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1, p2, p3, p4, p5);
	}

	template<class S, class P1, class P2, class P3, class P4, class P5, class P6>
	static inline void execute(Macho::_StateInstance & instance, const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6) {
		S & behaviour = static_cast<S &>(instance.specification());
		behaviour.init(p1, p2, p3, p4, p5, p6);
	}

};


namespace Macho {

	////////////////////////////////////////////////////////////////////////////////
	// Base class for state initializers.
	// Initializer are used to provide parameters to states.
	// The 'execute' method of Initializers will call a state's init method with
	// the data of the initializer object.
	class _Initializer {
	public:
		virtual ~_Initializer() {}

		// Create copy of initializer.
		virtual _Initializer * clone() = 0;

		// Deallocate object.
		virtual void destroy() { delete this; }

		virtual Key adapt(Key key) { return key; }

		// Initialize given state. State is new current state of a state machine.
		virtual void execute(_StateInstance & instance) = 0;
	};


	// Base class for Singleton initializers.
	class _StaticInitializer : public _Initializer {
		// Copy of Singleton is Singleton.
		virtual _Initializer * clone() { return this; }

		// Singletons are never destroyed.
		virtual void destroy() {}
	};


	// Default initializer: provides no parameters, calls state's 'init' method
	// only.
	class _DefaultInitializer : public _StaticInitializer {
	public:
		virtual void execute(_StateInstance & instance) {
			instance.init(false);
		}
	};


	// History initializer: provides no parameters, performs transition to
	// history of state if available.
	class _HistoryInitializer : public _StaticInitializer {
	public:
		virtual void execute(_StateInstance & instance) {
			instance.init(true);
		}
	};


	// Special initializer: Helps alias impersonate as history state of given state.
	class _AdaptingInitializer : public _Initializer {
	public:
		_AdaptingInitializer(const _MachineBase & machine) : myMachine(machine) {}

		virtual void execute(_StateInstance & instance) {
			instance.init(true);
		}

		virtual _Initializer * clone() {
			return new _AdaptingInitializer(myMachine);
		}

		virtual Key adapt(Key key);

	protected:
		const _MachineBase & myMachine;
	};


	// Initializers with one to six parameters.
	template<class S, class P1>
	class _Initializer1 : public _Initializer {
	public:
		_Initializer1(const P1 & p1)
			: myParam1(p1)
		{}

		virtual _Initializer * clone() {
			return new _Initializer1<S, P1>(myParam1);
		}

		virtual void execute(_StateInstance & instance) {
			::_VS8_Bug_101615::execute<S, P1>(instance, myParam1);
		}

		P1 myParam1;
	};


	template<class S, class P1, class P2>
	class _Initializer2 : public _Initializer {
	public:
		_Initializer2(const P1 & p1, const P2 & p2)
			: myParam1(p1)
			, myParam2(p2)
		{}

		virtual _Initializer * clone() {
			return new _Initializer2<S, P1, P2>(myParam1, myParam2);
		}

		void execute(_StateInstance & instance) {
			::_VS8_Bug_101615::execute<S, P1, P2>(instance, myParam1, myParam2);
		}

		P1 myParam1;
		P2 myParam2;
	};


	template<class S, class P1, class P2, class P3>
	class _Initializer3 : public _Initializer {
	public:
		_Initializer3(const P1 & p1, const P2 & p2, const P3 & p3)
			: myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
		{}

		virtual _Initializer * clone() {
			return new _Initializer3<S, P1, P2, P3>(myParam1, myParam2, myParam3);
		}

		void execute(_StateInstance & instance) {
			::_VS8_Bug_101615::execute<S, P1, P2, P3>(instance, myParam1, myParam2, myParam3);
		}

		P1 myParam1;
		P2 myParam2;
		P3 myParam3;
	};


	template<class S, class P1, class P2, class P3, class P4>
	class _Initializer4 : public _Initializer {
	public:
		_Initializer4(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4)
			: myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
		{}

		virtual _Initializer * clone() {
			return new _Initializer4<S, P1, P2, P3, P4>(myParam1, myParam2, myParam3, myParam4);
		}

		void execute(_StateInstance & instance) {
			::_VS8_Bug_101615::execute<S, P1, P2, P3, P4>(instance, myParam1, myParam2, myParam3, myParam4);
		}

		P1 myParam1;
		P2 myParam2;
		P3 myParam3;
		P4 myParam4;
	};


	template<class S, class P1, class P2, class P3, class P4, class P5>
	class _Initializer5 : public _Initializer {
	public:
		_Initializer5(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5)
			: myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
			, myParam5(p5)
		{}

		virtual _Initializer * clone() {
			return new _Initializer5<S, P1, P2, P3, P4, P5>(myParam1, myParam2, myParam3, myParam4, myParam5);
		}

		void execute(_StateInstance & instance) {
			::_VS8_Bug_101615::execute<S, P1, P2, P3, P4, P5>(instance, myParam1, myParam2, myParam3, myParam4, myParam5);
		}

		P1 myParam1;
		P2 myParam2;
		P3 myParam3;
		P4 myParam4;
		P5 myParam5;
	};


	template<class S, class P1, class P2, class P3, class P4, class P5, class P6>
	class _Initializer6 : public _Initializer {
	public:
		_Initializer6(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6)
			: myParam1(p1)
			, myParam2(p2)
			, myParam3(p3)
			, myParam4(p4)
			, myParam5(p5)
			, myParam6(p6)
		{}

		virtual _Initializer * clone() {
			return new _Initializer6<S, P1, P2, P3, P4, P5, P6>(myParam1, myParam2, myParam3, myParam4, myParam5, myParam6);
		}

		void execute(_StateInstance & instance) {
			_VS8_Bug_101615::execute<S, P1, P2, P3, P4, P5, P6>(instance, myParam1, myParam2, myParam3, myParam4, myParam5, myParam6);
		}

		P1 myParam1;
		P2 myParam2;
		P3 myParam3;
		P4 myParam4;
		P5 myParam5;
		P6 myParam6;
	};


	////////////////////////////////////////////////////////////////////////////////
	// Singleton initializers.
	static _DefaultInitializer _theDefaultInitializer;
	static _HistoryInitializer _theHistoryInitializer;


	////////////////////////////////////////////////////////////////////////////////
	// Base class for Machine objects.
	class _MachineBase {
	public:
		class Alias currentState() const;

	protected:
		_MachineBase();
		~_MachineBase();

		// Transition to new state.
		void setState(_StateInstance & instance, _Initializer * init);

		// Transition to new state specified by state alias.
		void setState(const Alias & state);

		// Prepare transition to new state (performed on call to 'rattleOn').
		// There can be only one state transition pending (asserts otherwise)!
		// 'init' is an initializer for the new state.
		void setPendingState(_StateInstance & instance, _Initializer * init) {
			assert( (!myPendingState || (myPendingState == &instance && myPendingInit == init) ) &&
			        "There is already a state transition pending!");

			myPendingState = &instance;
			myPendingInit = init;
		}

		// Provide event object to be executed on current state.
		void setPendingEvent(_IEventBase * event) {
			assert(event);
			assert(!myPendingEvent && "There is already an event pending!");

			myPendingEvent = event;
		}

		// Performs pending state transition.
		void rattleOn();

		// Get StateInstance object for ID.
		_StateInstance * & getInstance(ID id) {
			return myInstances[id];
		}

		// Get StateInstance object for ID.
		const _StateInstance * getInstance(ID id) const {
			return myInstances[id];
		}

		// Starts the machine with the specified start state.
		void start(_StateInstance & instance);
		void start(const Alias & state);

		// Shuts machine down. Will exit any states and free all allocated
		// resources.
		void shutdown();

		// Allocate space for pointers to StateInstance objects.
		void allocate(unsigned int count);

		// Free all StateInstance objects.
		void free(unsigned int count);

		void clearHistoryDeep(unsigned int count, const _StateInstance & instance);

#ifdef MACHO_SNAPSHOTS
		// Create a copy of another machines StateInstance objects (includes boxes).
		void copy(_StateInstance ** other, unsigned int count);

		// Create a copy of another machines StateInstance object.
		_StateInstance * createClone(ID id, _StateInstance * original);
#endif

	protected:
		// C++ needs something like package visibility

		// for getInstance
		template<class C, class P>
		friend class Link;

		// for setPendingEvent
		template<class T>
		friend class TopBase;

		// for getInstance
		friend class _StateSpecification;

		// for getInstance
		friend class Alias;

		// for getInstance
		friend class _AdaptingInitializer;

		// for setPendingState
		friend class _StateInstance;

		// for Tests
		friend class ::TestAccess;

		// Current state of Machine object.
		_StateInstance * myCurrentState;

		// Information about pending state transition.
		_StateInstance * myPendingState;
		_Initializer * myPendingInit;

		// Deprecated!
		void * myPendingBox;

		_IEventBase * myPendingEvent;

		// Array of StateInstance objects.
		_StateInstance ** myInstances;
	};


	////////////////////////////////////////////////////////////////////////////////
	// This is the base class for state aliases. A state alias represents a
	// state of a machine. A transition to that state can be initiated by
	// giving the alias object to the 'setState' method.
	//
	// State aliases are created by the template functions 'State' further below.
	// Parameters can be provided to these functions that will be used to
	// initialize the state on transition to it.
	class Alias {
	public:
		explicit Alias(Key key, bool history = false)
			: myStateKey(key)
			, myInitializer(
					history ?
						static_cast<_Initializer *>(&_theHistoryInitializer)
					:
						static_cast<_Initializer *>(&_theDefaultInitializer)
				)
		{
			assert(key);
		}

		Alias(Key key, _Initializer * init)
			: myStateKey(key)
			, myInitializer(init)
		{
			assert(key);
		}

		Alias(const Alias & other)
			: myStateKey(other.myStateKey)
			, myInitializer(other.myInitializer->clone())
		{}

		Alias & operator=(const Alias & other) {
			if (this == &other) return *this;

			myInitializer->destroy();

			myStateKey = other.myStateKey;
			myInitializer = other.myInitializer->clone();

			return *this;
		}

		~Alias() {
			myInitializer->destroy();
		}

		operator Key() const {
			return key();
		}

		bool isChild(Key k) const {
			return key()->childPredicate(k);
		}

		bool isParent(Key k) const {
			return static_cast<_KeyData *>(k)->childPredicate(key());
		}

		const char * name() const {
			return key()->name();
		}

		ID id() const {
			return key()->id;
		}

	protected:
		friend class _MachineBase;
		friend class _StateSpecification;

		void setState(_MachineBase & machine) const;

		_KeyData * key() const { return static_cast<_KeyData *>(myInitializer->adapt(myStateKey)); }

	protected:
		// Key of specified state.
		Key myStateKey;

		// Initializer of this alias.
		_Initializer * myInitializer;
	};

	// Deprecated: alias for Alias
	typedef Alias StateAlias;


	// Create alias with 0 to 6 parameters.
	template<class S>
	Alias State() {
		return Alias(S::key());
	}

	template<class S, class P1>
	Alias State(const P1 & p1) {
		return Alias(S::key(), new _Initializer1<S, P1>(p1));
	}

	template<class S, class P1, class P2>
	Alias State(const P1 & p1, const P2 & p2) {
		return Alias(S::key(), new _Initializer2<S, P1, P2>(p1, p2));
	}

	template<class S, class P1, class P2, class P3>
	Alias State(const P1 & p1, const P2 & p2, const P3 & p3) {
		return Alias(S::key(), new _Initializer3<S, P1, P2, P3>(p1, p2, p3));
	}

	template<class S, class P1, class P2, class P3, class P4>
	Alias State(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4) {
		return Alias(S::key(), new _Initializer4<S, P1, P2, P3, P4>(p1, p2, p3, p4));
	}

	template<class S, class P1, class P2, class P3, class P4, class P5>
	Alias State(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5) {
		return Alias(S::key(), new _Initializer5<S, P1, P2, P3, P4, P5>(p1, p2, p3, p4, p5));
	}

	template<class S, class P1, class P2, class P3, class P4, class P5, class P6>
	Alias State(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6) {
		return Alias(S::key(), new _Initializer6<S, P1, P2, P3, P4, P5, P6>(p1, p2, p3, p4, p5, p6));
	}

	// Create alias for state's history: not the current history state, but
	// really the history of a state. This means that the alias may point to
	// different states during its life. Needs a machine instance to take history from.
	template<class S>
	Alias StateHistory(const _MachineBase & machine) {
		return Alias(S::key(), new _AdaptingInitializer(machine));
	}


	////////////////////////////////////////////////////////////////////////////////
	// Snapshot of a machine object.
	// Saves the state of a machine object at a specific point in time to be restored
	// later (can be used to achieve something like backtracking).
	// Includes boxes of current state and persistent boxes.
	// Assign a snapshot to a machine (operator=) to restore state.
	// Note that no exit/entry actions of the overwritten machine state are performed!
	// Box destructors however are executed!
#ifdef MACHO_SNAPSHOTS
	template<class TOP>
	class Snapshot : public _MachineBase {
	public:
		Snapshot(Machine<TOP> & machine);

		~Snapshot() {
			free(Machine<TOP>::theStateCount);
		}

	private:
		friend class Machine<TOP>;

		Snapshot(const Snapshot<TOP> & other);
		Snapshot & operator=(const Snapshot<TOP> & other);
	};
#endif


	////////////////////////////////////////////////////////////////////////////////
	// A Machine object maintains a current state.
	// The state can be any substate of template parameter TOP.
	// TOP is the Machine's top most and inital state. TOP must be defined by
	// the macro TOPSTATE. Event processing is done by calling methods (event
	// handlers) on the current state.
	// This is realized by defining an arrow ('->') operator on Machine,
	// forwarding to the interface of TOP.
	// Every possible event handler to be called must therefore appear in the
	// interface of TOP. Events are dispatched by using this operator on a
	// Machine object (e.g. 'machine->event()').
	template<class TOP>
	class Machine : public _MachineBase {
	public:

		// This class performs an action in its destructor after an event
		// handler has finished. Comparable to an After Advice in AOP.
		struct AfterAdvice {
			AfterAdvice(Machine<TOP> & m) : myMachine(m) {}

			// Event handler has finished execution. Execute pending transitions now.
			~AfterAdvice() { myMachine.rattleOn(); }

			// this arrow operator finally dispatches to TOP interface.
			TOP * operator->() {
				return static_cast<TOP *>(& (myMachine.myCurrentState->specification()) );
			}

		private:
			Machine<TOP> & myMachine;
		};

		// State machine instance can be initialized with a top state box.
		Machine(typename TOP::Box * box = 0) {
			// Compile time check: TOP must directly derive from TopBase<TOP>
			typename _SameType<TopBase<TOP>, typename TOP::SUPER>::Check _mustDeriveFromTopBase;
                        (void)_mustDeriveFromTopBase;

			allocate(theStateCount);

			_StateInstance & top = TOP::_getInstance(*this);
			top.setBox(box);

			start(top);
		}

		// Initialize with a state alias object to have machine go to a state
		// other than TOP on startup. Box of top state may also be initialized.
		Machine(const Alias & state, typename TOP::Box * box = 0) {
			// Compile time check: TOP must directly derive from TopBase<TOP>
			typename _SameType<TopBase<TOP>, typename TOP::SUPER>::Check _mustDeriveFromTopBase;
                        (void)_mustDeriveFromTopBase;

			allocate(theStateCount);

			_StateInstance & top = TOP::_getInstance(*this);
			top.setBox(box);

			start(state);
		}

#ifdef MACHO_SNAPSHOTS
		// Create machine from a snapshot.
		Machine(const Snapshot<TOP> & snapshot) {
			allocate(theStateCount);
			copy(snapshot.myInstances, theStateCount);
		}

		// Overwrite current machine state by snapshot.
		Machine<TOP> & operator=(const Snapshot<TOP> & snapshot) {
			assert(!myPendingState);
			assert(!myPendingEvent);

			myCurrentState->shutdown();

			free(theStateCount);
			copy(snapshot.myInstances, theStateCount);

			// Go to Root state first
			myCurrentState = getInstance(0);

			// Then set previous current state
			_StateInstance * current = getInstance(snapshot.myCurrentState->id());
			current->restore(*current);
			rattleOn();

			return *this;
		}
#endif

		~Machine() {
			myCurrentState->shutdown();
			free(theStateCount);
		}

		// Don't return pointer to interface right now: we need to know when the
		// event handler has finished; return an AfterAdvice object instead:
		// it allows us to perform actions after access.
		AfterAdvice operator->() {
			assert(myCurrentState);
			assert(!myPendingState);

			// We need to know when the event handler has finished.
			return AfterAdvice(*this);
		}
		const TOP * operator->() const {
			assert(myCurrentState);
			assert(!myPendingState);

			// Const access so nothing can change
			return static_cast<const TOP *>(& (myCurrentState->specification()) );
		}

		// Dispatch an event object to machine.
		void dispatch(IEvent<TOP> * event, bool destroy = true) {
			assert(event);

			event->dispatch(*myCurrentState);
			if (destroy) delete event;

			rattleOn();
		}

		// Allow (const) access to top state's box (for state data extraction).
		const typename TOP::Box & box() const {
			assert(myCurrentState);
			return static_cast<TOP &>(myCurrentState->specification()).TOP::box();
		}

	private:
		template<class C, class P>
		friend class Link;

	private:
		Machine(const Machine<TOP> & other);
		Machine<TOP> & operator=(const Machine<TOP> & other);

#ifdef MACHO_SNAPSHOTS
		friend class Snapshot<TOP>;
#endif

		template<class T> friend class StateID;

		// Next free identifier for StateInstance objects.
		static ID theStateCount;
	};

	// Root is always there and has ID 0, so start from 1
	template<class TOP>
	ID Machine<TOP>::theStateCount = 1;

	// Each state has a unique ID number.
	// The identifiers are consecutive integers starting from zero,
	// which allows use as index into a vector for fast access.
	// 'Root' always has zero as id.
	template<class S>
	const ID StateID<S>::value = Machine<typename S::TOP>::theStateCount++;


	////////////////////////////////////////////////////////////////////////////////
	// Implementation for StateSpecification

	// Initiate state transition with 0 to six parameters.
	template<class S>
	inline void _StateSpecification::setState() {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, &_theDefaultInitializer);
	}

	template<class S, class P1>
	inline void _StateSpecification::setState(const P1 & p1) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer1<S, P1>(p1));
	}

	template<class S, class P1, class P2>
	inline void _StateSpecification::setState(const P1 & p1, const P2 & p2) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer2<S, P1, P2>(p1, p2));
	}

	template<class S, class P1, class P2, class P3>
	inline void _StateSpecification::setState(const P1 & p1, const P2 & p2, const P3 & p3) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer3<S, P1, P2, P3>(p1, p2, p3));
	}

	template<class S, class P1, class P2, class P3, class P4>
	inline void _StateSpecification::setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer4<S, P1, P2, P3, P4>(p1, p2, p3, p4));
	}

	template<class S, class P1, class P2, class P3, class P4, class P5>
	inline void _StateSpecification::setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer5<S, P1, P2, P3, P4, P5>(p1, p2, p3, p4, p5));
	}

	template<class S, class P1, class P2, class P3, class P4, class P5, class P6>
	inline void _StateSpecification::setState(const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5, const P6 & p6) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, new _Initializer6<S, P1, P2, P3, P4, P5, P6>(p1, p2, p3, p4, p5, p6));
	}

	// Initiate state transition to a state's history.
	template<class S>
	inline void _StateSpecification::setStateHistory() {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.setPendingState(instance, &_theHistoryInitializer);
	}

	// Deprecated!
	template<class S>
	inline void _StateSpecification::setStateBox(typename S::Box * box) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.myPendingBox = box;
		m.setPendingState(instance, &_theHistoryInitializer);
	}

	// Deprecated!
	template<class S>
	inline void _StateSpecification::setStateDirect(typename S::Box * box) {
		_MachineBase & m = _myStateInstance.machine();
		_StateInstance & instance = S::_getInstance(m);
		m.myPendingBox = box;
		m.setPendingState(instance, &_theDefaultInitializer);
	}


	////////////////////////////////////////////////////////////////////////////////
	// Implementation for TopBase
	template<class T>
	inline void TopBase<T>::dispatch(IEvent<TOP> * event) {
		assert(event);
		_myStateInstance.machine().setPendingEvent(event);
	}

	template<class T>
	// Returns current state machine instance.
	inline _MachineBase & TopBase<T>::machine() {
		return this->_myStateInstance.machine();
	}


	////////////////////////////////////////////////////////////////////////////////
	// Implementation for Link
	template<class C, class P>
	inline Link<C, P>::Link(_StateInstance & instance)
		: P(P::_getInstance(instance.machine()))
		// Can't initialize _myStateInstance with _getInstance,
		// because this would result in an endless loop (at least for first call)
		, _myStateInstance(instance)
	{}

	// This method keeps '_myStateInstance' attribute private.
	template<class C, class P>
	inline void * Link<C, P>::_box() {
		return _myStateInstance.box();
	}
	template<class C, class P>
	inline const void * Link<C, P>::_box() const {
		return _myStateInstance.box();
	}

	// Default behaviour: free box on exit.
	template<class C, class P>
	inline void Link<C, P>::_deleteBox(_StateInstance & instance) {
		instance.deleteBox();
	}

	// Create StateInstance object of state.
	template<class C, class P>
	/* static */ inline _StateInstance & Link<C, P>::_getInstance(_MachineBase & machine) {
		// Look first in machine for existing StateInstance.
		_StateInstance * & instance = machine.getInstance(StateID<C>::value);
		if (!instance)
			// Will create parent StateInstance object if not already created.
			instance = new _SubstateInstance<C>(machine, &P::_getInstance(machine));

		return *instance;
	}

	template<class C, class P>
	/* static */ inline bool Link<C, P>::isCurrent(const _MachineBase & machine) {
		return machine.currentState().isChild(key());
	}

	// Deprecated!
	template<class C, class P>
	/* static */ inline bool Link<C, P>::isCurrentDirect(const _MachineBase & machine) {
		return key() == machine.currentState();
	}

	template<class C, class P>
	/* static */ void Link<C, P>::clearHistory(_MachineBase & machine) {
		const _StateInstance * instance = machine.getInstance(StateID<C>::value);
		if (instance)
			instance->setHistory(0);
	}

	template<class C, class P>
	/* static */ void Link<C, P>::clearHistoryDeep(_MachineBase & machine) {
		const _StateInstance * instance = machine.getInstance(StateID<C>::value);
		if (instance)
			machine.clearHistoryDeep(Machine<TOP>::theStateCount, *instance);
	}

	template<class C, class P>
	/* static */ Alias Link<C, P>::history(const _MachineBase & machine) {
		const _StateInstance * instance = machine.getInstance(StateID<C>::value);
		_StateInstance * history = 0;

		if (instance)
			history = instance->history();

		return Alias(history ? history->key() : key());
	}

	template<class C, class P>
	/* static */ inline Key Link<C, P>::key() {
		static _KeyData k = { _getInstance, isChild, C::_state_name, StateID<C>::value };
		return &k;
	}

	template<class C, class P>
	/* static */ inline Alias Link<C, P>::alias() {
		return Alias(key());
	}


	////////////////////////////////////////////////////////////////////////////////
	// Implementation for Snapshot
#ifdef MACHO_SNAPSHOTS
	template<class TOP>
	Snapshot<TOP>::Snapshot(Machine<TOP> & machine) {
		assert(!machine.myPendingState);
		assert(!machine.myPendingEvent);
		assert(machine.myCurrentState);

		allocate(Machine<TOP>::theStateCount);
		copy(machine.myInstances, Machine<TOP>::theStateCount);

		myCurrentState = getInstance(machine.myCurrentState->id());
	}
#endif

} // namespace Macho


#endif // __MACHO_HPP__
