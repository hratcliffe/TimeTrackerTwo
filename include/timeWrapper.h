#ifndef ____timeWrapper__
#define ____timeWrapper__

#include <chrono>
#include <ctime> // Still need some C-style stuff for dates
#include <iomanip>
#include <sstream>


using TW_clock = std::chrono::system_clock; /**< \brief Type of clock used for time operations */
using TW_timePoint = std::chrono::time_point<TW_clock>; /**< \brief Type of time point used in the application */
using TW_duration = std::chrono::duration<long long>; /**< \brief Type of duration used for time calculations */

/** \brief A wrapper for time operations
 * 
 * Provides a consistent interface for getting the current time, formatting it, and converting between different time representations. STATELESS. Allows the rest of the app to use time without worrying about the underlying implementation.
 */
class timeWrapper{
  public:
    using clock = TW_clock;
    using timePoint = TW_timePoint;
    using duration = TW_duration;

    static timePoint now() {
      return clock::now(); /**< \brief Get the current time point */
    }

    static long long toSeconds(timePoint tp) {
      return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count(); /**< \brief Convert time point to seconds since epoch */
    }

    static timePoint fromSeconds(long long seconds) {
      return clock::time_point(std::chrono::seconds(seconds)); /**< \brief Create a time point from seconds since epoch */
    }

    static timePoint referenceTime() {
      return clock::from_time_t(0); /**< \brief Get the reference time (epoch) */
    }
    static std::string formatTime(timePoint tp) {
      std::time_t time = clock::to_time_t(tp); /**< \brief Convert time point to time_t for formatting */
      char buffer[100];
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time)); /**< \brief Format time as a string */
      return std::string(buffer);
    }
    static std::string formatTimeAsClock(timePoint tp) {
      std::time_t time = clock::to_time_t(tp); /**< \brief Convert time point to time_t for formatting */
      char buffer[100];
      std::strftime(buffer, sizeof(buffer), "%H:%M", std::localtime(&time)); /**< \brief Format time as a string */
      return std::string(buffer);
    }
    static timePoint parseTime(const std::string &timeStr) {
      //Assumes string is GMT, does not attempt to parse any zoning
      std::tm tm = {};
      std::istringstream ss(timeStr);
      ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
      if (ss.fail()) {
        throw std::runtime_error("Failed to parse time string: " + timeStr);
      }
      return clock::from_time_t(std::mktime(&tm));
    }
    static timePoint parseTimeZoned(const std::string &timeStr) {
      //Parse the time string correctly as clock time in current time zone
      std::tm tm = {};
      std::istringstream ss(timeStr);
      ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
      if (ss.fail()) {
        throw std::runtime_error("Failed to parse time string: " + timeStr);
      }
      tm.tm_isdst = -1; //TODO - try and verify that this works?
      return clock::from_time_t(std::mktime(&tm));
    }

    // Get the time which is the midnight (start of day) containing the given time
    static timePoint midnightBefore(timePoint tp){
      std::time_t theTime = clock::to_time_t(tp);
      auto timeInfo = localtime(&theTime);
      std::cout<<timeInfo->tm_hour<<" "<<timeInfo->tm_min<<std::endl;
      timeInfo->tm_hour = 0;
      timeInfo->tm_min = 0;
      timeInfo->tm_sec = 0;
      return clock::from_time_t(mktime(timeInfo));
    }
    static timePoint startOfMonth(timePoint tp){
      std::time_t theTime = clock::to_time_t(tp);
      auto timeInfo = localtime(&theTime);
      std::cout<<timeInfo->tm_hour<<" "<<timeInfo->tm_min<<std::endl;
      timeInfo->tm_mday = 1;
      timeInfo->tm_hour = 0;
      timeInfo->tm_min = 0;
      timeInfo->tm_sec = 0;
      return clock::from_time_t(mktime(timeInfo));
    }

  };



#endif