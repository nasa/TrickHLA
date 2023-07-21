/*!
@file TrickHLA/Int64BaseTime.hh
@ingroup TrickHLA
@brief This class represents an integer time for a given base time units.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../source/TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2023, --, Base time for given time unit.}
@revs_end

*/

#ifndef TRICKHLA_INT64_BASE_TIME_HH
#define TRICKHLA_INT64_BASE_TIME_HH

// System include files.
#include <cstdint>
#include <string>

// Trick include files.

// TrickHLA includes
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class Int64BaseTime
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Int64BaseTime();

  public:
   //
   // Constructors and Destructor
   //
   /*! @brief Default constructor with microsecond base units_string. */
   Int64BaseTime();

   /*! @brief Constructor with base units_string specified.
    *  @param units The base time units to use. */
   Int64BaseTime( HLABaseTimeEnum const units );

   /*! @brief Destructor for the TrickHLA Int64BaseTime class. */
   virtual ~Int64BaseTime();

   /*! @brief Determine if the specified value exceeds the base time resolution.
    *  @param units The base time units to use. */
   static void set( HLABaseTimeEnum const units );

   /*! @brief The base_units of the base time.
    *  @return The base_units of the base time. */
   static HLABaseTimeEnum get_base_units()
   {
      return base_units;
   }

   /*! @brief The units_string of the base time as a string.
    *  @return The units_string of the base time as a string. */
   static std::string get_units()
   {
      return units_string;
   }

   /*! @brief A string representing the specified units.
    *  @param units The base time units.
    *  @return A string representing the specified units. */
   static std::string get_units_string( HLABaseTimeEnum const units );

   /*! @brief The base time multiplier.
    *  @return The base time multiplier. */
   static int64_t get_base_time_multiplier()
   {
      return base_time_multiplier;
   }

   /*! @brief The maximum logical time in seconds given the base time.
    *  @return The maximum logical time in seconds given the base time. */
   static double get_max_logical_time_in_seconds()
   {
      return max_logical_time_seconds;
   }

   /*! @brief The maximum base time.
    *  @return The maximum base time. */
   static int64_t get_max_base_time()
   {
      return ( INT64_MAX );
   }

   /*! @brief Determine the best supporting base time resolution for the value.
    *  @return The best supporting base time enum value.
    *  @param value Time value as a floating point double in seconds. */
   static HLABaseTimeEnum best_base_time_resolution( double const value );

   /*! @brief Determine if the specified value exceeds the resolution of
    *  the base time (i.e. value is much smaller than base time resolution).
    *  @return True if the value exceeds the resolution of the base time.
    *  @param value Time value as a floating point double in seconds. */
   static bool exceeds_base_time_resolution( double const value );

   /*! @brief Determine if the specified value exceeds the resolution of
    *  a base time with the corresponding multiplier.
    *  @return True if the value exceeds the resolution of the base time.
    *  @param value Time value as a floating point double in seconds.
    *  @param multiplier Base time multiplier. */
   static bool exceeds_base_time_resolution( double const value, long long multiplier );

   /*! @brief Converts the given floating point time to an integer representing
    *  the time in the HLA Logical base time.
    *  @return Time value in the HLA Logical base time.
    *  @param value Time value as a floating point double in seconds. */
   static int64_t to_base_time( double const value );

   /*! @brief Converts the given integer time to an floating-point time representing seconds.
    *  @return Time value in seconds.
    *  @param time_in_base_units Time value as a 64-bit integer in the units_string specified for this class. */
   static double to_seconds( int64_t const time_in_base_units );

   /*! @brief Converts the given integer time to an integer time representing whole seconds.
    *  @return Time value in whole seconds.
    *  @param time_in_base_units Time value as a 64-bit integer in the units_string specified for this class. */
   static int64_t to_whole_seconds( int64_t const time_in_base_units );

  protected:
   static HLABaseTimeEnum base_units;               ///< @trick_units{--} Base time units.
   static std::string     units_string;             ///< @trick_units{--} Base time units as a string.
   static int64_t         base_time_multiplier;     ///< @trick_units{--} Multiplier for the base units.
   static double          max_logical_time_seconds; ///< @trick_units{--} Maximum logical time supported in seconds.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Int64BaseTime class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Int64BaseTime( Int64BaseTime const &rhs );

   /*! @brief Assignment operator for Int64BaseTime class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Int64BaseTime &operator=( Int64BaseTime const &rhs );

}; // end of Int64BaseTime class

} // namespace TrickHLA

#endif // TRICKHLA_INT64_BASE_TIME_HH
