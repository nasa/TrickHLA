/*!
@file TrickHLA/Int64Time.hh
@ingroup TrickHLA
@brief This class represents the HLA time.

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
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Interval.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Changed function names to match TrickHLA coding style.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2023, --, Added operator overloads for int64_t and Int64Interval types.}
@revs_end

*/

#ifndef TRICKHLA_INT64_TIME_HH
#define TRICKHLA_INT64_TIME_HH

// System includes.
#include <cstdint>
#include <string>

// TrickHLA includes.
#include "Int64BaseTime.hh"
#include "Int64Interval.hh"
#include "StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"
#include <RTI/time/HLAinteger64Time.h>

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class Int64Time
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Int64Time();

  public:
   //
   // Constructors and Destructor
   //
   /*! @brief Default constructor for the TrickHLA Int64Time class. */
   Int64Time() : hla_time( 0LL )
   {
      return;
   }

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value 64bit integer value initialization. */
   explicit Int64Time( int64_t const value );

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value Floating point double value initialization. */
   explicit Int64Time( double const value );

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value HLA Logical Time value initialization. */
   explicit Int64Time( RTI1516_NAMESPACE::LogicalTime const &value );

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value HLA 64bit Integer Time value initialization. */
   explicit Int64Time( RTI1516_NAMESPACE::HLAinteger64Time const &value );

   /*! @brief Copy constructor for the TrickHLA Int64Time class.
    *  @param value TrickHLA Long Integer Time value initialization. */
   Int64Time( Int64Time const &value );

   /*! @brief Destructor for the TrickHLA Int64Time class. */
   virtual ~Int64Time();

   //
   // Operators
   //
   /*! @brief Assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as floating point double time in seconds. */
   Int64Time &operator=( double const rhs )
   {
      this->hla_time = Int64BaseTime::to_base_time( rhs );
      return ( *this );
   }

   /*! @brief Assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time &operator=( int64_t const rhs )
   {
      this->hla_time = rhs;
      return ( *this );
   }

   /*! @brief Assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as TrickHLA::Int64Time. */
   Int64Time &operator=( Int64Time const &rhs )
   {
      this->hla_time = rhs.get_base_time();
      return ( *this );
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator+=( double const rhs )
   {
      set( get_base_time() + Int64BaseTime::to_base_time( rhs ) );
      return ( *this );
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator+=( int64_t const rhs )
   {
      set( get_base_time() + rhs );
      return ( *this );
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator+=( Int64Interval const &rhs )
   {
      set( get_base_time() + rhs.get_base_time() );
      return ( *this );
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator+=( Int64Time const &rhs )
   {
      set( get_base_time() + rhs.get_base_time() );
      return ( *this );
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator+( double const rhs ) const
   {
      Int64Time t( get_base_time() + Int64BaseTime::to_base_time( rhs ) );
      return ( t );
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator+( int64_t const rhs ) const
   {
      Int64Time t( get_base_time() + rhs );
      return ( t );
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator+( Int64Interval const &rhs ) const
   {
      Int64Time t( get_base_time() + rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator+( Int64Time const &rhs ) const
   {
      Int64Time t( get_base_time() + rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator-( double const rhs ) const
   {
      Int64Time t( get_base_time() - Int64BaseTime::to_base_time( rhs ) );
      return ( t );
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator-( int64_t const rhs ) const
   {
      Int64Time t( get_base_time() - rhs );
      return ( t );
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator-( Int64Interval const &rhs ) const
   {
      Int64Time t( get_base_time() - rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator-( Int64Time const &rhs ) const
   {
      Int64Time t( get_base_time() - rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Multiplication operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator*( double const rhs ) const
   {
      Int64Time t( get_base_time() * Int64BaseTime::to_base_time( rhs ) );
      return ( t );
   }

   /*! @brief Multiplication operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator*( int64_t const rhs ) const
   {
      Int64Time t( get_base_time() * rhs );
      return ( t );
   }

   /*! @brief Multiplication operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator*( Int64Interval const &rhs ) const
   {
      Int64Time t( get_base_time() * rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Multiplication operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator*( Int64Time const &rhs ) const
   {
      Int64Time t( get_base_time() * rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Division operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator/( double const rhs ) const
   {
      Int64Time t( get_base_time() / Int64BaseTime::to_base_time( rhs ) );
      return ( t );
   }

   /*! @brief Division operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator/( int64_t const rhs ) const
   {
      Int64Time t( get_base_time() / rhs );
      return ( t );
   }

   /*! @brief Division operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator/( Int64Interval const &rhs ) const
   {
      Int64Time t( get_base_time() / rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Division operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator/( Int64Time const &rhs ) const
   {
      Int64Time t( get_base_time() / rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Modulo operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   Int64Time operator%( double const rhs ) const
   {
      Int64Time t( get_base_time() % Int64BaseTime::to_base_time( rhs ) );
      return ( t );
   }

   /*! @brief Modulo operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   Int64Time operator%( int64_t const rhs ) const
   {
      Int64Time t( get_base_time() % rhs );
      return ( t );
   }

   /*! @brief Modulo operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator%( Int64Interval const &rhs ) const
   {
      Int64Time t( get_base_time() % rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Modulo operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator%( Int64Time const &rhs ) const
   {
      Int64Time t( get_base_time() % rhs.get_base_time() );
      return ( t );
   }

   /*! @brief Less than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   bool operator<( double const rhs ) const
   {
      return ( get_base_time() < Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Less than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   bool operator<( int64_t const rhs ) const
   {
      return ( get_base_time() < rhs );
   }

   /*! @brief Less than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   bool operator<( Int64Time const &rhs ) const
   {
      return ( get_base_time() < rhs.get_base_time() );
   }

   /*! @brief Greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as floating point time in seconds. */
   bool operator>( double const rhs ) const
   {
      return ( get_base_time() > Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Greater than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand as a 64bit integer time in the base time. */
   bool operator>( int64_t const rhs ) const
   {
      return ( get_base_time() > rhs );
   }

   /*! @brief Greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param rhs Right hand side operand a TrickHLA::Int64Time. */
   bool operator>( Int64Time const &rhs ) const
   {
      return ( get_base_time() > rhs.get_base_time() );
   }

   /*! @brief Less than or equal to comparison operator.
    *  @return True is right operand is less than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   bool operator<=( double const rhs ) const
   {
      return ( get_base_time() <= Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Less than or equal to comparison operator.
    *  @return True is right operand is less than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   bool operator<=( int64_t const rhs ) const
   {
      return ( get_base_time() <= rhs );
   }

   /*! @brief Less than or equal to comparison operator.
    *  @return True is right operand is less than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand a TrickHLA::Int64Time. */
   bool operator<=( Int64Time const &rhs ) const
   {
      return ( get_base_time() <= rhs.get_base_time() );
   }

   /*! @brief Greater than or equal to comparison operator.
    *  @return True is right operand is greater than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   bool operator>=( double const rhs ) const
   {
      return ( get_base_time() >= Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Greater than or equal to comparison operator.
    *  @return True is right operand is greater than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as 64bit integer time in the base time. */
   bool operator>=( int64_t const rhs ) const
   {
      return ( get_base_time() >= rhs );
   }

   /*! @brief Greater than or equal to comparison operator.
    *  @return True is right operand is greater than or equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   bool operator>=( Int64Time const &rhs ) const
   {
      return ( get_base_time() >= rhs.get_base_time() );
   }

   /*! @brief Equals comparison operator.
    *  @return True is right operand is equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   bool operator==( double const rhs ) const
   {
      return ( get_base_time() == Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Equals comparison operator.
    *  @return True is right operand is equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a 64bit integer time in the base time. */
   bool operator==( int64_t const rhs ) const
   {
      return ( get_base_time() == rhs );
   }

   /*! @brief Equals comparison operator.
    *  @return True is right operand is equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   bool operator==( Int64Time const &rhs ) const
   {
      return ( get_base_time() == rhs.get_base_time() );
   }

   /*! @brief Not equal to comparison operator.
    *  @return True is right operand is not equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a floating point double time in seconds. */
   bool operator!=( double const rhs ) const
   {
      return ( get_base_time() != Int64BaseTime::to_base_time( rhs ) );
   }

   /*! @brief Not equal to comparison operator.
    *  @return True is right operand is not equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a 64bit integer time in the base time. */
   bool operator!=( int64_t const rhs ) const
   {
      return ( get_base_time() != rhs );
   }

   /*! @brief Not equal to comparison operator.
    *  @return True is right operand is not equal to the left operand; False otherwise.
    *  @param rhs Right hand side operand as a TrickHLA::Int64Time. */
   bool operator!=( Int64Time const &rhs ) const
   {
      return ( get_base_time() != rhs.get_base_time() );
   }

   //
   // Interface routines
   //
   /*! @brief Get the HLA integer time.
    *  @return A copy of the encapsulated HLAinteger64Time class. */
   RTI1516_NAMESPACE::HLAinteger64Time get() const
   {
      return ( this->hla_time );
   }

   /*! @brief Encode the time as an HLAinteger64BE.
    *  @return Encoded time as an HLAinteger64BE. */
   RTI1516_NAMESPACE::VariableLengthData encode() const;

   /*! @brief Saves the incoming HLA encoded LogicalTime into the encapsulated class.
    *  @param user_supplied_tag Time encoded in user supplied tag. */
   void decode( RTI1516_NAMESPACE::VariableLengthData const &user_supplied_tag );

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

   /*! @brief Returns a string representing the time.
    *  @return String representing the time. */
   std::string to_string() const;

   //
   // Mutator methods
   //
   /*! @brief Set the time to the given value.
    *  @param value The desired time in integer the base time. */
   void set( int64_t const value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time interval in seconds. */
   void set( double const value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time as an HLA LogicalTime. */
   void set( RTI1516_NAMESPACE::LogicalTime const &value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time as a TrickHLA::Int64Time. */
   void set( Int64Time const &value );

  private:
   /*! @brief Return the whole seconds part of the current timestamp.
    *  @return The whole seconds part of the timestamp in seconds. */
   int64_t get_seconds() const;

   /*! @brief Return the the fractional time part of the current timestamp.
    *  @return The the fractional part of the current timestamp. */
   int64_t get_fractional_seconds() const;

   RTI1516_NAMESPACE::HLAinteger64Time hla_time; /**< @trick_io{**}
      HLA standard's class representation of integer64 time. */

}; // end of Int64Time class

} // namespace TrickHLA

#endif // TRICKHLA_INT64_TIME_HH
