/*!
@ingroup Sine
@file models/sine/include/SineLagCompensation.hh
@brief Send and receiving side lag compensation.

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
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{sine/src/SineData.cpp}
@trick_link_dependency{sine/src/SineLagCompensation.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2006, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2023, --, Added lag-comp bypass functions.}
@revs_end

*/

#ifndef TRICKHLA_MODLE_SINE_LAG_COMPENSATION_HH
#define TRICKHLA_MODLE_SINE_LAG_COMPENSATION_HH

// System include files.
#include <string>

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "SineData.hh"

namespace TrickHLAModel
{

class SineLagCompensation : public SineData, public TrickHLA::LagCompensation
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineLagCompensation();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineLagCompensation class. */
   SineLagCompensation();
   /*! @brief Destructor for the TrickHLAModel SineLagCompensation class. */
   virtual ~SineLagCompensation();

   /*! @brief Configure the LagCompensation object.
    *  @param sim_data The sine wave data object. */
   void configure( SineData *sim_data );

   /*! @brief Initialize the LagCompensation object. */
   void initialize();

   //
   // From the TrickHLALag::Compensation class.
   //
   /*! @brief Initialization callback as part of the TrickHLA::LagCompensation functions.
    *  @param obj TrickHLA Object associated with this LagCompensation class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Send side lag-compensation where we propagate the sine wave state
    *  head by dt to predict the value at the next data cycle. */
   virtual void send_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the send side lag compensation and your implementation must copy
    * the sim-data to the lag-comp data to effect the bypass. */
   virtual void bypass_send_lag_compensation();

   /*! @brief Receive side lag-compensation where we propagate the sine wave
    *  state ahead by dt to predict the value at the next data cycle. */
   virtual void receive_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the receive side lag compensation and your implementation must
    * copy the lag-comp data to the sim-data to effect the bypass. */
   virtual void bypass_receive_lag_compensation();

  private:
   SineData *sim_data; ///< @trick_units{--} Simulation data.

   TrickHLA::Attribute *time_attr;  ///< @trick_io{**} Reference to the "Time" TrickHLA::Attribute.
   TrickHLA::Attribute *value_attr; ///< @trick_io{**} Reference to the "Value" TrickHLA::Attribute.
   TrickHLA::Attribute *dvdt_attr;  ///< @trick_io{**} Reference to the "dvdt" TrickHLA::Attribute.
   TrickHLA::Attribute *phase_attr; ///< @trick_io{**} Reference to the "Phase" TrickHLA::Attribute.
   TrickHLA::Attribute *freq_attr;  ///< @trick_io{**} Reference to the "Frequency" TrickHLA::Attribute.
   TrickHLA::Attribute *amp_attr;   ///< @trick_io{**} Reference to the "Amplitude" TrickHLA::Attribute.
   TrickHLA::Attribute *tol_attr;   ///< @trick_io{**} Reference to the "Tolerance" TrickHLA::Attribute.
   TrickHLA::Attribute *name_attr;  ///< @trick_io{**} Reference to the "Name" TrickHLA::Attribute.

   std::string lag_comp_type_str; ///< @trick_units{--} Type of lag compensation as a string.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SineLagCompensation class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SineLagCompensation( SineLagCompensation const &rhs );
   /*! @brief Assignment operator for SineLagCompensation class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SineLagCompensation &operator=( SineLagCompensation const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODLE_SINE_LAG_COMPENSATION_HH: Do NOT put anything after this line!
