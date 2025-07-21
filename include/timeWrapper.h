#ifndef ____timeWrapper__
#define ____timeWrapper__

#include <chrono>
#include <ctime> // Still need some C-style stuff for dates
#include <iomanip>
#include <sstream>

/** \brief A wrapper for time operations
 * 
 * Provides a consistent interface for getting the current time, formatting it, and converting between different time representations. STATELESS. Allows the rest of the app to use time without worrying about the underlying implementation.
 */
class timeWrapper{
  public:
    using clock = std::chrono::system_clock; /**< \brief Type of clock used for time operations */
    using timePoint = std::chrono::time_point<clock>; /**< \brief Type of time point used in the application */
    using duration = std::chrono::duration<long long>; /**< \brief Type of duration used for time calculations */

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
    static timePoint parseTime(const std::string &timeStr) {
      std::tm tm = {};
      std::istringstream ss(timeStr);
      ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S"); /**< \brief Parse a time string into a time_point */
      if (ss.fail()) {
        throw std::runtime_error("Failed to parse time string: " + timeStr);
      }
      return clock::from_time_t(std::mktime(&tm));
    }

};

#endif