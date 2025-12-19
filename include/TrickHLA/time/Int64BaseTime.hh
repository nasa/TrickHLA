/*!
@file TrickHLA/time/Int64BaseTime.hh
@ingroup TrickHLA
@brief This class represents an integer time for a given base time unit.

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
@trick_link_dependency{../../../source/TrickHLA/time/Int64BaseTime.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2023, --, Base time for given time unit.}
@revs_end

*/

#ifndef TRICKHLA_INT64_BASE_TIME_HH
#define TRICKHLA_INT64_BASE_TIME_HH

// System includes.
#include <cstdint>
#include <string>

// TrickHLA includes.
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
   /*! @brief Default constructor with microsecond base base_unit_string. */
   Int64BaseTime();

   /*! @brief Constructor with base time multiplier specified.
    *  @param multiplier The base time multiplier to use. */
   explicit Int64BaseTime( int64_t const multiplier );

   /*! @brief Constructor with base unit enum specified.
    *  @param unit The base time unit to use. */
   explicit Int64BaseTime( HLABaseTimeEnum const unit );

   /*! @brief Destructor for the TrickHLA Int64BaseTime class. */
   virtual ~Int64BaseTime();

   /*! @brief Set the base time resolution multiplier.
    *  @param multiplier The base time multiplier to use. */
   static void set( int64_t const multiplier );

   /*! @brief Set the base time resolution for the enum value.
    *  @param unit The base time unit to use. */
   static void set( HLABaseTimeEnum unit );

   /*! @brief Update the unit and unit string for the multiplier. */
   static void update_unit_for_multiplier();

   /*! @brief The base_unit of the base time.
    *  @return The base_unit of the base time. */
   static HLABaseTimeEnum const get_base_unit_enum()
   {
      return base_unit;
   }

   /*! @brief The base_unit_string of the base time as a string.
    *  @return The base_unit_string of the base time as a string. */
   static std::string const &get_base_unit()
   {
      return base_unit_string;
   }

   /*! @brief A string representing the specified unit.
    *  @param unit The base time unit.
    *  @return A string representing the specified unit. */
   static std::string const get_base_unit_enum_string( HLABaseTimeEnum const unit );

   /*! @brief The base time multiplier.
    *  @return The base time multiplier. */
   static int64_t const get_base_time_multiplier()
   {
      return base_time_multiplier;
   }

   /*! @brief The maximum base time.
    *  @return The maximum base time. */
   static int64_t const get_max_base_time()
   {
      return INT64_MAX;
   }

   /*! @brief Determine the best supporting base time resolution for the value.
    *  @return The best supporting base time enum value.
    *  @param value Time value as a floating point double in seconds. */
   static HLABaseTimeEnum const best_base_time_resolution( double const value );

   /*! @brief Determine if the specified value exceeds the resolution of
    *  the base time (i.e. value is much smaller than base time resolution).
    *  @return True if the value exceeds the resolution of the base time.
    *  @param value Time value as a floating point double in seconds. */
   static bool const exceeds_base_time_resolution( double const value );

   /*! @brief Determine if the specified value exceeds the resolution of
    *  a base time with the corresponding multiplier.
    *  @return True if the value exceeds the resolution of the base time.
    *  @param value Time value as a floating point double in seconds.
    *  @param multiplier Base time multiplier. */
   static bool const exceeds_base_time_resolution( double const value, int64_t multiplier );

   /*! @brief Converts the given floating point time to an integer representing
    *  the time in the HLA Logical base time.
    *  @return Time value in the HLA Logical base time.
    *  @param time The time in seconds as a floating point double. */
   static int64_t const to_base_time( double const time );

   /*! @brief Converts the given integer time to an floating-point time representing seconds.
    *  @return Time value in seconds.
    *  @param time_in_base_unit Time value as a 64-bit integer in the base_unit_string specified for this class. */
   static double const to_seconds( int64_t const time_in_base_unit );

   /*! @brief Converts the given integer time to an integer time representing whole seconds.
    *  @return Time value in whole seconds.
    *  @param time_in_base_unit Time value as a 64-bit integer in the base_unit_string specified for this class. */
   static int64_t const to_whole_seconds( int64_t const time_in_base_unit );

  protected:
   static HLABaseTimeEnum base_unit;            ///< @trick_units{--} Base time unit.
   static std::string     base_unit_string;     ///< @trick_units{--} Base time unit as a string.
   static int64_t         base_time_multiplier; ///< @trick_units{--} Multiplier for the base unit.

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
