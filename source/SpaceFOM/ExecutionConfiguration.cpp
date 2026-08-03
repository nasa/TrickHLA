/*!
@file SpaceFOM/ExecutionConfiguration.cpp
@ingroup SpaceFOM
@brief Implementation of the TrickHLA SpaceFOM Execution Configuration Object (ExCO).

\par<b>Assumptions and Limitations:</b>
- One and only one ExecutionConfiguration object should exist in an federation
execution.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../TrickHLA/ObjectServices.cpp}
@trick_link_dependency{../TrickHLA/ObjectDeletedHandler.cpp}
@trick_link_dependency{../TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{../TrickHLA/time/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/utils/SleepTimeout.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2007, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SpaceFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System includes.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/attributes.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/reference.h"

// SpaceFOM includes.
#include "SpaceFOM/ExecutionConfiguration.hh"
#include "SpaceFOM/Types.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/CTETimelineBase.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/ScenarioTimeline.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

#ifdef __cplusplus
extern "C" {
#endif
// Include the Trick generated ATTRIBUTES for the ExecutionConfiguration class.
// This is used to set up Trick based simulation variable mapping into the
// TrickHLA::Object associated with this class.
extern ATTRIBUTES attrSpaceFOM__ExecutionConfiguration[]; // NOLINT(bugprone-reserved-identifier)

#ifdef __cplusplus
}
#endif

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration()
   : TrickHLA::ExecutionConfigurationBase(),
     root_frame_name(),
     scenario_time_epoch( std::numeric_limits< double >::lowest() ),
     next_mode_scenario_time( std::numeric_limits< double >::lowest() ),
     next_mode_cte_time( std::numeric_limits< double >::lowest() ),
     current_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     next_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     least_common_time_step( std::numeric_limits< long long >::lowest() )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration(
   string const &s_define_name )
   : TrickHLA::ExecutionConfigurationBase( s_define_name ),
     root_frame_name(),
     scenario_time_epoch( std::numeric_limits< double >::lowest() ),
     next_mode_scenario_time( std::numeric_limits< double >::lowest() ),
     next_mode_cte_time( std::numeric_limits< double >::lowest() ),
     current_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     next_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     least_common_time_step( std::numeric_limits< long long >::lowest() )
{
   return;
}

/*!
 * @details Even though this is a pure virtual destructor, we provide a
 * default implementation that can be called from an inheriting class.
 * @job_class{shutdown}
 */
ExecutionConfiguration::~ExecutionConfiguration() // RETURN: -- None.
{
   return;
}

/*!
 * @details These can be overridden in the input.py file.
 * @job_class{default_data}
 */
void ExecutionConfiguration::configure_attributes()
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( S_define_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::configure_attributes():" << __LINE__
             << " ERROR: Unexpected empty S_define_name.\n";
      DebugHandler::terminate( errmsg.str() );
   }

   //
   // Assign an empty root frame name to start with.
   // This will be reset at root frame discovery. It can
   // also be specified in the input.py file for the Root Reference
   // Frame Publisher (RRFP).
   //
   this->root_frame_name = "";

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   this->FOM_name = "ExecutionConfiguration";
   this->name     = "ExCO";
   this->packing  = this;
   // Allocate the attributes for the ExCO HLA object.
   this->attr_count = 7;
   this->attributes = MemoryServices::declare_var( this->attributes, this->attr_count );

   //
   // Specify the ExCO attributes.
   //
   this->attributes[0].FOM_name     = "root_frame_name";
   this->attributes[0].trick_name   = S_define_name + string( ".root_frame_name" );
   this->attributes[0].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[0].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[1].FOM_name     = "scenario_time_epoch";
   this->attributes[1].trick_name   = S_define_name + string( ".scenario_time_epoch" );
   this->attributes[1].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[2].FOM_name     = "next_mode_scenario_time";
   this->attributes[2].trick_name   = S_define_name + string( ".next_mode_scenario_time" );
   this->attributes[2].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[2].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[3].FOM_name     = "next_mode_cte_time";
   this->attributes[3].trick_name   = S_define_name + string( ".next_mode_cte_time" );
   this->attributes[3].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[3].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[4].FOM_name     = "current_execution_mode";
   this->attributes[4].trick_name   = S_define_name + string( ".current_execution_mode" );
   this->attributes[4].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[4].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[5].FOM_name     = "next_execution_mode";
   this->attributes[5].trick_name   = S_define_name + string( ".next_execution_mode" );
   this->attributes[5].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[5].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[6].FOM_name     = "least_common_time_step";
   this->attributes[6].trick_name   = S_define_name + string( ".least_common_time_step" );
   this->attributes[6].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[6].rti_encoding = ENCODING_BIG_ENDIAN;
}

/*!
 * @job_class{initialization}
 */
void ExecutionConfiguration::configure()
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Federate.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Clear out the existing object instance name, because we are going to
   // make sure it is ExCO regardless of what the user set it to be.
   this->name = "ExCO";

   // Lag compensation is not supported for the Execution Configuration object.
   set_lag_compensation_type( LAG_COMPENSATION_NONE );

   // Ownership transfer will not be used for the Execution Configuration object.
   this->ownership = NULL;

   // Make sure the ExecutionConfiguration attributes go out in
   // Receive-Order so that a late joining federate can get them.
   reset_preferred_order();
}

/*!
@details This function is called before the data is sent to the RTI.
*/
void ExecutionConfiguration::pack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration::pack():" << __LINE__ << "\n"
          << "      Current Scenario Time: " << StringUtilities::format_time( execution_control->scenario_timeline->get_time() ) << "\n"
          << "    Current Simulation Time: " << StringUtilities::format_time( the_exec->get_sim_time() ) << "\n"
          << "   Current HLA Granted Time: " << StringUtilities::format_time( federate->get_granted_time() ) << "\n"
          << " Current HLA Requested Time: " << StringUtilities::format_time( federate->get_requested_time() ) << "\n"
          << ".............................................................\n"
          << "                Object-Name: '" << get_name() << "'\n"
          << "            root_frame_name: '" << root_frame_name << "'\n"
          << "        scenario_time_epoch: " << StringUtilities::format_time( scenario_time_epoch ) << "\n"
          << "    next_mode_scenario_time: " << StringUtilities::format_time( next_mode_scenario_time ) << "\n"
          << "         next_mode_cte_time: " << StringUtilities::format_time( next_mode_cte_time ) << "\n";
      if ( execution_control->does_cte_timeline_exist() ) {
         msg << "           current-cte-time: " << StringUtilities::format_time( execution_control->cte_timeline->get_time() ) << "\n";
      } else {
         msg << "           current-cte-time: Not Enabled\n";
      }
      msg << "     current_execution_mode: " << execution_mode_enum_to_string( execution_mode_int16_to_enum( current_execution_mode ) ) << "\n"
          << "        next_execution_mode: " << execution_mode_enum_to_string( execution_mode_int16_to_enum( next_execution_mode ) ) << "\n"
          << "     least_common_time_step: " << least_common_time_step << " " << Int64BaseTime::get_base_unit() << "\n";
      if ( this->next_execution_mode == EXECUTION_MODE_FREEZE ) {
         msg << "     simulation_freeze_time: " << execution_control->get_simulation_freeze_time() << " seconds\n";
      }
      msg << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   if ( ( federate != NULL ) && !federate->verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::pack():" << __LINE__
             << " ERROR: Invalid time constraints!\n";
      DebugHandler::terminate( errmsg.str() );
   }
}

/*!
@details This function is called after data is received from the RTI.
*/
void ExecutionConfiguration::unpack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__ << "\n"
          << "      Current Scenario Time: " << StringUtilities::format_time( execution_control->scenario_timeline->get_time() ) << "\n"
          << "    Current Simulation Time: " << StringUtilities::format_time( the_exec->get_sim_time() ) << "\n"
          << "   Current HLA Granted Time: " << StringUtilities::format_time( federate->get_granted_time() ) << "\n"
          << " Current HLA Requested Time: " << StringUtilities::format_time( federate->get_requested_time() ) << "\n"
          << ".............................................................\n"
          << "                Object-Name: '" << get_name() << "'\n"
          << "            root_frame_name: '" << root_frame_name << "'\n"
          << "        scenario_time_epoch: " << StringUtilities::format_time( scenario_time_epoch ) << "\n"
          << "    next_mode_scenario_time: " << StringUtilities::format_time( next_mode_scenario_time ) << "\n"
          << "         next_mode_cte_time: " << StringUtilities::format_time( next_mode_cte_time ) << "\n";
      if ( execution_control->does_cte_timeline_exist() ) {
         msg << "           current-cte-time: " << StringUtilities::format_time( execution_control->cte_timeline->get_time() ) << "\n";
      } else {
         msg << "           current-cte-time: Not Enabled\n";
      }
      msg << "     current_execution_mode: " << execution_mode_enum_to_string( execution_mode_int16_to_enum( current_execution_mode ) ) << "\n"
          << "        next_execution_mode: " << execution_mode_enum_to_string( execution_mode_int16_to_enum( next_execution_mode ) ) << "\n"
          << "     least_common_time_step: " << least_common_time_step << " " << Int64BaseTime::get_base_unit() << "\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   verify_ExCO_data();

   // Mark that we have an ExCO update with pending changes.
   this->pending_update = true;
}

/*! @brief Verify the ExCO data is valid. */
void ExecutionConfiguration::verify_ExCO_data()
{
   if ( execution_control->does_cte_timeline_exist()
        && ( next_mode_cte_time <= std::numeric_limits< double >::lowest() ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::verify_ExCO_data():" << __LINE__
             << " ERROR: Execution Configuration has an invalid next mode"
             << " CTE time of " << next_mode_cte_time << "! Please make sure"
             << " all your Central Timing Equipment is using the same"
             << " synchronized time source.\n";
      DebugHandler::terminate( errmsg.str() );
   }
}

void ExecutionConfiguration::set_root_frame_name(
   string const &name )
{
   // Duplicate the new root reference frame name.
   this->root_frame_name = string( name );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_scenario_time_epoch(
   double scenario_time )
{
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->scenario_time_epoch = scenario_time;
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_next_mode_scenario_time(
   double next_mode_time )
{
   // TODO: Need more checking here.
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->next_mode_scenario_time = next_mode_time;
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_next_mode_cte_time(
   double cte_time )
{
   // TODO: Need more checking here.
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->next_mode_cte_time = cte_time;
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_current_execution_mode(
   short mode )
{
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->current_execution_mode = mode;
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_current_execution_mode(
   SpaceFOM::ExecutionModeEnum mode )
{
   // WARNING: Only the Master federate should ever set this.
   set_current_execution_mode( SpaceFOM::execution_mode_enum_to_int16( mode ) );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_next_execution_mode(
   short mode )
{
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->next_execution_mode = mode;
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_next_execution_mode(
   SpaceFOM::ExecutionModeEnum mode )
{
   // WARNING: Only the Master federate should ever set this.
   set_next_execution_mode( SpaceFOM::execution_mode_enum_to_int16( mode ) );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_least_common_time_step(
   double const lcts )
{
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->least_common_time_step = Int64BaseTime::to_base_time( lcts );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         ostringstream msg;
         msg << "SpaceFOM::ExecutionConfiguration::set_least_common_time_step():" << __LINE__
             << " WARNING: This is not a Master federate so this setting will be ignored."
             << "\n";
         message_publish( MSG_WARNING, msg.str().c_str() );
      }
   }
}

/*!
 * @details WARNING: This function is BROKEN!
 */
void ExecutionConfiguration::setup_ref_attributes(
   Packing *packing_obj )
{
   ostringstream errormsg;
   errormsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
            << " ERROR: This routine does NOT work and should not be called!\n";
   DebugHandler::terminate( errormsg.str() );

   //
   // Set up object properties specifically for the ExCO.
   //
   // Set original data changed flag to false.
   this->data_changed = false;

   // Set up the fixed ExCO naming.
   this->name          = "ExCO";
   this->name_required = true;
   this->FOM_name      = "SpaceFOM::ExecutionConfiguration";

   // Create the ExCO instance only if the SpaceFOM Master federate.
   if ( execution_control->is_master() ) {
      this->create_HLA_instance = true;
   } else {
      this->create_HLA_instance = false;
   }

   // All SpaceFOM compliant federates MUST have an ExCO
   this->required = true;

   // Do not block waiting for an ExCO update in normal cyclic data reads.
   this->blocking_cyclic_read = false;

   // There's no Lag Compensation with the ExCO.
   this->lag_comp      = (LagCompensation *)NULL;
   this->lag_comp_type = LAG_COMPENSATION_NONE;

   // Need to set the packing object.
   this->packing = packing_obj;

   // No ownership transfer of the ExCO. Only the master can own this.
   this->ownership = (OwnershipHandler *)NULL;

   // No Object Deleted callback.
   this->deleted                         = (ObjectDeletedHandler *)NULL;
   this->process_object_deleted_from_RTI = false;
   this->object_deleted_from_RTI         = false;

   // Set up attributes.
   this->attr_count = 7;
   this->attributes = MemoryServices::declare_var( this->attributes, this->attr_count );
   if ( this->attributes == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the attributes of the ExCO!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   //
   // Specify the ExCO attributes.
   //
   // Setup the "root_frame_name" attribute.
   this->attributes[0].FOM_name = "root_frame_name";
   if ( execution_control->is_master() ) {
      this->attributes[0].publish       = true;
      this->attributes[0].subscribe     = false;
      this->attributes[0].locally_owned = true;
   } else {
      this->attributes[0].publish       = false;
      this->attributes[0].subscribe     = true;
      this->attributes[0].locally_owned = false;
   }
   this->attributes[0].config       = CONFIG_INTERMITTENT;
   this->attributes[0].rti_encoding = ENCODING_UNICODE_STRING;

   // Normally, we would specify the Trick 'name' of the simulation
   // variable. However, this will be replaced with a direct construction
   // of the Trick REF2 ATTRIBUTES for the associated variable in memory.
   // this->attributes[0].trick_name = S_define_name + string( ".root_frame_name" );

   // Normally we would use the Trick variable to resolve to at run time,
   // which is supplied by the input.py file. Instead, we must build the
   // Trick REF2 data structures with sufficient information for the
   // Attribute class to link itself into Execution Configuration
   // instance variables.

   // TODO: Determine if exco_ref2 is needed and used.
   // Allocate the Trick REF2 data structure.
   REF2 *exco_ref2 = reinterpret_cast< REF2 * >( malloc( sizeof( REF2 ) ) );
   if ( exco_ref2 == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the REF2 structure for"
             << " the 'root_frame_name' value of the ExCO!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Allocate the Trick ATTRIBUTES data structure with room for two
   // entries: 1) the 'root_frame_name' parameter and 2) an empty entry
   // marking the end of the structure.
   ATTRIBUTES *exco_attr = reinterpret_cast< ATTRIBUTES * >( malloc( 2 * sizeof( ATTRIBUTES ) ) );
   if ( exco_attr == NULL ) {
      free( static_cast< void * >( exco_ref2 ) );
      exco_ref2 = NULL;

      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the ATTRIBUTES for the"
             << " 'root_frame_name' value of the ExCO!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Find the 'root_frame_name' value in the ExCO ATTRIBUTES.
   // since we may not know the total # of elements, we look for an empty
   // element as an ending marker of the ATTRIBUTES.
   int attr_index = 0;

   // Loop until the current ATTRIBUTES name is a NULL string
   while ( strcmp( attrSpaceFOM__ExecutionConfiguration[attr_index].name, "" ) != 0 ) {
      if ( strcmp( attrSpaceFOM__ExecutionConfiguration[attr_index].name, "root_frame_name" ) == 0 ) {
         memcpy( &exco_attr[0], // flawfinder: ignore
                 &attrSpaceFOM__ExecutionConfiguration[attr_index],
                 sizeof( ATTRIBUTES ) );
      }
      ++attr_index;
   }

   // Now that we have hit the end of the ATTRIBUTES array, copy the last
   // entry into my exco_attr array to make it a valid ATTRIBUTE array.
   memcpy( &exco_attr[1], // flawfinder: ignore
           &attrSpaceFOM__ExecutionConfiguration[attr_index],
           sizeof( ATTRIBUTES ) );

   // Initialize the attribute.
   attributes[0].initialize( get_FOM_name(), 0, 0 ); // NOLINT

   // Initialize the TrickHLA Attribute. Since we built the attributes
   // in-line, and not via the Trick input.py file, use the alternate version of
   // the initialize routine which does not resolve the fully-qualified Trick
   // name to access the ATTRIBUTES if the trick variable...
   // attributes[0].initialize( this->FOM_name,
   //                           &(this->root_frame_name),
   //                           (ATTRIBUTES *) exco_attr );

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionConfiguration::setup_interaction_ref_attributes():" << __LINE__
          << " FOM-Parameter:'" << attributes[0].get_FOM_name() << "'"
          << " NOTE: This is an auto-generated parameter so there is no"
          << " associated 'Trick-Name'.\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
          << "\n"
          << "--------------- Trick REF-Attributes ---------------"
          << "\n"
          << " Object FOM name:'" << this->FOM_name << "'\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   free( static_cast< void * >( exco_ref2 ) );
   exco_ref2 = NULL;
}

void ExecutionConfiguration::print_execution_configuration() const
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration::print_exec_config():" << __LINE__ << "\n"
          << "             Object-Name: '" << get_name() << "'\n"
          << "         root_frame_name: '" << root_frame_name << "'\n"
          << "     scenario_time_epoch: " << StringUtilities::format_time( scenario_time_epoch ) << "\n"
          << " next_mode_scenario_time: " << StringUtilities::format_time( next_mode_scenario_time ) << "\n"
          << "      next_mode_cte_time: " << StringUtilities::format_time( next_mode_cte_time ) << "\n";
      if ( execution_control->does_cte_timeline_exist() ) {
         msg << "        current-cte-time: " << StringUtilities::format_time( execution_control->cte_timeline->get_time() ) << "\n";
      } else {
         msg << "        current-cte-time: Not Enabled\n";
      }
      msg << "  current_execution_mode: " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( current_execution_mode ) ) << "\n"
          << "     next_execution_mode: " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( next_execution_mode ) ) << "\n"
          << "  least_common_time_step: " << least_common_time_step << " " << Int64BaseTime::get_base_unit() << "\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

bool ExecutionConfiguration::wait_for_update() // RETURN: -- None.
{

   // We can only receive the exec-configuration if we are not the master.
   if ( execution_control->is_master() ) {
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Waiting...\n",
                       __LINE__ );
   }

   // Make sure we have at least one piece of exec-config data we can receive.
   if ( any_remotely_owned_subscribed_init_attribute() ) {

      SleepTimeout print_timer;
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !is_changed() ) {

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionConfiguration::wait_for_update():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Received data.\n",
                          __LINE__ );
      }

      // Receive the exec-config data from the master federate.
      receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::wait_for_update():" << __LINE__
             << " ERROR: Execution-Configuration"
             << " is not configured to receive at least one object attribute."
             << " Make sure at least one 'exec_config' attribute has"
             << " 'subscribe = true' set. Please check your input or modified-data"
             << " files to make sure the 'subscribe' value is correctly specified.\n";
      DebugHandler::terminate( errmsg.str() );
   }

   return true;
}
