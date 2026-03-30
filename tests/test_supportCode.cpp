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

TEST_CASE("Bounded float create", "[Support]"){

  SECTION("Small"){
    eb_float value{100};
    REQUIRE( std::abs((float)value - 0.01) < 1e-5);
  }
  SECTION("Max"){
    eb_float value{10000};
    REQUIRE( std::abs((float)value - 1.0) < 1e-5);
  }
  SECTION("Mid"){
    eb_float value{4731};
    REQUIRE( std::abs((float)value - 0.4731) < 1e-5);
  }
  SECTION("From double"){
    eb_float value{0.125};//Exactly rep.
    REQUIRE(value.value == 1250);
  }
}
TEST_CASE("Bounded float set", "[Support]"){

  eb_float value;
  SECTION("Small"){
    value.set(100);// 1% or 0.01
    REQUIRE( std::abs((float)value - 0.01) < 1e-5);
  }
  SECTION("Max"){
    value.set(10000);
    REQUIRE( std::abs((float)value - 1.0) < 1e-5);
  }
  SECTION("Mid range"){
    value.set(5320);
    REQUIRE( std::abs((float)value - 0.532) < 1e-5);
  }
  SECTION("Zero"){
    value.set(0);
    REQUIRE( std::abs((float)value - 0.0) < 1e-5);
  }
}
TEST_CASE("Bounded float arithmetic", "[Support]"){
  eb_float a{1001}, b{3003}, c{4004}, d{2002};
  eb_float pl = a+b;
  REQUIRE(pl == c);
  eb_float min = b-a;
  REQUIRE(min == d);

  a+=a;
  REQUIRE(a == d);
  c -=d;
  REQUIRE(c == d);
}
TEST_CASE("Bounded float comparators", "[Support]"){
  eb_float a{1000}, b{3000};
  float ea=0.1, eb=0.3;

  SECTION("Equals With same type"){
    REQUIRE(a != b);
    REQUIRE(a == a);
    REQUIRE_FALSE(a == b);
  }
  SECTION("Ordering with same type: <"){
    REQUIRE(a < b);
    REQUIRE(a <= a);
    REQUIRE(a <= b);
    REQUIRE_FALSE(b < a);
    REQUIRE_FALSE(b <= a);
  }
  SECTION("Ordering with same type: >"){
    REQUIRE(b > a);
    REQUIRE(a >= a);
    REQUIRE(b >= a);
    REQUIRE_FALSE(a > b);
    REQUIRE_FALSE(a >= b);
  }
  SECTION("Equals with float: >"){
    REQUIRE(a == ea);
    REQUIRE(b == eb);
  }


}
TEST_CASE("Bounded float stream", "[Support]"){
  eb_float val;
  std::stringstream ss;
  SECTION("Midrange"){
    val.set(7249);
    ss<< val;
    REQUIRE(ss.str() == "0.7249");
  }
  SECTION("Midrange - trailing 0"){
    val.set(7240);
    ss<<val;
    REQUIRE(ss.str() == "0.724");
  }
  SECTION("Small"){
    val.set(15);
    ss<<val;
    REQUIRE(ss.str() == "0.0015");
  }
  SECTION("Tiny"){
    val.set(1);
    ss<<val;
    REQUIRE(ss.str() == "0.0001");
  }
}
TEST_CASE("Bounded float display", "[Support]"){
  eb_float val;
  SECTION("Midrange"){
    val.set(7249);
    auto str = exactPercent(val);
    REQUIRE(str == "72.49");
    str = integerPercent(val);
    REQUIRE(str == "72");
  }
  SECTION("Midrange - trailing 0"){
    val.set(7240);
    auto str = exactPercent(val);
    REQUIRE(str == "72.40");
    str = integerPercent(val);
    REQUIRE(str == "72");
  }
  SECTION("Small"){
    val.set(15);
    auto str = exactPercent(val);
    REQUIRE(str == "0.15");
    str = integerPercent(val);
    REQUIRE(str == "0");
  }
  SECTION("Tiny"){
    val.set(1);
    auto str = exactPercent(val);
    REQUIRE(str == "0.01");
    str = integerPercent(val);
    REQUIRE(str == "0");
  }
  SECTION("Zero"){
    val.set(0);
    auto str = exactPercent(val);
    REQUIRE(str == "0.00");
    str = integerPercent(val);
    REQUIRE(str == "0");
 }
  SECTION("Max"){
    val.set(10000);
    auto str = exactPercent(val);
    REQUIRE(str == "100.00");
    str = integerPercent(val);
    REQUIRE(str == "100");
  }
}
TEST_CASE("Bounded float out of bounds", "[Support]"){
  eb_float value;
  SECTION("Setters"){
    REQUIRE_THROWS( value.set(-1));
    REQUIRE_THROWS( value.set(10001));
    REQUIRE_THROWS( value.set(50001));
  }
  SECTION("Bad construction"){
    auto fn =[](int i){return eb_float{i};};
    REQUIRE_THROWS(fn(-1));
    REQUIRE_THROWS(fn(10001));
    REQUIRE_THROWS(fn(500001));
  }
}
TEST_CASE("Bounded float set and compare float", "[Support]"){
  eb_float value;
  SECTION("Small"){
    value.set(0.01);// 1% or 0.01
    std::cout<<(float) value<<std::endl;
    REQUIRE(value == 0.01);
  }
  SECTION("Max"){
    value.set(1.0);
    REQUIRE(value == 1.0);
  }
  SECTION("Mid range"){
    value.set(0.532);
    REQUIRE(value == 0.532);
  }
  SECTION("Zero"){
    value.set(0.0);
    REQUIRE( value == 0.0);
  }
}
TEST_CASE("Bounded float special functions - one minus", "[Support]"){
  SECTION("Zero"){
    eb_float val{0};
    REQUIRE(oneMinus(val) == eb_float{10000});
  }
  SECTION("Max"){
    eb_float val{10000};
    REQUIRE(oneMinus(val) == eb_float{0});
  }
  SECTION("Mid range"){
    eb_float val{3467};
    REQUIRE(oneMinus(val) == eb_float{6533});
  }
  SECTION("Mid range, another"){
    eb_float val{5600};
    REQUIRE(oneMinus(val) == eb_float{4400});
  }
}