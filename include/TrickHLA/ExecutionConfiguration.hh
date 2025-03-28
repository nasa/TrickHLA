/*!
@file TrickHLA/ExecutionConfiguration.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA simple Execution Configuration Object (ExCO).

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

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionConfiguration.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, SISO TrickHLA Initialization.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, TrickHLA support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_EXECUTION_CONFIGURATION_HH
#define TRICKHLA_EXECUTION_CONFIGURATION_HH

// System include files.
#include <cstdint>

// TrickHLA include files.
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class ExecutionConfiguration : public TrickHLA::ExecutionConfigurationBase
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ExecutionConfiguration();

  public:
   double  run_duration;           ///< @trick_units{s}  The run duration of the simulation.
   int64_t run_duration_base_time; ///< @trick_units{us} The run duration in base HLA Logical Time.

   int   num_federates;      ///< @trick_units{--} Number of required federates.
   char *required_federates; ///< @trick_units{--} Comma-separated list of required federates.

   char *owner; ///< @trick_units{--} Federate's name publishing the object.

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the TrickHLA ExecutionConfiguration class. */
   ExecutionConfiguration();
   /*! @brief Initialization constructor for the TrickHLA ExecutionConfiguration class.
    *  @param s_define_name Full path name in the S_define for this ExecutionConfiguration instance. */
   explicit ExecutionConfiguration( char const *s_define_name );
   /*! @brief Pure virtual destructor for the TrickHLA ExecutionConfiguration class. */
   virtual ~ExecutionConfiguration();

   // Default data.
   /*! @brief Sets up the attributes for the ExCO using default values.
    *  These can be overridden in the input file. */
   virtual void configure_attributes();

   /*! @brief Configure the execution configuration object. */
   virtual void configure();

   // From the TrickHLA::Packing class.
   virtual void pack();
   virtual void unpack();

   // Execution configuration specific functions.
   /*! @brief Setup the Trick Ref Attributes for the ExecutionConfiguration object.
    *  @param packing_obj Associated packing object. */
   virtual void setup_ref_attributes( Packing *packing_obj );
   /*! @brief Print the current Execution Configuration object to the console. */
   virtual void print_execution_configuration() const;

  private:
   // Do not allow the copy constructor or assignment operator.
   ExecutionConfiguration( ExecutionConfiguration const &rhs );
   ExecutionConfiguration &operator=( ExecutionConfiguration const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_EXECUTION_CONFIGURATION_HH: Do NOT put anything after this line!
