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
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

// Check given string is valid as a name - currently not blank nor all whitespace
inline bool isValidNameString(std::string name){
  return name.find_first_not_of("\t ") != std::string::npos;
};

using timecode = long long; /**< \brief Type for timecodes, representing seconds since epoch. SIGNED to allow -1 for sentinel below*/

inline const timecode timecodeNull = -1; /**< \brief Sentinel for null time * * Need a sentinel - do not rely on this value, use the named constant */

inline std::string timecode_as_string(timecode tt){
  std::stringstream ss;
  ss<< tt;
  return ss.str();
}

// Definitions to make call-sites clearer
const bool FORCE=true;
const bool NO_FORCE = false;

const std::string appVersion = "0.2.0";
const std::string appName = "Time Tracker Two";

// Exact float.
struct exactBoundedFloat{
  static constexpr int permyriad = 10000; // Normalisation constant
  static constexpr double halfinc = 0.5; // Float corresponding to half a tick
  static constexpr int fromPercent = 100; //Factor to get value from a percent
  static constexpr int toPercent = 100; // Factor to turn value into percent
  int value = 0;
  exactBoundedFloat():value(0){;}
  explicit exactBoundedFloat(int val){set(val);}
  explicit exactBoundedFloat(double val){set(val);} // Temporary, consider removing
  explicit operator float()const{return (double)(value)/(double)permyriad;}
  double asPercent()const{return (double)(value)/(double)permyriad * toPercent;}
  void set(int value_in){
    if(value_in >= 0 && value_in <= permyriad){
      value = value_in;
    }else{
      throw std::runtime_error("Bad value in setter");
    }
  }
  void set(double approx_in){
    int val = std::floor((approx_in * permyriad)+halfinc); // NINT
    set(val);
  }
  exactBoundedFloat operator+=(const exactBoundedFloat & a){
    set(value + a.value);
    return *this;
  }
  exactBoundedFloat operator-=(const exactBoundedFloat & a){
    set(value - a.value);
    return *this;
  }
};
inline exactBoundedFloat operator+(exactBoundedFloat a, const exactBoundedFloat & b){
  return a += b;
}
inline exactBoundedFloat operator-(exactBoundedFloat a, const exactBoundedFloat & b){
  return a -= b;
}
inline bool operator==(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value == b.value;
}
inline bool operator!=(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value != b.value;
}
inline bool operator<=(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value <= b.value;
}
inline bool operator<(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value < b.value;
}
inline bool operator>=(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value >= b.value;
}
inline bool operator>(const exactBoundedFloat & a, const exactBoundedFloat & b){
  return a.value > b.value;
}
inline bool operator==(const exactBoundedFloat & a, const float & b){
  return std::abs((float)a - b) < 1e-5;
}
inline std::ostream& operator<< (std::ostream& stream, const exactBoundedFloat & flt){
  stream<<((float) flt);
  return stream;
}
// Equivalent of 1.0 - a
inline exactBoundedFloat oneMinus(exactBoundedFloat a){
  a.set(a.permyriad -a.value);
  return a;
}
using eb_float = exactBoundedFloat;
inline std::string exactPercent(exactBoundedFloat val){
  if(val.value == 0) return "0.00"; //Easier to handle this case specially
  int leading_digs = (val.value == 10000 ? 3 : (val.value > 999 ? 2 : 1)); //Display as 0.xyz
  std::stringstream ss;
  ss<<val.value;
  std::string str = ss.str();
  if(val.value < 100) str = '0'+str; //Restore leading zero
  if(val.value < 10) str = '0'+str; //Another
  //Insert decimal pt
  str = str.substr(0, leading_digs)+'.'+str.substr(leading_digs, str.size());
  return str;
}
/**
 * @brief Return the number with no dp
 *
 * Rounds, not trunc
 * @param val 
 * @return 
 */
inline std::string integerPercent(exactBoundedFloat val){
  std::stringstream ss;
  int tmp = (int)((float)val*100 +0.5);
  ss<< tmp;
  return ss.str();
}

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
  none, /**< \brief Default invalid value */
  flatfile, /**< \brief Flat file data backend */
  database /**< \brief Database data backend */
};

struct appDigestConfig{
  timecode digestCheckPeriod =-1;
  timecode digestCreationDelay = -1;
  bool disableDigests = false;
};
struct timeStampIssueConfig{
  bool ignoreCollisions = true; /**< @brief Indicates to silently bump a non-unique time stamp to later, by up to maxBump */
  timecode maxBump = 2; /**< Max value for silent de-duplication. Setting to 0 is effectively the same as ignore=false but does more work on the way*/
  timecode aLongTime = 60*60*12; /**< An unexpected length of time to be on a single project */
};

struct appFTEConfig{
  // 3 general ideas - per: per project
  //                 - tot: total sum
  //                 - inc: increment in menus etc
  // These MOSTLY control restrictions at the UI level
  eb_float min_per{0}; //Minimum FTE - default to 0 to allow projects with no FTE (one-offs are exempt)
  eb_float max_per{100*eb_float::fromPercent}; // Max FTE for a single project
  eb_float min_inc{5*eb_float::fromPercent}; // Minimum FTE increment
  eb_float max_tot{100*eb_float::fromPercent}; // Maximum FTE
  bool allow_oversub = false; // Disable checks on over subscription - TODO - check consequences
};

struct appUIConfig{
  appFTEConfig fte;
};

struct appConfig{
  bool read_only = false; /**< \brief App backend should be opened in read-only mode (Many operations will fail) */
  std::string dataFileName = "";
  dataBackendType backend = dataBackendType::database; /**< \brief Type of data backend to use */
  appDigestConfig digestConfig;
  timeStampIssueConfig stampConfig;
  appUIConfig UIConfig;
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
  //Create string for given float to nearest 0.25
  float flt = std::floor(value * 4 + 0.5);
  float rem = std::abs(std::remainder(flt, 4));
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
