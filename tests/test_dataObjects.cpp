#include "catch2/catch_all.hpp"

#include "dataObjects.h"

// Mostly stream operators....

TEST_CASE("TimeStamp Comparators", "[Basic]"){

  auto theGen = uniqueIdGenerator();
  timeStamp ts, ts2, ts3;
  ts.time = 1275;
  ts2.time = 3359;
  ts3.time = 1275;
  ts.projectUid = theGen.getNextId();
  ts2.projectUid = theGen.getNextId();
  ts3.projectUid = theGen.getNextId();

  REQUIRE(ts2 > ts);
  REQUIRE(ts < ts2);
  REQUIRE(ts != ts2);
  REQUIRE_FALSE(ts3 == ts); // Unmatched IDS
  REQUIRE_FALSE(ts == ts2);
  REQUIRE(ts <= ts2);
  REQUIRE(ts <= ts3);
  REQUIRE(ts2 >= ts);
  REQUIRE(ts3 >= ts);

  ts3.projectUid = ts.projectUid;
  REQUIRE(ts == ts3);
}

TEST_CASE("TimeStamp Comparators to code", "[Basic]"){

  auto theGen = uniqueIdGenerator();
  timeStamp ts;
  ts.time = 1275;
  ts.projectUid = proIds::NullUid;

  REQUIRE(ts == 1275);
  REQUIRE(ts > 1200);
  REQUIRE(ts < 1500);
  REQUIRE_FALSE(ts == 1200);
  REQUIRE(ts >= 1275);
  REQUIRE(ts >= 1200);
  REQUIRE(ts <= 1500);
  REQUIRE(ts <= 1275);
  REQUIRE(ts != 0);
  REQUIRE_FALSE(ts != 1275);
}

TEST_CASE("TimeStamp Stream", "[Stream]"){

}