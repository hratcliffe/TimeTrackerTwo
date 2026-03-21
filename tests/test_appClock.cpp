#include "catch2/catch_all.hpp"

#include "timeWrapper.h"
#include "appClock.h"

//App clock syncs when ticked, but otherwise does NOT advance

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
    REQUIRE_FALSE(clk.travelling());
    clk.travelBy(timeWrapper::makeDuration(-10,0,0)); // Ten minutes ago
    clk.tick();
    auto end = clk.now();
    REQUIRE( std::abs((st - end) - 600) <= 1); // Allow one second of slip
    REQUIRE(clk.travelling());

}

TEST_CASE("Time travel restore", "[Basic]"){
  auto clk = appClock();
  clk.travelBy(-100);
  REQUIRE(clk.travelling());
  clk.restoreToNow();
  REQUIRE_FALSE(clk.travelling());
}

TEST_CASE("Travelling to now", "[Long]"){
  auto clk = appClock();
  clk.travelBy(-1);
  auto now = timeWrapper::now();
  //If we do this part dead on a seconds-boundary it fails.
  // Repeating it twice is much _less likely_ to fail twice
  // Perhaps there is a genuinely better way?
  bool failed = true;
  for(int i =0; i<2; i++){
    clk.travelTo(now);
    failed &= clk.travelling();
  }
  REQUIRE_FALSE(failed);
  auto st = timeWrapper::toSeconds(timeWrapper::now());
  auto end = timeWrapper::toSeconds(timeWrapper::now());
  for(;end == st;){
    end = timeWrapper::toSeconds(timeWrapper::now());
  }
  clk.travelTo(now);
  REQUIRE_FALSE(clk.travelling()); // Will fail in case we pass a second between these lines of code...
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