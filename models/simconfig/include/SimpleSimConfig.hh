/*!
@ingroup SimConfig
@file models/simconfig/include/SimpleSimConfig.hh
@brief This class contains a basic simulation configuration.

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
@trick_link_dependency{../../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{simconfig/src/SimpleSimConfig.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2007, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_SIMPLE_SIM_CONFIG_HH
#define TRICKHLA_MODEL_SIMPLE_SIM_CONFIG_HH

// System include files.
#include <cstdint>

// TrickHLA include files.
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/Packing.hh"

namespace TrickHLAModel
{

class SimpleSimConfig : public TrickHLA::Packing
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SimpleSimConfig();

  public:
   double  run_duration;           ///< @trick_units{s}  The run duration of the simulation in seconds.
   int64_t run_duration_base_time; ///< @trick_units{us} The run duration in the HLA Logical Time base.

   int   num_federates;      ///< @trick_units{--} Number of required federates.
   char *required_federates; ///< @trick_units{--} Comma-separated list of required federates.

   char *owner; ///< @trick_units{--} Federates name publishing the object.

   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SimpleSimConfig class. */
   SimpleSimConfig();
   /*! @brief Destructor for the TrickHLAModel SimpleSimConfig class. */
   virtual ~SimpleSimConfig();

   /*! @brief Configure the simulation configuration and build the list of
    * federates based on the known federates.
    *  @param known_feds_count Number of known federates.
    *  @param known_feds       Array of known federates. */
   virtual void configure( int const known_feds_count, TrickHLA::KnownFederate *known_feds );

   /*! @brief Initialize the simulation configuration and build the list of
    * federates based on the known federates. */
   virtual void initialize();

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SimpleSimConfig class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SimpleSimConfig( SimpleSimConfig const &rhs );
   /*! @brief Assignment operator for SimpleSimConfig class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SimpleSimConfig &operator=( SimpleSimConfig const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_SIMPLE_SIM_CONFIG_HH: Do NOT put anything after this line!
