#include "catch2/catch_all.hpp"

#include "timeWrapper.h"

TEST_CASE( "Round trip Seconds", "[Basic]" ) {
    long long secs = 1000000;
    REQUIRE(timeWrapper::toSeconds(timeWrapper::fromSeconds(secs)) == secs);

}

