/*!
@file TrickHLA/InteractionHandler.hh
@ingroup TrickHLA
@brief This class is the abstract base class for handling HLA interactions.

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
@trick_link_dependency{../../source/TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/Parameter.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial Interaction Handler.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_INTERACTION_HANDLER_HH
#define TRICKHLA_INTERACTION_HANDLER_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "Int64Interval.hh"
#include "Int64Time.hh"
#include "Interaction.hh"
#include "Parameter.hh"
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

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Interaction;

class InteractionHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__InteractionHandler();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA InteractionHandler class. */
   InteractionHandler();
   /*! @brief Destructor for the TrickHLA InteractionHandler class. */
   virtual ~InteractionHandler();

  public:
   /*! @brief Configure the TrickHLA Interaction. */
   virtual void configure() { configured = true; }

   /*! @brief Check if the TrickHLA Interaction is configured.
    *  @param status Configuration status. */
   virtual void set_configured( bool status = true ) { configured = status; }

   /*! @brief Check if the TrickHLA Interaction is configured. */
   virtual bool is_configured() { return ( configured ); }

   // Initialize the Interaction handler.
   /*! @brief Finish the initialization of the TrickHLA Initialization handler. */
   virtual void initialize() { initialized = true; }

   /*! @brief Initializes the callback to the interaction.
    *  @param inter Associated interaction for this handler. */
   virtual void initialize_callback( Interaction *inter );

   /*! @brief Set the TrickHLA managed Interaction associated with this handler.
    *  @param inter Pointer to the associated TrickHLA Interaction. */
   virtual void set_interaction( TrickHLA::Interaction *inter );

   /*! @brief Get the TrickHLA managed Interaction associated with this handler.
    *  @return Pointer to the associated TrickHLA Interaction. */
   virtual TrickHLA::Interaction *get_interaction() { return interaction; }

   /*! @brief Sends the interaction to to RTI using Receive Order.
    *  @return True if the interaction was sent; False otherwise. */
   bool send_interaction();

   /*! @brief Sends the interaction to to RTI using Receive Order.
    *  @return True if the interaction was sent; False otherwise.
    *  @param the_user_supplied_tag Users tag. */
   bool send_interaction( RTI1516_NAMESPACE::VariableLengthData const &the_user_supplied_tag );

   /*! @brief Sends the interaction to to RTI using Timestamp Order.
    *  @return True if the interaction was sent; False otherwise.
    *  @param send_HLA_time User specified HLA logical time to send the interaction. */
   bool send_interaction( double send_HLA_time );

   /*! @brief Sends the interaction to to RTI using Timestamp Order.
    *  @return True if the interaction was sent; False otherwise.
    *  @param send_HLA_time User specified HLA logical time to send the interaction.
    *  @param the_user_supplied_tag Users tag. */
   bool send_interaction( double send_HLA_time, RTI1516_NAMESPACE::VariableLengthData const &the_user_supplied_tag );

   /*! @brief Return a copy of the interactions lookahead time.
    *  @return A copy of the federation lookahead time. */
   Int64Interval get_lookahead() const;

   /*! @brief Return a copy of the granted HLA logical time.
    *  @return A copy of the federation granted time. */
   Int64Time get_granted_time() const;

   /*! @brief Returns the current simulation time.
    *  @return Current simulation time. */
   double get_sim_time();

   /*! @brief Returns the current scenario time.
    *  @return Current scenario time. */
   double get_scenario_time();

   /*! @brief Returns the current Central Timing Equipment (CTE) time.
    *  @return Current CTE time. */
   double get_cte_time();

   /*! @brief Get the Parameter by FOM name.
    *  @return Parameter for the given name.
    *  @param param_FOM_name Parameter FOM name. */
   Parameter *get_parameter( std::string const &param_FOM_name );

   /*! @brief This function returns the Parameter for the given interaction FOM name.
    *  @return Parameter for the given name.
    *  @param param_FOM_name Parameter FOM name. */
   Parameter *get_parameter_and_validate( std::string const &param_FOM_name );

   //-----------------------------------------------------------------
   // This is a virtual function and must be defined by a full class.
   //-----------------------------------------------------------------
   /*! @brief Called when the interaction is received from the RTI.
    *  @param the_user_supplied_tag Users tag. */
   virtual void receive_interaction( RTI1516_NAMESPACE::VariableLengthData const &the_user_supplied_tag );

  protected:
   bool         configured;  ///< @trick_units{--} Configured status flag.
   bool         initialized; ///< @trick_units{--} Initialization status flag.
   Interaction *interaction; ///< @trick_io{**} Pointer to the TrickHLA interaction.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for InteractionHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   InteractionHandler( InteractionHandler const &rhs );
   /*! @brief Assignment operator for InteractionHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   InteractionHandler &operator=( InteractionHandler const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INTERACTION_HANDLER_HH: Do NOT put anything after this line!
