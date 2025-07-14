/*!
@file SpaceFOM/MTRInteractionHandler.hh
@ingroup SpaceFOM
@brief This is the base implementation for the Space Reference FOM (SpaceFOM)
Mode Transition Request (MTR) interaction handler.

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/MTRInteractionHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_MTR_INTERACTION_HANDLER_HH
#define SPACEFOM_MTR_INTERACTION_HANDLER_HH

// System includes.
#include <cstdint>
#include <string>

// SpaceFOM includes.
#include "Types.hh"

// TrickHLA includes.
#include "../TrickHLA/InteractionHandler.hh"
#include "../TrickHLA/StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include "RTI/RTI1516.h"
#pragma GCC diagnostic pop

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
namespace TrickHLA
{
class Federate;
}

namespace SpaceFOM
{

class MTRInteractionHandler : public TrickHLA::InteractionHandler
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__MTRInteractionHandler();

  public:
   // Public constructors and destructors.
   /*! @brief Initialization constructor for the SpaceFOM MTRInteractionHandler class.
    *  @param fed  TrickHLA::Federate associated with this MTRInteractionHandler instance. */
   explicit MTRInteractionHandler( TrickHLA::Federate const *fed );
   /*! @brief Destructor for the SpaceFOM MTRInteractionHandler class. */
   virtual ~MTRInteractionHandler(); // Destructor.

  public:
   // Working send and receive functions.
   /*! @brief Send the HLA interaction.
    *  @param mode_request Requested mode transition. */
   virtual void send_interaction( MTREnum mode_request );

   /*! @brief Receive the HLA interaction.
    *  @param the_user_supplied_tag User supplied interaction tag. */
   virtual void receive_interaction( RTI1516_USERDATA const &the_user_supplied_tag );

   // Public utility functions.
   /*! @brief Set the associated name for this interaction handler.
    *  @param new_name  Associated name. */
   virtual void set_name( std::string const &new_name );

   /*! @brief Get the address of the MTR interaction mode transition state.
    *  @return Address of the MTR interaction mode transition state. */
   int16_t *get_address_of_interaction_mode()
   {
      return ( &mtr_mode_int );
   }

  public:
   std::string name; ///< @trick_units{--} Federation instance name for this interaction.

   MTREnum mtr_mode;     ///< @trick_units{--} Requested mode transition state.
   int16_t mtr_mode_int; ///< @trick_units{--} Requested mode transition state (integer version).

  protected:
   double scenario_time; ///< @trick_units{s} Scenario time when MTR was sent/received.
   double sim_time;      ///< @trick_units{s} Simulation time when MTR was sent/received.
   double cte_time;      ///< @trick_units{s} CTE time when MTR was sent/received, if CTE used.
   double granted_time;  ///< @trick_units{s} HLA granted time when MTR was sent/received.

   int send_cnt;    ///< @trick_units{count} The number of times an interaction is sent.
   int receive_cnt; ///< @trick_units{count} The number of times an interaction was received.

  private:
   // Do not allow these constructors or assignment operator.
   /*! @brief Default constructor for MTRInteractionHandler class.
    *  @details This constructor is private to prevent instantiation without
    *  an associated TrickHLA::Federate. */
   MTRInteractionHandler();
   /*! @brief Copy constructor for MTRInteractionHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   MTRInteractionHandler( MTRInteractionHandler const &rhs );
   /*! @brief Assignment operator for MTRInteractionHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   MTRInteractionHandler &operator=( MTRInteractionHandler const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_MTR_INTERACTION_HANDLER_HH: Do NOT put anything after this line!
