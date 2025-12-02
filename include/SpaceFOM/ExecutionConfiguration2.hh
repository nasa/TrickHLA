/*!
@file SpaceFOM/ExecutionConfiguration2.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM Execution Configuration Object (ExCO).

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/SpaceFOM/ExecutionConfiguration2.cpp}
@trick_link_dependency{../../source/SpaceFOM/ExecutionConfiguration.cpp}
@trick_link_dependency{../../source/SpaceFOM/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, SISO SpaceFOM Initialization.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, SpaceFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_EXECUTION_CONFIGURATION2_HH
#define SPACEFOM_EXECUTION_CONFIGURATION2_HH

// System includes.
#include <cstdint>
#include <string>

// SpaceFOM includes.
#include "SpaceFOM/ExecutionConfiguration.hh"

// TrickHLA includes.
#include "TrickHLA/Types.hh"

namespace SpaceFOM
{

class ExecutionConfiguration2 : public SpaceFOM::ExecutionConfiguration
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__ExecutionConfiguration2();

  public:
   // The members below are part of the FOM data ExCO exchange.
   int64_t hla_base_time_multiplier; /**< @trick_units{--} Base time multiplier, extended ExCO attribute. */

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM ExecutionConfiguration2 class. */
   ExecutionConfiguration2();
   /*! @brief Initialization constructor for the TrickHLA ExecutionConfiguration2 class.
    *  @param s_define_name Full path name in the S_define for this ExecutionConfiguration2 instance. */
   explicit ExecutionConfiguration2( std::string const &s_define_name );
   /*! @brief Pure virtual destructor for the SpaceFOM ExecutionConfiguration2 class. */
   virtual ~ExecutionConfiguration2();

   // Default data.
   /*! @brief Sets up the attributes for the ExCO using default values.
    *  These can be overridden in the input file. */
   virtual void configure_attributes();

   // From the TrickHLA::Packing class.
   virtual void pack();
   virtual void unpack();

   /*! @brief Get the value of the ExCO base time multiplier.
    *  @return Base time multiplier. */
   virtual int64_t const get_exco_base_time_multiplier() const;

   // ExecutionConfiguration2 specific functions.
   /*! @brief Print the current ExCO state to the console. */
   virtual void print_execution_configuration() const;

  private:
   // Do not allow the copy constructor or assignment operator.
   ExecutionConfiguration2( ExecutionConfiguration2 const &rhs );
   ExecutionConfiguration2 &operator=( ExecutionConfiguration2 const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_EXECUTION_CONFIGURATION2_HH: Do NOT put anything after this line!
