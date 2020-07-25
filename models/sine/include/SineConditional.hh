/*!
@file models/sine/include/SineConditional.hh
@ingroup TrickHLAModel
@brief Subclass the base class to provide sine wave-specific CONDITIONAL
attribute.

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
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{sine/src/SineConditional.cpp}
@trick_link_dependency{sine/src/SineData.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, TrickHLA, Oct 2009, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_MODEL_SINE_CONDITIONAL_HH_
#define _TRICKHLA_MODEL_SINE_CONDITIONAL_HH_

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"

// Model include files.
#include "SineData.hh"

namespace TrickHLAModel
{

class SineConditional : public TrickHLA::Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
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

   /*! @brief Initializes the sim_data to the supplied.
    *  @param data          External simulation data.
    *  @param attr_FOM_name FOM name of the attribute to track when changed.
    */
   void initialize( SineData *data, const char *attr_FOM_name );

   /*! @brief Determines if the attribute has changed and returns the truth of
    *  that determination.
    *  @return True if value should be sent.
    *  @param attr Attribute to check. */
   virtual bool should_send( TrickHLA::Attribute *attr );

  private:
   /*! @brief Determines the supplied name's position in the SineData structure.
    *  @return position in SineData.
    *  @param attr_FOM_name FOM name of the attribute. */
   int convert_FOM_name_to_pos( const char *attr_FOM_name );

   SineData *sim_data;      ///< @trick_units{--} pointer to the data to reflect in this cycle
   SineData  prev_sim_data; ///< @trick_units{--} copy of the data we previously reflected

   int attr_pos; ///< @trick_units{--} attribute position in SineData
};

} // namespace TrickHLAModel

#endif // _TRICKHLA_MODEL_SINE_CONDITIONAL_HH_: Do NOT put anything after this line!
