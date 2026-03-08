#include "catch2/catch_all.hpp"

#include "support.h"

TEST_CASE("Float formatting", "[Support]"){

    REQUIRE(displayFloat(17.3, 1) == "17.3");
    REQUIRE(displayFloat(0.782, 2) == "0.78");
    REQUIRE(displayFloat(-1.23, 1) == "-1.2");
    REQUIRE(displayFloat(-45.0, 2) == "-45.00");
}
TEST_CASE("Float formatting 0.5", "[Support]"){

    REQUIRE(displayFloatHalves(17.3) == "17.5");
    REQUIRE(displayFloatHalves(0.782) == "1.0");
    REQUIRE(displayFloatHalves(-1.23) == "-1.0");
    REQUIRE(displayFloatHalves(-45.0) == "-45.0");
    REQUIRE(displayFloatHalves(-4.56) == "-4.5");
}
TEST_CASE("Float formatting 0.25", "[Support]"){

    REQUIRE(displayFloatQuarters(17.3) == "17.25");
    REQUIRE(displayFloatQuarters(0.782) == "0.75");
    REQUIRE(displayFloatQuarters(-1.23) == "-1.25");
    REQUIRE(displayFloatQuarters(-45.0) == "-45.0");
    REQUIRE(displayFloatQuarters(-4.56) == "-4.5");
}
