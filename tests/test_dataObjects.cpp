#include "catch2/catch_all.hpp"

#include "dataObjects.h"
#include "idGenerators.h"
#include <sstream>

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
  // Don't check details, just contains
  timeStamp ts;
  std::stringstream ss, ss_tmp;
  ts.time = 1175;
  ts.projectUid = proIds::NullUid;
  ss<<ts;
  ss_tmp<<ts.time;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<ts.projectUid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}

TEST_CASE("Project and Sub", "[Stream]"){
  projectData pd;
  std::stringstream ss;
  float fte = 0.7;
  pd.name = "Wibble";
  pd.FTE = fte;
  ss<<pd;
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find("70%") != std::string::npos);

  subprojectData sd;
  float frac = 0.1;
  sd.name = "SubWibble";
  sd.frac = frac;
  ss.str("");
  ss<<sd;
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  REQUIRE(ss.str().find("10%") != std::string::npos);

  oneOffProjectData oo;
  oo.name = "OnceWibble";
  oo.description = "A project for wibbling";
  ss.str("");
  ss<<oo;
  REQUIRE(ss.str().find(oo.name) != std::string::npos);
  REQUIRE(ss.str().find(oo.description) != std::string::npos);
 
}

TEST_CASE("Full Project", "[Stream]"){
  projectData pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  fullProjectData fpd(id, pd);
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<fpd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find("0.24") != std::string::npos);

}
TEST_CASE("Full Sub", "[Stream]"){
  subprojectData sd;
  float frac = 0.11;
  sd.name = "Wibble";
  sd.frac = frac;
  
  uniqueIdGenerator theGen;
  auto sid = theGen.getNextId();
  auto pid = theGen.getNextId();
  fullSubProjectData fsd(sid, sd, pid);
  std::stringstream ss, ss_tmp;
  ss_tmp<<sid;
  ss<<fsd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  REQUIRE(ss.str().find("0.11") != std::string::npos);

  ss_tmp.str("");
  ss_tmp<<pid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
}

TEST_CASE("Full One Off", "[Stream]"){
  oneOffProjectData pd;
  pd.name = "Wibble";
  pd.description = "Wobbly project";
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  fullOneOffProjectData fpd(id, pd.name, pd.description);
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<fpd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find(pd.description) != std::string::npos);

}

TEST_CASE("Project Details", "[Stream]"){
  projectDetails pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  pd.subprojectCount = 9;
  pd.assignedSubprojFraction = 0.7;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  pd.uid = id;
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<pd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find("24 %") != std::string::npos);
  REQUIRE(ss.str().find("9 subprojects") != std::string::npos);
  REQUIRE(ss.str().find("totalling 70 %") != std::string::npos);
  REQUIRE(ss.str().find("inactive") == std::string::npos);

  pd.active = false;
  ss.str("");
  ss<<pd;
  REQUIRE(ss.str().find("inactive") != std::string::npos);
}
TEST_CASE("SubProject Details", "[Stream]"){
  subprojectDetails sd;
  float frac = 0.27;
  sd.name = "Wibble";
  sd.frac = frac;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  sd.uid = id;
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<sd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  CHECK(ss.str().find("27 %") != std::string::npos);
  REQUIRE(ss.str().find("inactive") == std::string::npos);

  sd.active = false;
  ss.str("");
  ss<<sd;
  REQUIRE(ss.str().find("inactive") != std::string::npos);
}

TEST_CASE("Time Units", "[Stream]"){
  std::string unit = unitToString(timeSummaryUnit::hour);
  REQUIRE(unit == "hours");
  unit = unitToString(timeSummaryUnit::minute);
  REQUIRE(unit == "minutes");
  unit = unitToString(timeSummaryUnit::debug);
  REQUIRE(unit == "units");

  auto div = unitToDivisor(timeSummaryUnit::hour);
  REQUIRE(div == 60*60);
  div = unitToDivisor(timeSummaryUnit::minute);
  REQUIRE(div == 60);
  div = unitToDivisor(timeSummaryUnit::debug);
  REQUIRE(div == 1);
}

TEST_CASE("Time Summary", "[Stream]"){

  std::stringstream ss;
  timeSummaryItem item;
  item.text ="Wibbly Wobbly";
  item.stat = timeSummaryStatus::onTarget;

  ss<<item;
  REQUIRE(ss.str().find(item.text) != std::string::npos);
  REQUIRE(ss.str().find("==") != std::string::npos);
  REQUIRE(ss.str().find("++") == std::string::npos);
  REQUIRE(ss.str().find("--") == std::string::npos);

  ss.str("");
  item.stat = timeSummaryStatus::underTarget;
  ss<<item;
  REQUIRE(ss.str().find("--") != std::string::npos);
  REQUIRE(ss.str().find("++") == std::string::npos);
  REQUIRE(ss.str().find("==") == std::string::npos);

  ss.str("");
  item.stat = timeSummaryStatus::overTarget;
  ss<<item;
  REQUIRE(ss.str().find("++") != std::string::npos);
  REQUIRE(ss.str().find("==") == std::string::npos);
  REQUIRE(ss.str().find("--") == std::string::npos);

}

TEST_CASE("Time Digest Period", "[Stream]"){

  std::stringstream ss, ss_tmp;
  timeDigestPeriod tpd;
  tpd.displayName = "Timey";
  tpd.start = 1234;
  tpd.duration = 789;

  ss<<tpd;
  REQUIRE(ss.str().find(tpd.displayName) != std::string::npos);
  ss_tmp<<tpd.start;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<tpd.duration;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}

TEST_CASE("Time Digest Entry", "[Stream]"){

  uniqueIdGenerator theGen;
  std::stringstream ss, ss_tmp;
  timeDigestEntry td;
  td.projectUid = theGen.getNextId();
  td.duration = 1149;
  td.period = 73;

  ss<<td;
  ss_tmp<<td.projectUid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<td.period;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<td.duration;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}