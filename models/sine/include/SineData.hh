/*!
@ingroup Sine
@file models/sine/include/SineData.hh
@brief This is a container class for general test data used in the general HLA
test routines.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{sine/src/SineData.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan-AEU, DSES, January 2003, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_SINE_DATA_HH
#define TRICKHLA_MODEL_SINE_DATA_HH

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

class SineData
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineData();

  public:
   // Model state values.
   double time;  ///< @trick_units{s}  Current time for test data model.
   double value; ///< @trick_units{--} Current output value for test data model.
   double dvdt;  ///< @trick_units{--} Current time derivative of the test data.

   // Modeling parameters.
   double phase; ///< @trick_units{radian}   Phase offset.
   double freq;  ///< @trick_units{radian/s} Frequency of sine wave.
   double amp;   ///< @trick_units{--}       Amplitude of sine wave.
   double tol;   ///< @trick_units{--}       Tolerance for phase adjustment.

   char *name; ///< @trick_units{--} Name of the data.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineData class. */
   SineData();
   /*! @brief Initialization constructor for the TrickHLAModel SineData class.
    *  @param phi   Sine wave phase {rad}.
    *  @param omega Sine wave frequency {rad/s}.
    *  @param mag   Sine wave magnitude. */
   SineData( double phi, double omega, double mag );
   /*! @brief Destructor for the TrickHLAModel SineData class. */
   virtual ~SineData();

   // Public access functions.
   /*! @brief Set the current time for the sine wave.
    *  @param t Time value for sine wave function. */
   int set_time( double const t )
   {
      this->time = t;
      return ( 0 );
   }

   /*! @brief Get the current time value for the sine wave function.
    *  @return The current time value for the sine wave function. */
   double get_time() const
   {
      return time;
   }

   /*! @brief Set the current value of the sine wave function.
    *  @param val Sine wave value. */
   void set_value( double const val )
   {
      this->value = val;
   }

   /*! @brief Get the current value of the sine wave function.
    *  @return Current value of the sine wave function. */
   double get_value() const
   {
      return value;
   }

   /*! @brief Set the value of the time derivative of the sine wave function.
    *  @param deriv The time derivative value of the sine wave function. */
   void set_derivative( double const deriv )
   {
      this->dvdt = deriv;
   }

   /*! @brief Get the value of the time derivative of the sine wave function.
    *  @return The current value of the time derivative of the sine wave function. */
   double get_derivative() const
   {
      return dvdt;
   }

   /*! @brief Set the phase value of the sine wave function.
    *  @param phi The phase value of the sine wave function. */
   void set_phase( double const phi )
   {
      this->phase = phi;
   }

   /*! @brief Get the phase value of the sine wave function.
    *  @return The phase value of the sine wave function. */
   double get_phase() const
   {
      return phase;
   }

   /*! @brief Set the frequency value of the sine wave function.
    *  @param omega The frequency value of the sine wave function. */
   void set_frequency( double const omega )
   {
      this->freq = omega;
   }

   /*! @brief Get the frequency value of the sine wave function.
    *  @return The frequency value of the sine wave function. */
   double get_frequency() const
   {
      return freq;
   }

   /*! @brief Set the amplitude value of the sine wave function.
    *  @param mag The amplitude value of the sine wave function. */
   void set_amplitude( double const mag )
   {
      this->amp = mag;
   }

   /*! @brief Get the amplitude value of the sine wave function.
    *  @return The amplitude value of the sine wave function. */
   double get_amplitude() const
   {
      return amp;
   }

   /*! @brief Set the tolerance value of the sine wave function.
    *  @param epsilon The tolerance value of the sine wave function. */
   void set_tolerance( double const epsilon )
   {
      this->tol = epsilon;
   }

   /*! @brief Get the tolerance value of the sine wave function.
    *  @return The tolerance value of the sine wave function. */
   double get_tolerance() const
   {
      return tol;
   }

   /*! @brief Get the name of the sine wave object.
    *  @return A constant pointer to the name of the sine wave object. */
   char const *get_name() const
   {
      return name;
   }

   /*! @brief Set the name of the sine wave object.
    *  @param new_name The name of the sine wave object. */
   void set_name( char const *new_name );

   //
   // Public utility functions.
   //
   /*! @brief Utility function to copy data from source to this object.
    *  @param orig Sine wave data object to copy into. */
   void copy_data( SineData const *orig );

   //
   // Public modeling functions.
   //
   /*! @brief Computes the value of the test data. */
   void compute_value();

   /*! @brief Computes the value of the test data.
    *  @param t Current model time. */
   void compute_value( double const t );

   /*! @brief Computes the derivative of the test data. */
   void compute_derivative();

   /*! @brief Computes the derivative of the test data.
    *  @param t Current integration time. */
   void compute_derivative( double const t );

   /*! @brief Computes the current phase offset from the current value and derivative.*/
   void adjust_phase();

   /*! @brief Computes the phase offset for the provided model time.
    *  @param t Current model time {s}. */
   void adjust_phase( double const t );

   /*! @brief Sine wave integration routine.
    *  @return Intermediate step ID. */
   int integration();
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_SINE_DATA_HH: Do NOT put anything after this line!
