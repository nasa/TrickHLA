/*!
@file TrickHLA/Int64Interval.hh
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
@trick_link_dependency{../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Interval.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Changed function names to match TrickHLA coding style.}
@revs_end

*/

#ifndef _TRICKHLA_INT64_INTERVAL_HH_
#define _TRICKHLA_INT64_INTERVAL_HH_

// System includes.

// Trick includes.

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"

// HLA include files.
#include RTI1516_HEADER
#include <RTI/time/HLAinteger64Interval.h>

namespace TrickHLA
{

class Int64Interval
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
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
   explicit Int64Interval( int64_t value );

   /*! @brief Initialization constructor for the TrickHLA Int64Interval class.
    *  @param value Floating point double value initialization. */
   explicit Int64Interval( double value );

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
    *  @param lhs Left hand side operand as floating point time interval in seconds. */
   virtual Int64Interval &operator=( double lhs )
   {
      hla_interval = to_microseconds( lhs );
      return ( *this );
   }

   /*! @brief Assignment operator from 64bit integer time value.
    *  @return A corresponding TrickHLA::Int64Interval time value.
    *  @param lhs Left hand side operand as 64bit integer time interval in microseconds. */
   virtual Int64Interval &operator=( int64_t lhs )
   {
      hla_interval = lhs;
      return ( *this );
   }

   /*! @brief Assignment operator from TrickHLA::Int64Inteval time interval value.
    *  @return A corresponding TrickHLA::Int64Interval time value.
    *  @param lhs Left hand side operand as TrickHLA::Int64Interval time interval. */
   virtual Int64Interval &operator=( Int64Interval const &lhs )
   {
      hla_interval = lhs.get_time_in_micros();
      return ( *this );
   }

   /*! @brief Interval time greater than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand as 64bit integer time interval in microseconds. */
   bool operator>( int64_t lhs ) { return ( this->get_time_in_micros() > lhs ); }

   /*! @brief Interval time greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand as floating point time interval in seconds. */
   bool operator>( double lhs )
   {
      return ( this->get_time_in_micros() > Int64Interval::to_microseconds( lhs ) );
   }

   //
   // Access routines
   //
   /*! @brief Get the HLA integer time.
    *  @return A copy of the encapsulated HLAinteger64Interval class. */
   RTI1516_NAMESPACE::HLAinteger64Interval get() const { return ( hla_interval ); }

   //
   // Conversion routines
   //
   // We need only enough methods to intercept / convert incoming data into
   // the encapsulated class...
   /*! @brief Return the whole seconds part of the current timestamp.
    *  @return The whole seconds part of the timestamp in seconds. */
   int64_t get_seconds() const;

   /*! @brief Return the microseconds part of the current timestamp.
    *  @return The microseconds part of the current timestamp. */
   int32_t get_micros() const;

   /*! @brief Return the time, in microseconds, contained in the current
    * timestamp as a 64-bit integer value.
    *  @return Time in integer microseconds. */
   int64_t get_time_in_micros() const;

   /*! @brief Return the current timestamp in seconds as a double precision floating point value.
    *  @return Time in seconds as a floating point double. */
   double get_time_in_seconds() const;

   /*! @brief Returns a summary of the time.
    *  @return Summary of time as a string. */
   std::wstring to_string() const;

   //
   // Mutator methods
   //
   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval in integer microseconds. */
   void set( const int64_t value );

   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval in seconds. */
   void set( const double value );

   /*! @brief Set the time interval to the given value.
    *  @param value The desired time interval as an HLA LogicalTimeInterval. */
   void set( RTI1516_NAMESPACE::LogicalTimeInterval const &value );

   //
   // Static methods
   //
   /*! @brief Converts the given floating point time to an integer representing microseconds.
    *  @return Time value in microseconds.
    *  @param value Time value as a floating point double in seconds. */
   static int64_t to_microseconds( const double value );

   /*! @brief Converts the given integer time in microseconds to an floating-point time representing seconds.
    *  @return Time value in seconds.
    *  @param usec Time value as a 64-bit integer in microseconds. */
   static double to_seconds( const int64_t usec );

   //
   // Private data.
   //
  private:
   RTI1516_NAMESPACE::HLAinteger64Interval hla_interval; /**< @trick_io{**}
      The HLA standard's class representation of integer64 interval. */
};

} // namespace TrickHLA

#endif // _TRICKHLA_INT64_INTERVAL_HH_
