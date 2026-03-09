#include "catch2/catch_all.hpp"

#include "idGenerators.h"

TEST_CASE("Unique Genny Basics", "[Basic]"){
  auto theGen = uniqueIdGenerator();

  REQUIRE(theGen.getNullId() == proIds::NullUid); // Dumb, but fundamental
  auto ones = theGen.getOnesId().to_string();
  auto checkOnesUid = [](std::string str){
    return (str.find_first_not_of("{}-1") == std::string::npos);
  };
  REQUIRE(checkOnesUid(ones));

}

TEST_CASE("Bit more Uid Generator", "[Basic]"){
  auto theGen = uniqueIdGenerator();

  auto a = theGen.getNextId();
  auto b = theGen.getNextId();
  REQUIRE(a != proIds::NullUid);
  REQUIRE(b != proIds::NullUid);
  REQUIRE(a != b);

}

TEST_CASE("Uid tagging", "[Basic]"){
  auto theGen = uniqueIdGenerator();

  auto taggedUid = theGen.getNextId(proIds::uidTag::sub);
  REQUIRE(taggedUid.isTaggedAs(proIds::uidTag::sub));

  auto untagged = theGen.getNextId();
  REQUIRE(untagged.isTaggedAs(proIds::uidTag::none));
  untagged.tag(proIds::uidTag::oneoff);
  REQUIRE(untagged.isTaggedAs(proIds::uidTag::oneoff));

}

TEST_CASE("Uid string functions", "[Basic]"){
  auto theGen = uniqueIdGenerator();
  std::stringstream ss;
  auto id = theGen.getNextId();
  //To string
  ss<<id;
  std::cout<<ss.str()<<std::endl;

  // Checking only valid characters
  auto checkUid = [](std::string str){
    return (str.find_first_not_of("{}-abcdefghijklmnopqrstuvwxyz0123456789") == std::string::npos);
  };
  REQUIRE(checkUid(ss.str()));

  // Back from string
  auto cp = proIds::Uuid(ss.str());
  REQUIRE(id == cp);
}


TEST_CASE("Uid comparisons", "[Basic]"){
  auto theGen = uniqueIdGenerator();

  auto a = theGen.getNextId();
  auto b = theGen.getNextId();
  //auto c = 

  REQUIRE(a.isEq(a));
  REQUIRE_FALSE(a.isEq(b));

  REQUIRE(a.isExactEq(a));
  REQUIRE_FALSE(a.isExactEq(b));

 }

TEST_CASE("Uid questionable choice", "[Questionable]"){
   auto theGen = uniqueIdGenerator();

  auto null = proIds::NullUid;
  REQUIRE_FALSE( ((bool)null));

}
