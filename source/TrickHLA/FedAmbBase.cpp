/*!
@file TrickHLA/FedAmbBase.cpp
@ingroup TrickHLA
@brief Provides base methods called by the Fed Ambassador.

@copyright Copyright 2026 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{FedAmbBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{InteractionServices.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{SaveRestoreServices.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Created base class.}
@revs_end

*/

// TrickHLA includes.
#include "TrickHLA/FedAmbBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/InteractionServices.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/SaveRestoreServices.hh"

using namespace TrickHLA;

/*!
 * @details Construct the base Federate Ambassador given the Federate reference.
 * @job_class{initialization}
 */
FedAmbBase::FedAmbBase( Federate &fed )
   : federate( &fed ),
     object_service( fed.get_object_service() ),
     interaction_service( fed.get_interaction_service() ),
     save_restore_service( fed.get_save_restore_service() ),
     // FIXME:
     // federation_restore_status_response_context_switch( false ), // process, not echo.
     federation_restored_rebuild_federate_handle_set( false )
{
   return;
}
