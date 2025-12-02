/*!
@file TrickHLA/time/Int64Interval.hh
@ingroup TrickHLA
@brief This class represents the HLA Interval time.

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
@trick_link_dependency{../../../source/TrickHLA/time/Int64Time.cpp}
@trick_link_dependency{../../../source/TrickHLA/time/Int64Interval.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Changed function names to match TrickHLA coding style.}
@revs_end

*/

#ifndef TRICKHLA_INT64_INTERVAL_HH
#define TRICKHLA_INT64_INTERVAL_HH

// System includes.
#include <cstdint>
#include <string>

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/time/Int64BaseTime.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/time/HLAinteger64Interval.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class Int64Interval
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Int64Interval();

  public:
   //
   // Constructors and Destructor
   //
   /*! @brief Default constructor for the TrickHLA Int64Interval class. */
   Int64Interval() : hla_interval( 0LL )
   {
      return;
   }

   /*! @brief Initialization constructor for the TrickHLA Int64Interval class.
    *  @param value 64bit integer value initialization. */
   explicit Int64Interval( int64_t const value );

   /*! @brief Initialization constructor for the TrickHLA Int64Interval class.
    *  @param value Floating point double value initialization. */
   explicit Int64Interval( double const value );

   /*! @brief Initialization constructor for the TrickHLA Int64Interval class.
    *  @param value HLA Logical Time Interval value initialization. */
   explicit Int64Interval( RTI1516_NAMESPACE::LogicalTimeInterval const &value );

   /*! @brief Initialization constructor for the TrickHLA Int64Interval class.
    *  @param value HLA 64bit Integer Time Interval value initialization. */
   explicit Int64Interval( RTI1516_NAMESPACE::HLAinteger64Interval const &value );

   /*! @brief Copy constructor for the TrickHLA Int64Interval class.
    *  @param value TrickHLA Long Integer Time Interval value initialization. */
   Int64Interval( Int64Interval const &value );

   /*! @brief Destructor for the TrickHLA Int64Interval class. */
   virtual ~Int64Interval();

   //
   // Operators
   //
   /*! @brief Assignment operator from double time value.
    *  @return A corresponding TrickHLA::Int64Interval time value.
    *  @param rhs Right hand side operand as floating point time interval in seconds. */
   Int64Interval &operator=( double rhs )
   {
      this->hla_interval = Int64BaseTime::to_base_time( rhs );
      return ( *this );
   }

   /*! @brief Assignment operator from 64bit integer time value.
    *  @return A corresponding TrickHLA::Int64Interval time value.
    *  @param rhs Right hand side operand as 64bit integer time interval in the base time. */
   Int64Interval &operator=( int64_t const rhs )
   {
      this->hla_interval = rhs;
      return ( *this );
   }

   /*! @brief Assignment operator from TrickHLA::Int64Inteval time interval value.
    *  @return A corresponding TrickHLA::Int64Interval time value.
    *  @param rhs Right hand side operand as TrickHLA::Int64Interval time interval. */
   Int64Interval &operator=( Int64Interval const &rhs )
   {
      this->hla_interval = rhs.get_base_time();
      return ( *this );
   }

   /*! @brief Interval time greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as floating point time interval in seconds. */
   bool operator>( double const rhs ) const
   {
      return ( get_base_time() > Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Interval time greater than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as 64bit integer time interval in the base time. */
   bool operator>( int64_t const rhs ) const
   {
      return ( get_base_time() > rhs );
   }

   /*! @brief Interval time greater than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as TrickHLA::Int64Interval time interval. */
   bool operator>( Int64Interval const &rhs ) const
   {
      return ( get_base_time() > rhs.get_base_time() );
   }

   //
   // Access routines
   //
   /*! @brief Get the HLA integer time.
    *  @return A copy of the encapsulated HLAinteger64Interval class. */
   RTI1516_NAMESPACE::HLAinteger64Interval get() const
   {
      return ( this->hla_interval );
   }

   //
   // Conversion routines
   //
   /*! @brief Return the time, in the base time, contained in the current
    * timestamp as a 64-bit integer value.
    *  @return Time in integer the base time. */
   int64_t get_base_time() const;

   /*! @brief Return the current timestamp in seconds as a double precision floating point value.
    *  @return Time in seconds as a floating point double. */
   double get_time_in_seconds() const;

   /*! @brief Return true if the value is zero, false otherwise.
    *  @return True if zero, false otherwise. */
   bool is_zero() const;

   /*! @brief Returns a summary of the time.
    *  @return Summary of time as a wide string. */
   std::wstring to_wstring() const;

   //
   // Mutator methods
   //
   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval in integer the base time. */
   void set( int64_t const value );

   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval in seconds. */
   void set( double const value );

   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval as an HLA LogicalTimeInterval. */
   void set( RTI1516_NAMESPACE::LogicalTimeInterval const &value );

   //
   // Private data.
   //
  private:
   /*! @brief Return the whole seconds part of the current timestamp.
    *  @return The whole seconds part of the timestamp in seconds. */
   int64_t get_seconds() const;

   /*! @brief Return the the fractional part of the current timestamp.
    *  @return The the fractional part of the current timestamp. */
   int64_t get_fractional_seconds() const;

   RTI1516_NAMESPACE::HLAinteger64Interval hla_interval; /**< @trick_io{**}
      The HLA standard's class representation of integer64 interval. */
};

} // namespace TrickHLA

#endif // TRICKHLA_INT64_INTERVAL_HH
