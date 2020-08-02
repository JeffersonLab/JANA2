
#include "catch.hpp"

#include <JANA/Compatibility/JStatusBits.h>

TEST_CASE("JStatusBitsTest_Unscoped") {

	enum MyUnscopedEnum { FirstBit, SecondBit, ThirdBit, MyEnumSize };
	JStatusBits<MyUnscopedEnum> sut;

	sut.SetStatusBit(FirstBit, true);
	REQUIRE(sut.GetStatusBit(FirstBit) == true);
	REQUIRE(sut.GetStatusBit(SecondBit) == false);
	REQUIRE(sut.GetStatusBit(ThirdBit) == false);

	sut.SetStatusBit(ThirdBit, true);
	REQUIRE(sut.GetStatusBit(FirstBit) == true);
	REQUIRE(sut.GetStatusBit(SecondBit) == false);
	REQUIRE(sut.GetStatusBit(ThirdBit) == true);

	sut.SetStatusBit(FirstBit, false);
	REQUIRE(sut.GetStatusBit(FirstBit) == false);
	REQUIRE(sut.GetStatusBit(SecondBit) == false);
	REQUIRE(sut.GetStatusBit(ThirdBit) == true);

	sut.ClearStatusBit(ThirdBit);
	REQUIRE(sut.GetStatusBit(FirstBit) == false);
	REQUIRE(sut.GetStatusBit(SecondBit) == false);
	REQUIRE(sut.GetStatusBit(ThirdBit) == false);

	sut.SetStatusBit(FirstBit, true);
	sut.ClearStatus();
	REQUIRE(sut.GetStatusBit(FirstBit) == false);
	REQUIRE(sut.GetStatusBit(SecondBit) == false);
	REQUIRE(sut.GetStatusBit(ThirdBit) == false);
}

TEST_CASE("JStatusBitsTest_Scoped") {

	enum class MyScopedEnum { FirstBit, SecondBit, ThirdBit };
	JStatusBits<MyScopedEnum> sut;

	sut.SetStatusBit(MyScopedEnum::FirstBit, true);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::FirstBit) == true);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::SecondBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::ThirdBit) == false);

	sut.SetStatusBit(MyScopedEnum::ThirdBit, true);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::FirstBit) == true);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::SecondBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::ThirdBit) == true);

	sut.SetStatusBit(MyScopedEnum::FirstBit, false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::FirstBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::SecondBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::ThirdBit) == true);

	sut.ClearStatusBit(MyScopedEnum::ThirdBit);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::FirstBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::SecondBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::ThirdBit) == false);

	sut.SetStatusBit(MyScopedEnum::FirstBit, true);
	sut.ClearStatus();
	REQUIRE(sut.GetStatusBit(MyScopedEnum::FirstBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::SecondBit) == false);
	REQUIRE(sut.GetStatusBit(MyScopedEnum::ThirdBit) == false);
}
