//
//  support.h
//  
//
//  Created by Heather Ratcliffe on 28/03/2017.
//
//

#ifndef _support_h
#define _support_h


#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

const std::string appVersion = "0.2.0";
const std::string appName = "Time Tracker Two";

/** \brief Time constants
*
* Factors for quick conversions from seconds etc. For important time values use timeWrapper
*/
namespace timeFactors{
  const int day = 60*60*24;
  const int hour = 60*60;/**< \brief Seconds to hours */
  const int minute = 60;/**< \brief Seconds to minutes */
}

//This should really be a class-type ENUM
namespace dateLimits{
  const int earliest = -1;
  const int latest = 1;
}

enum class dataBackendType{
  flatfile, /**< \brief Flat file data backend */
  database /**< \brief Database data backend */
};

struct appConfig{
  std::string dataFileName = "";
  dataBackendType backend = dataBackendType::database; /**< \brief Type of data backend to use */
};

inline std::string displayFloat(float value, int dp=2){
  //Create a string from given float with exactly dp decimal places
  // Simple, but nice to have inline
  std::stringstream ss;
  ss<<std::fixed<<std::setprecision(dp)<<value;
  return ss.str();
}
inline std::string displayFloatHalves(float value){
  //Create string for given float to nearest 0.5
  return displayFloat(std::floor(value * 2 + 0.5)/2.0, 1);
}
inline std::string displayFloatQuarters(float value){
  //Create string for given float to nearest 0.5
  float flt = std::floor(value * 4 + 0.5);
  float rem = std::remainder(flt, 4);
  return displayFloat(flt/4.0, (rem == 1 or rem == 3) ? 2: 1);
}

// Functor to wrap pointer-to-member -
// Getting parse errors from QT MOC creation so this wrapper
// leaves a normal signature for a signal taking a 
// 'pointer-to-member-function' 
//Any return type and any number of arguments
// Suggest using makeCallback which does type-inference so can be used like:
// auto callbk = makeCallback(&myClass::myFunction);
template<typename T, typename ret, typename... Args>
struct callbackWrapper{
  ret (T::*fn)(Args...);
  ret operator()(T* that, Args... args){return (that->*fn)(std::forward<Args>(args)...);};
  callbackWrapper(ret (T::*fn_in)(Args...)):fn(fn_in){;};
};
template<typename T, typename ret, typename... Args>
auto makeCallback(ret (T::*fn_in)(Args... args)){
  return callbackWrapper<T, ret, Args...>(fn_in);
};

#endif
