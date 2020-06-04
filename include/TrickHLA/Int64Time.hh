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
@trick_link_dependency{../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Interval.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_INT64_TIME_HH_
#define _TRICKHLA_INT64_TIME_HH_

// System include files.
#include <stdint.h>

// Trick include files.

// HLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include RTI1516_HEADER
#include <RTI/time/HLAinteger64Time.h>

// TrickHLA includes
#include "TrickHLA/Int64Interval.hh"

namespace TrickHLA
{

class Int64Time
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
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
   Int64Time() : _HLAtime( 0LL ) {}

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value 64bit integer value initialization. */
   explicit Int64Time( int64_t value );

   /*! @brief Initialization constructor for the TrickHLA Int64Time class.
    *  @param value Floating point double value initialization. */
   explicit Int64Time( double value );

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
    *  @param lhs Left hand side operand as floating point double time in seconds. */
   virtual Int64Time &operator=( double lhs )
   {
      _HLAtime = Int64Interval::toMicroseconds( lhs );
      return *this;
   }

   /*! @brief Assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as 64bit integer time in microseconds. */
   virtual Int64Time &operator=( int64_t lhs )
   {
      _HLAtime = lhs;
      return *this;
   }

   /*! @brief Assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as TrickHLA::Int64Time. */
   virtual Int64Time &operator=( Int64Time const &lhs )
   {
      _HLAtime = lhs.getTimeInMicros();
      return *this;
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a floating point double time in seconds. */
   Int64Time operator+=( double lhs )
   {
      int64_t val = this->getTimeInMicros();
      val += Int64Interval::toMicroseconds( lhs );
      this->setTo( val );
      return *this;
   }

   /*! @brief Addition then assignment operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator+=( const Int64Interval &lhs )
   {
      int64_t val = this->getTimeInMicros();
      val += lhs.getTimeInMicros();
      this->setTo( val );
      return *this;
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator+( const Int64Time &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() + lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Addition operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator+( const Int64Interval &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() + lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   Int64Time operator-( const Int64Time &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() - lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Subtraction operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator-( const Int64Interval &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() - lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Multiplication operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator*( const Int64Interval &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() * lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Division operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator/( const Int64Interval &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() / lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Modulo operator.
    *  @return A corresponding TrickHLA::Int64Time time value.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Interval. */
   Int64Time operator%( const Int64Interval &lhs ) const
   {
      Int64Time x( this->getTimeInMicros() % lhs.getTimeInMicros() );
      return x;
   }

   /*! @brief Less than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   bool operator<( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() < lhs.getTimeInMicros();
   }

   /*! @brief Greater than comparison operator.
    *  @return True if right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand as a 64bit integer time in microseconds. */
   bool operator>( int64_t lhs ) { return this->getTimeInMicros() > lhs; }

   /*! @brief Greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand as floating point time in seconds. */
   bool operator>( double lhs )
   {
      return this->getTimeInMicros() > Int64Interval::toMicroseconds( lhs );
   }

   /*! @brief Greater than comparison operator.
    *  @return True is right operand is greater than the left operand; False otherwise.
    *  @param lhs Left hand side operand a TrickHLA::Int64Time. */
   bool operator>( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() > lhs.getTimeInMicros();
   }

   /*! @brief Less than or equal to comparison operator.
    *  @return True is right operand is less than or equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand a TrickHLA::Int64Time. */
   bool operator<=( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() <= lhs.getTimeInMicros();
   }

   /*! @brief Greater than or equal to comparison operator.
    *  @return True is right operand is greater than or equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   bool operator>=( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() >= lhs.getTimeInMicros();
   }

   /*! @brief Equals comparison operator.
    *  @return True is right operand is equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand as a 64bit integer time in microseconds. */
   bool operator==( int64_t lhs ) const
   {
      return this->getTimeInMicros() == lhs;
   }

   /*! @brief Equals comparison operator.
    *  @return True is right operand is equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   bool operator==( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() == lhs.getTimeInMicros();
   }

   /*! @brief Not equal to comparison operator.
    *  @return True is right operand is not equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand as a 64bit integer time in microseconds. */
   bool operator!=( int64_t lhs ) const
   {
      return this->getTimeInMicros() != lhs;
   }

   /*! @brief Not equal to comparison operator.
    *  @return True is right operand is not equal to the left operand; False otherwise.
    *  @param lhs Left hand side operand as a TrickHLA::Int64Time. */
   bool operator!=( const Int64Time &lhs ) const
   {
      return this->getTimeInMicros() != lhs.getTimeInMicros();
   }

   //
   // Interface routines
   //
   /*! @brief Get the HLA integer time.
    *  @return A copy of the encapsulated HLAinteger64Time class. */
   RTI1516_NAMESPACE::HLAinteger64Time get() const { return _HLAtime; }

   // decodes the HLA encoded time into encapsulated class
   /*! @brief Saves the incoming HLA encoded LogicalTime into the encapsulated class.
    *  @param user_supplied_tag Time encoded in user supplied tag. */
   void decode( RTI1516_USERDATA const &user_supplied_tag );

   //
   // Conversion routines
   //
   // we need only enough methods to intercept / convert incoming data into
   // the encapsulated class...
   /*! @brief Return the seconds contained in the current timestamp.
    *  @return The current timestamp in seconds. */
   long getSeconds() const;

   /*! @brief Return the microseconds seconds contained in the current timestamp.
    *  @return The current timestamp in integer microseconds. */
   int getMicros() const;

   /*! @brief Return the time, in microseconds, contained in the current
    * timestamp as a 64-bit integer value.
    *  @return Time in integer microseconds. */
   int64_t getTimeInMicros() const;

   /*! @brief Return the current timestamp as a double precision floating point value.
    *  @return Time in seconds as a floating point double. */
   double getDoubleTime() const;

   /*! @brief Returns a wide string representing the time.
    *  @return Wide string representing the time. */
   std::wstring toString() const;

   //
   // Mutator methods
   //
   /*! @brief Set the time to the given value.
    *  @param value The desired time in integer microseconds. */
   void setTo( const int64_t value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time interval in seconds. */
   void setTo( const double value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time as an HLA LogicalTime. */
   void setTo( RTI1516_NAMESPACE::LogicalTime const &value );

   /*! @brief Set the time to the given value.
    *  @param value The desired time as a TrickHLA::Int64Time. */
   void setTo( Int64Time const &value );

  private:
   RTI1516_NAMESPACE::HLAinteger64Time _HLAtime; /**< @trick_io{**}
      HLA standard's class representation of integer64 time. */

}; // end of Int64Time class

} // namespace TrickHLA

#endif // _TRICKHLA_INT64_TIME_HH_
