#include "unit_006.hpp"

namespace Unit_006 {

	Test::Test() : _value(Test::A)
	{
	}

	void Test::set(Test::Enum v) { _value = v; }
	Test::Enum Test::get() const { return _value; }
	bool Test::equal(Test::Enum v) const { return v == _value; }



	void wrapper()
	{
		SLB_DEBUG_CALL;
		SLB_DEBUG(1, "--> Loading unit_006 wrapper -->");
		
		SLB::Class< Test >("Unit_006::Test")
			.constructor()
			.set("set", &Test::set)
			.set("get", &Test::get)
			.set("equal", &Test::equal)
		;

		SLB::Class< Test::Enum >("Unit_006::Test::Enum")
			.set("A", Test::A)
			.set("B", Test::B)
			.set("C", Test::C)
		;

		SLB_DEBUG(1, "<-- Loading unit_006 wrapper <--");
	}


	SLB_ON_LOAD(wrapper)

}
