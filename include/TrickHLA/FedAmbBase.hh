/*!
@file TrickHLA/FedAmbBase.hh
@ingroup TrickHLA
@brief Provides base methods for a Federate Ambassador.

@copyright Copyright 2025 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/FedAmbBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionServices.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectServices.cpp}
@trick_link_dependency{../../source/TrickHLA/SaveRestoreServices.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2025, --, Refactored into base class.}
@revs_end

*/

#ifndef TRICKHLA_FED_AMB_BASE_HH
#define TRICKHLA_FED_AMB_BASE_HH

// System includes.
#include <set>
#include <string>

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class InteractionServices;
class ObjectServices;
class SaveRestoreServices;

class FedAmbBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__FedAmbBase();

  public:
   /*! @brief Default constructor for the TrickHLA FedAmb class.
    *  @param fed Associated Federate instance. */
   explicit FedAmbBase( Federate &fed );

   /*! @brief Destructor for the TrickHLA FedAmb class. */
   virtual ~FedAmbBase()
   {
      return;
   }

   /*! @brief Switch to echo (versus process) in a federationRestoreStatusResponse() callback... */
   // void set_federation_restore_status_response_to_echo()
   //{
   //    this->federation_restore_status_response_context_switch = true;
   // }
   /*! @brief Switch to process (versus echo) in a federationRestoreStatusResponse() callback... */
   // void set_federation_restore_status_response_to_process()
   //{
   //    this->federation_restore_status_response_context_switch = false;
   // }

   /*! @brief Enable the option to rebuild the federate handle set after a federation restore. */
   void set_federation_restored_rebuild_federate_handle_set()
   {
      this->federation_restored_rebuild_federate_handle_set = true;
   }
   /*! @brief Disable the option to rebuild the federate handle set after a federation restore. */
   void reset_federation_restored_rebuild_federate_handle_set()
   {
      this->federation_restored_rebuild_federate_handle_set = false;
   }

  protected:
   Federate            *federate;             ///< @trick_units{--} Associated TrickHLA::Federate.
   ObjectServices      *object_service;       ///< @trick_units{--} Associated TrickHLA::ObjectServices.
   InteractionServices *interaction_service;  ///< @trick_units{--} Associated TrickHLA::InteractionServices.
   SaveRestoreServices *save_restore_service; ///< @trick_units{--} Associated TrickHLA::SaveRestoreServices.

   // bool federation_restore_status_response_context_switch;
   bool federation_restored_rebuild_federate_handle_set;

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Don't allow default constructor. */
   FedAmbBase();
   /*! @brief Copy constructor for FedAmb class.
    *  @details This constructor is private to prevent inadvertent copies. */
   FedAmbBase( FedAmbBase const &rhs );
   /*! @brief Assignment operator for FedAmb class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   FedAmbBase &operator=( FedAmbBase const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_FED_AMB_BASE_HH -- Do NOT put anything after this line.
