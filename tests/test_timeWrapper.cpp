#include "catch2/catch_all.hpp"

#include "timeWrapper.h"

using tw = timeWrapper;

TEST_CASE("Check clock goes forward", "[BasicTime]"){
    // Pretty clumsy...
    auto tp = tw::now();
    auto epoch = tw::referenceTime();
    REQUIRE(tp > epoch);
}

TEST_CASE( "Round trip Seconds", "[BasicTime]" ) {
    long long secs = 1234519;
    REQUIRE(tw::toSeconds(tw::fromSeconds(secs)) == secs);
}

TEST_CASE( "Duration Seconds", "[BasicTime]" ) {
    long long secs = 973;
    REQUIRE(tw::toSeconds(TW_duration{secs}) == secs);
}

TEST_CASE( "Epoch check", "[BasicTime]"){
    auto tp = tw::fromSeconds(0);
    std::string str = tw::formatTime(tp);
    // Should time-zone properly, but here may be GMT or BST so allow both
    REQUIRE( (str == "1970-01-01 01:00:00" || str == "1970-01-01 00:00:00"));
}

// Strings

TEST_CASE( "String format round trip zoned", "[BasicTime]" ) {
    std::string str = "2020-05-03 11:45:13";
    REQUIRE(tw::formatTime(tw::parseTimeZoned(str)) == str);
}
TEST_CASE( "String format round trip", "[BasicTime]" ) {
    std::string str = "2020-05-03 11:45:13";
    // Forming the two possibles because we are too lazy to do timezones
    auto roundtrip = tw::formatTime(tw::parseTime(str));
    bool OK = roundtrip == str;
    if(! OK){
        //try an hour offset
        OK |= roundtrip == "2020-05-03 12:45:13";
    }
    REQUIRE(OK);
}

TEST_CASE("String Format bad case", "[BasicTime]"){
    REQUIRE_THROWS(tw::parseTime("not a time string"));
    REQUIRE_THROWS(tw::parseTimeZoned("not a time string"));
}
TEST_CASE( "Clock format round trip", "[BasicTime]" ) {
    std::string str = "2020-05-03 11:45:13";
    REQUIRE(tw::formatTimeAsClock(tw::parseTimeZoned(str)) == "11:45");
}
TEST_CASE( "String format short date", "[BasicTime]" ) {
    std::string str = "2020-05-03 11:45:13";
    REQUIRE(tw::formatTimeAsShortDate(tw::parseTimeZoned(str)) == "03-05-20");
}

// Time adjustments

TEST_CASE("Previous Midnight", "[AdjustTime]"){
    std::string str = "2023-02-28 23:59:59";
    std::string str2 = "2023-02-28 00:00:00";
    auto day = tw::parseTimeZoned(str);
    REQUIRE(tw::formatTime(tw::midnightBefore(day)) == str2);
}

TEST_CASE("Start of Month", "[AdjustTime]"){
    std::string str = "2023-02-28 00:00:00";
    std::string str2 = "2023-02-01 00:00:00";
    auto day = tw::parseTimeZoned(str);
    REQUIRE(tw::formatTime(tw::startOfMonth(day)) == str2);
}

TEST_CASE("Duration Construction", "[AdjustTime]"){
    auto dur = tw::makeDuration(10, 0, 0);
    REQUIRE(tw::toSeconds(dur) == 600);
}
TEST_CASE("Duration Construction 2", "[AdjustTime]"){
    auto dur = tw::makeDuration(0, 2, 0);
    REQUIRE(tw::toSeconds(dur) == 60*60*2);
}
TEST_CASE("Duration Construction 3", "[AdjustTime]"){
    auto dur = tw::makeDuration(0, 0, 5);
    REQUIRE(tw::toSeconds(dur) == 60*60*5*24);
}

TEST_CASE("Duration Offset", "[AdjustTime]"){
  tw::timePoint t = tw::fromSeconds(34783);
  auto t_plus = tw::addDuration(t, 20, 0, 0);
  REQUIRE( (tw::toSeconds(t_plus) - tw::toSeconds(t)) == tw::toSeconds(tw::makeDuration(20, 0, 0)) );
}

TEST_CASE("Duration Difference", "[AdjustTime]"){
    std::string str = "2023-02-23 00:00:11";
    std::string str2 = "2023-02-22 23:59:00";
    tw::timePoint day = tw::parseTimeZoned(str);
    tw::timePoint day2 = tw::parseTimeZoned(str2);
    REQUIRE(tw::toSeconds(tw::getDifference(day2, day)) == 71);
}

