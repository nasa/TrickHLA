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
@trick_link_dependency{../TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../TrickHLA/Manager.cpp}
@trick_link_dependency{../TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/SleepTimeout.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2007, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SpaceFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <iostream>
#include <limits>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/ObjectDeleted.hh"
#include "TrickHLA/OwnershipHandler.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionConfiguration.hh"
#include "SpaceFOM/ExecutionControl.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace SpaceFOM;

#ifdef __cplusplus
extern "C" {
#endif
// Include the Trick generated ATTRIBUTES for the ExecutionConfiguration class.
// This is used to set up Trick based simulation variable mapping into the
// TrickHLA::Object associated with this class.
extern ATTRIBUTES attrSpaceFOM__ExecutionConfiguration[];
#ifdef __cplusplus
}
#endif

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration()
   : root_frame_name( NULL ),
     scenario_time_epoch( -std::numeric_limits< double >::max() ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     current_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     next_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     least_common_time_step( -1 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration(
   const char *s_define_name )
   : ExecutionConfigurationBase( s_define_name ),
     root_frame_name( NULL ),
     scenario_time_epoch( -std::numeric_limits< double >::max() ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     current_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     next_execution_mode( SpaceFOM::MTR_UNINITIALIZED ),
     least_common_time_step( -1 )
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
   // Free the allocated root reference frame name.
   if ( this->root_frame_name != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( this->root_frame_name ) ) {
         TMM_delete_var_a( this->root_frame_name );
      }
      this->root_frame_name = static_cast< char * >( NULL );
   }
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{default_data}
 */
void ExecutionConfiguration::configure_attributes()
{
   string exco_name_str;
   string trick_name_str;

   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( S_define_name == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::configure_attributes():" << __LINE__
             << " ERROR: Unexpected NULL S_define_name." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Convert the S_define_name to a string.
   exco_name_str = string( S_define_name );

   //
   // Assign an empty root frame name to start with.
   // This will be reset at root frame discovery. It can
   // also be specified in the input file for the Root Reference
   // Frame Publisher (RRFP).
   //
   this->root_frame_name = trick_MM->mm_strdup( "" );

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   this->FOM_name = trick_MM->mm_strdup( "ExecutionConfiguration" );
   this->name     = trick_MM->mm_strdup( "ExCO" );
   this->packing  = this;
   // Allocate the attributes for the ExCO HLA object.
   this->attr_count = 7;
   this->attributes = (Attribute *)TMM_declare_var_1d( "TrickHLA::Attribute", this->attr_count );

   //
   // Specify the ExCO attributes.
   //
   this->attributes[0].FOM_name     = trick_MM->mm_strdup( "root_frame_name" );
   trick_name_str                   = exco_name_str + string( ".root_frame_name" );
   this->attributes[0].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[0].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[1].FOM_name     = trick_MM->mm_strdup( "scenario_time_epoch" );
   trick_name_str                   = exco_name_str + string( ".scenario_time_epoch" );
   this->attributes[1].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[2].FOM_name     = trick_MM->mm_strdup( "next_mode_scenario_time" );
   trick_name_str                   = exco_name_str + string( ".next_mode_scenario_time" );
   this->attributes[2].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[2].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[3].FOM_name     = trick_MM->mm_strdup( "next_mode_cte_time" );
   trick_name_str                   = exco_name_str + string( ".next_mode_cte_time" );
   this->attributes[3].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[3].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[4].FOM_name     = trick_MM->mm_strdup( "current_execution_mode" );
   trick_name_str                   = exco_name_str + string( ".current_execution_mode" );
   this->attributes[4].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[4].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[5].FOM_name     = trick_MM->mm_strdup( "next_execution_mode" );
   trick_name_str                   = exco_name_str + string( ".next_execution_mode" );
   this->attributes[5].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[5].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[6].FOM_name     = trick_MM->mm_strdup( "least_common_time_step" );
   trick_name_str                   = exco_name_str + string( ".least_common_time_step" );
   this->attributes[6].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[6].rti_encoding = ENCODING_LITTLE_ENDIAN;
}

/*!
 * @job_class{initialization}
 */
void ExecutionConfiguration::configure()
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Manager." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Clear out the existing object instance name, because we are going to
   // make sure it is ExCO regardless of what the user set it to be.
   if ( name != NULL ) {
      if ( TMM_is_alloced( name ) ) {
         TMM_delete_var_a( name );
      }
      name = static_cast< char * >( NULL );
   }
   name = TMM_strdup( (char *)"ExCO" );

   // Lag compensation is not supported for the Execution Configuration object.
   set_lag_compensation_type( LAG_COMPENSATION_NONE );

   // Ownership transfer will not be used for the Execution Configuration object.
   ownership = NULL;

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
      msg << endl
          << "=============================================================" << endl
          << "SpaceFOM::ExecutionConfiguration::pack():" << __LINE__ << endl
          << "\t Current Scenario Time:   " << setprecision( 18 ) << execution_control->scenario_timeline->get_time() << endl
          << "\t Current Simulation Time: " << the_exec->get_sim_time() << endl
          << "\t Current HLA grant time:  " << get_federate()->get_granted_time().get_time_in_seconds() << endl
          << "\t Current HLA request time:" << get_federate()->get_requested_time().get_time_in_seconds() << endl
          << "............................................................." << endl
          << "\t Object-Name:             " << this->get_name() << "'" << endl
          << "\t root_frame_name:         '" << ( root_frame_name != NULL ? root_frame_name : "" ) << "'" << endl
          << "\t scenario_time_epoch:     " << setprecision( 18 ) << scenario_time_epoch << endl
          << "\t next_mode_scenario_time: " << setprecision( 18 ) << next_mode_scenario_time << endl
          << "\t next_mode_cte_time:      " << setprecision( 18 ) << next_mode_cte_time << endl
          << "\t current_execution_mode:  " << execution_mode_enum_to_string( execution_mode_int16_to_enum( current_execution_mode ) ) << endl
          << "\t next_execution_mode:     " << execution_mode_enum_to_string( execution_mode_int16_to_enum( next_execution_mode ) ) << endl
          << "\t least_common_time_step:  " << least_common_time_step << " microseconds" << endl
          << "=============================================================" << endl;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   int64_t fed_lookahead = ( get_federate() != NULL ) ? get_federate()->get_lookahead().get_time_in_micros() : 0;

   // Do a bounds check on the least-common-time-step.
   if ( least_common_time_step < fed_lookahead ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::pack():" << __LINE__
             << " ERROR: ExCO least_common_time_step (" << least_common_time_step
             << " microseconds) is not greater than or equal to this federates lookahead time ("
             << fed_lookahead << " microseconds)!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The least-common-time-step time must be an integer multiple of
   // the federate's lookahead time.
   if ( least_common_time_step % fed_lookahead != 0 ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::pack():" << __LINE__
             << " ERROR: ExCO least_common_time_step (" << least_common_time_step
             << " microseconds) is not an integer multiple of the federate lookahead time ("
             << fed_lookahead << " microseconds)!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
@details This function is called after data is received from the RTI.
*/
void ExecutionConfiguration::unpack()
{
   int64_t software_frame_usec;
   double  software_frame_sec;

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << endl
          << "=============================================================" << endl
          << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__ << endl
          << "\t Current Scenario Time:   " << setprecision( 18 ) << execution_control->scenario_timeline->get_time() << endl
          << "\t Current Simulation Time: " << the_exec->get_sim_time() << endl
          << "\t Current HLA grant time:  " << get_federate()->get_granted_time().get_time_in_seconds() << endl
          << "\t Current HLA request time:" << get_federate()->get_requested_time().get_time_in_seconds() << endl
          << "............................................................." << endl
          << "\t Object-Name:            '" << this->get_name() << "'" << endl
          << "\t root_frame_name:        '" << ( root_frame_name != NULL ? root_frame_name : "" ) << "'" << endl
          << "\t scenario_time_epoch:    " << setprecision( 18 ) << scenario_time_epoch << endl
          << "\t next_mode_scenario_time:" << setprecision( 18 ) << next_mode_scenario_time << endl
          << "\t next_mode_cte_time:     " << setprecision( 18 ) << next_mode_cte_time << endl
          << "\t current_execution_mode: " << execution_mode_enum_to_string( execution_mode_int16_to_enum( current_execution_mode ) ) << endl
          << "\t next_execution_mode:    " << execution_mode_enum_to_string( execution_mode_int16_to_enum( next_execution_mode ) ) << endl
          << "\t least_common_time_step: " << least_common_time_step << " microseconds" << endl
          << "=============================================================" << endl;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   int64_t fed_lookahead = ( get_federate() != NULL ) ? get_federate()->get_lookahead().get_time_in_micros() : 0;

   // Do a bounds check on the least-common-time-step.
   if ( least_common_time_step < fed_lookahead ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__
             << " ERROR: ExCO least_common_time_step (" << least_common_time_step
             << " microseconds) is not greater than or equal to this federates lookahead time ("
             << fed_lookahead << " microseconds)!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Our federates lookahead time must be an integer multiple of the
   // least-common-time-step time.
   if ( least_common_time_step % fed_lookahead != 0 ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__
             << " ERROR: ExCO least_common_time_step (" << least_common_time_step
             << " microseconds) is not an integer multiple of the federate lookahead time ("
             << fed_lookahead << " microseconds)!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check the Trick executive software frame.
   // It must be smaller than the ExCO LCTS or moding won't work properly.
   // It must also be an integer multiple of the ExCO LCTS.
   software_frame_sec  = exec_get_software_frame();
   software_frame_usec = Int64Interval::to_microseconds( software_frame_sec );
   if ( software_frame_usec != least_common_time_step ) {
      if ( software_frame_usec > least_common_time_step ) {
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
            ostringstream message;
            message << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__
                    << " WARNING: ExCO least_common_time_step (" << least_common_time_step
                    << " microseconds) is less than the federate software frame ("
                    << software_frame_usec << " microseconds)! Resetting the software frame ("
                    << least_common_time_step << " microseconds)!!!!" << THLA_ENDL;
            send_hs( stdout, (char *)message.str().c_str() );
         }
         software_frame_sec = Int64Interval::to_seconds( least_common_time_step );
         exec_set_software_frame( software_frame_sec );
      } else if ( least_common_time_step % software_frame_usec != 0 ) {
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
            ostringstream message;
            message << "SpaceFOM::ExecutionConfiguration::unpack():" << __LINE__
                    << " WARNING: ExCO least_common_time_step (" << least_common_time_step
                    << " microseconds) is not an integer multiple of the federate software frame ("
                    << software_frame_usec << " microseconds)! Resetting the software frame ("
                    << least_common_time_step << " microseconds)!!!!" << THLA_ENDL;
            send_hs( stdout, (char *)message.str().c_str() );
         }
         software_frame_sec = Int64Interval::to_seconds( least_common_time_step );
         exec_set_software_frame( software_frame_sec );
      } else {
         // This must mean that the ExCO Least Common Time Step (LCTS) is
         // an integer multiple of the federates software frame. So,
         // nothing really needs to be done. It's okay for the ExCO LTCS
         // to be less than the software frame as long as it is an
         // integer multiple. This will still line up with the Master
         // federate mode control timing.
      }
   }

   // Mark that we have an ExCO update with pending changes.
   this->pending_update = true;
}

void ExecutionConfiguration::set_root_frame_name(
   const char *name )
{
   // Free the Trick memory if it's already allocated.
   if ( this->root_frame_name != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( this->root_frame_name ) ) {
         TMM_delete_var_a( this->root_frame_name );
      }
      this->root_frame_name = static_cast< char * >( NULL );
   }

   // Allocate and duplicate the new root reference frame name.
   this->root_frame_name = TMM_strdup( (char *)name );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionConfiguration::set_scenario_time_epoch(
   double scenario_time )
{
   // WARNING: Only the Master federate should ever set this.
   if ( this->execution_control->is_master() ) {
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
   if ( this->execution_control->is_master() ) {
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
   if ( this->execution_control->is_master() ) {
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
   if ( this->execution_control->is_master() ) {
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
   if ( this->execution_control->is_master() ) {
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
   int64_t lcts )
{
   // TODO: Need more checking here.
   // WARNING: Only the Master federate should ever set this.
   if ( execution_control->is_master() ) {
      this->least_common_time_step = lcts;
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
            << " ERROR: This routine does NOT work and should not be called!"
            << THLA_ENDL;
   DebugHandler::terminate_with_message( errormsg.str() );

   //
   // Set up object properties specifically for the ExCO.
   //
   // Set original data changed flag to false.
   this->data_changed = false;

   // Set up the fixed ExCO naming.
   this->name          = trick_MM->mm_strdup( "ExCO" );
   this->name_required = true;
   this->FOM_name      = trick_MM->mm_strdup( "SpaceFOM::ExecutionConfiguration" );

   // Create the ExCO instance only if the SpaceFOM Master federate.
   if ( this->execution_control->is_master() ) {
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
   this->deleted                         = (ObjectDeleted *)NULL;
   this->process_object_deleted_from_RTI = false;
   this->object_deleted_from_RTI         = false;

   // Set up attributes.
   this->attr_count = 7;
   this->attributes = (Attribute *)trick_MM->declare_var(
      "Attribute", this->attr_count );
   if ( this->attributes == static_cast< Attribute * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the attributes of the ExCO!"
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   //
   // Specify the ExCO attributes.
   //
   // Setup the "root_frame_name" attribute.
   this->attributes[0].FOM_name = trick_MM->mm_strdup( "root_frame_name" );
   if ( this->execution_control->is_master() ) {
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
   // variable. However, T=this will be replaced with a direct construction
   // of the Trick REF2 ATTRIBUTES for the associated variable in memory.
   // trick_name_str = exco_name_str + string( ".root_frame_name" );
   // this->attributes[0].trick_name = trick_MM->mm_strdup( trick_name_str.c_str() );

   // Normally we would use the Trick variable to resolve to at run time,
   // which is supplied by the input file. Instead, we must build the
   // Trick REF2 data structures with sufficient information for the
   // Attribute class to link itself into Execution Configuration
   // instance variables.

   // Allocate the Trick REF2 data structure.
   REF2 *exco_ref2 = reinterpret_cast< REF2 * >( malloc( sizeof( REF2 ) ) );
   if ( exco_ref2 == static_cast< REF2 * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the REF2 structure for"
             << " the 'root_frame_name' value of the ExCO!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Allocate the Trick ATTRIBUTES data structure with room for two
   // entries: 1) the 'root_frame_name' parameter and 2) an empty entry
   // marking the end of the structure.
   ATTRIBUTES *exco_attr = reinterpret_cast< ATTRIBUTES * >( malloc( 2 * sizeof( ATTRIBUTES ) ) );
   if ( exco_attr == static_cast< ATTRIBUTES * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the ATTRIBUTES for the"
             << " 'root_frame_name' value of the ExCO!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Find the 'root_frame_name' value in the ExCO ATTRIBUTES.
   // since we may not know the total # of elements, we look for an empty
   // element as an ending marker of the ATTRIBUTES.
   int attr_index = 0;

   // Loop until the current ATTRIBUTES name is a NULL string
   while ( strcmp( attrSpaceFOM__ExecutionConfiguration[attr_index].name, "" ) != 0 ) {
      if ( strcmp( attrSpaceFOM__ExecutionConfiguration[attr_index].name, "root_frame_name" ) == 0 ) {
         memcpy( &exco_attr[0], &attrSpaceFOM__ExecutionConfiguration[attr_index], sizeof( ATTRIBUTES ) );
      }
      attr_index++;
   }

   // Now that we have hit the end of the ATTRIBUTES array, copy the last
   // entry into my exco_attr array to make it a valid ATTRIBUTE array.
   memcpy( &exco_attr[1], &attrSpaceFOM__ExecutionConfiguration[attr_index], sizeof( ATTRIBUTES ) );

   // Initialize the attribute.
   this->attributes[0].initialize( this->FOM_name, 0, 0 );

   // Initialize the TrickHLA Attribute. Since we built the attributes
   // in-line, and not via the Trick input file, use the alternate version of
   // the initialize routine which does not resolve the fully-qualified Trick
   // name to access the ATTRIBUTES if the trick variable...
   // this->attributes[0].initialize( this->FOM_name,
   //                                &(this->root_frame_name),
   //                                (ATTRIBUTES *) exco_attr );

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionConfiguration::setup_interaction_ref_attributes():" << __LINE__
          << " FOM-Parameter:'" << this->attributes[0].get_FOM_name() << "'"
          << " NOTE: This is an auto-generated parameter so there is no"
          << " associated 'Trick-Name'." << THLA_NEWLINE;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
          << endl
          << "--------------- Trick REF-Attributes ---------------"
          << endl
          << " Object FOM name:'" << this->FOM_name << "'" << THLA_NEWLINE;
      send_hs( stdout, (char *)msg.str().c_str() );
   }
}

void ExecutionConfiguration::print_execution_configuration()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << endl
          << "=============================================================" << endl
          << "SpaceFOM::ExecutionConfiguration::print_exec_config():" << __LINE__ << endl
          << "\t Object-Name:             '" << this->get_name() << "'" << endl
          << "\t root_frame_name:         '" << ( root_frame_name != NULL ? root_frame_name : "" ) << "'" << endl
          << "\t scenario_time_epoch:     " << setprecision( 18 ) << scenario_time_epoch << endl
          << "\t next_mode_scenario_time: " << setprecision( 18 ) << next_mode_scenario_time << endl
          << "\t next_mode_cte_time:      " << setprecision( 18 ) << next_mode_cte_time << endl
          << "\t current_execution_mode:  " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( current_execution_mode ) ) << endl
          << "\t next_execution_mode:     " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( next_execution_mode ) ) << endl
          << "\t least_common_time_step:  " << least_common_time_step << " microseconds" << endl
          << "=============================================================" << THLA_ENDL;
      send_hs( stdout, (char *)msg.str().c_str() );
   }
}

bool ExecutionConfiguration::wait_for_update() // RETURN: -- None.
{
   Federate *federate = get_federate();

   // We can only receive the exec-configuration if we are not the master.
   if ( this->execution_control->is_master() ) {
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      send_hs( stdout, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Waiting...%c",
               __LINE__, THLA_NEWLINE );
   }

   // Make sure we have at least one piece of exec-config data we can receive.
   if ( this->any_remotely_owned_subscribed_init_attribute() ) {

      long long    wallclock_time;
      SleepTimeout print_timer( (double)federate->wait_status_time );
      SleepTimeout sleep_timer( (long)THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !this->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         (void)sleep_timer.sleep();

         if ( !this->is_changed() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionConfiguration::wait_for_update():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               send_hs( stdout, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Waiting...%c",
                        __LINE__, THLA_NEWLINE );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         send_hs( stdout, "SpaceFOM::ExecutionConfiguration::wait_for_update():%d Received data.%c",
                  __LINE__, THLA_NEWLINE );
      }

      // Receive the exec-config data from the master federate.
      this->receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration::wait_for_update():" << __LINE__
             << " ERROR: Execution-Configuration"
             << " is not configured to receive at least one object attribute."
             << " Make sure at least one 'exec_config' attribute has"
             << " 'subscribe = true' set. Please check your input or modified-data"
             << " files to make sure the 'subscribe' value is correctly specified."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return true;
}
