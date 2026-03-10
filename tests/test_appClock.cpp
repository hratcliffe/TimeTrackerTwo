#include "catch2/catch_all.hpp"

#include "timeWrapper.h"
#include "appClock.h"

//App clock suncs when ticked, but otherwise does NOT advance

TEST_CASE("App clock start", "[Basic]"){
  auto clk = appClock();
  auto now = clk.now();
  // Now is definitely later than 2000...
  auto refTime = timeWrapper::toSeconds(timeWrapper::parseTime("2000-01-01 00:00:00"));
  REQUIRE(now > refTime);
  REQUIRE_FALSE(clk.travelling());
}

TEST_CASE("App clock tick", "[Basic]"){
  auto clk = appClock();
  auto now = clk.now();
  clk.tick();
  auto now2 = clk.now();
  REQUIRE(now2 >= now);
}
TEST_CASE("App clock tick forced", "[Long]"){
  auto clk = appClock();
  auto now = clk.now();

  //Wait one second
  auto st = timeWrapper::toSeconds(timeWrapper::now());
  auto end = timeWrapper::toSeconds(timeWrapper::now());
  for(;end == st;){
    end = timeWrapper::toSeconds(timeWrapper::now());
  }
  clk.tick();
  auto now2 = clk.now();
  REQUIRE(now2 > now);
}

TEST_CASE("Time travel basic", "[Basic]"){
    auto clk = appClock();
    auto st = clk.now();
    clk.travelBy(timeWrapper::makeDuration(-10,0,0)); // Ten minutes ago
    clk.tick();
    auto end = clk.now();
    REQUIRE( std::abs((st - end) - 600) <= 1); // Allow one second of slip

}

TEST_CASE("Time travel restore", "[Basic]"){
  auto clk = appClock();

  clk.travelBy(-100);
  clk.travelBy(100);
  REQUIRE_FALSE(clk.travelling());

}

TEST_CASE("Clock strings", "[Basic]"){
    auto clk = appClock();
    clk.travelTo(timeWrapper::parseTimeZoned("2000-01-01 11:23:01"));
    clk.tick();
    // Until clock ticks again this is exact time
    auto str = clk.fullTimeString();
    REQUIRE(str =="2000-01-01 11:23:01");
    str = clk.shortTimeString();
    REQUIRE(str == "11:23");
}