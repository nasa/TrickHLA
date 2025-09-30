/*!
@ingroup Sine
@file models/sine/include/SineConditional.hh
@brief Implements the Conditional API to conditionally send attributes.

@copyright Copyright 2023 United States Government as represented by the
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
@trick_link_dependency{../../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{sine/src/SineConditional.cpp}
@trick_link_dependency{sine/src/SineData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, November 2023, --, Using updated Conditional API.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_SINE_CONDITIONAL_HH
#define TRICKHLA_MODEL_SINE_CONDITIONAL_HH

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/Object.hh"

// Model include files.
#include "SineData.hh"

namespace TrickHLAModel
{

class SineConditional : public SineData, public TrickHLA::Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineConditional();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineConditional class. */
   SineConditional();
   /*! @brief Destructor for the TrickHLAModel SineConditional class. */
   virtual ~SineConditional();

   /*! @brief Set the sim_datad.
    *  @param data External simulation data. */
   void set_sim_data( SineData *data );

   /*! @brief Initializes conditional object. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Conditional functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Determines if the attribute value should be sent.
    *  @return True if attribute value should be sent.
    *  @param attr Attribute to check. */
   virtual bool should_send( TrickHLA::Attribute *attr );

  protected:
   SineData *sim_data; ///< @trick_units{--} pointer to the data to reflect in this cycle

   TrickHLA::Attribute *time_attr;  ///< @trick_io{**} Reference to the "Time" TrickHLA::Attribute.
   TrickHLA::Attribute *value_attr; ///< @trick_io{**} Reference to the "Value" TrickHLA::Attribute.
   TrickHLA::Attribute *dvdt_attr;  ///< @trick_io{**} Reference to the "dvdt" TrickHLA::Attribute.
   TrickHLA::Attribute *phase_attr; ///< @trick_io{**} Reference to the "Phase" TrickHLA::Attribute.
   TrickHLA::Attribute *freq_attr;  ///< @trick_io{**} Reference to the "Frequency" TrickHLA::Attribute.
   TrickHLA::Attribute *amp_attr;   ///< @trick_io{**} Reference to the "Amplitude" TrickHLA::Attribute.
   TrickHLA::Attribute *tol_attr;   ///< @trick_io{**} Reference to the "Tolerance" TrickHLA::Attribute.
   TrickHLA::Attribute *name_attr;  ///< @trick_io{**} Reference to the "Name" TrickHLA::Attribute.
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_SINE_CONDITIONAL_HH: Do NOT put anything after this line!
