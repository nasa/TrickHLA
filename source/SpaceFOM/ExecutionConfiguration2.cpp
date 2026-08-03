/*!
@file SpaceFOM/ExecutionConfiguration2.cpp
@ingroup SpaceFOM
@brief Implementation of the TrickHLA SpaceFOM Execution Configuration Object (ExCO).

\par<b>Assumptions and Limitations:</b>
- One and only one ExecutionConfiguration2 object should exist in an federation
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
@trick_link_dependency{ExecutionConfiguration2.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2007, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SpaceFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2025, --, Extended ExCO.}
@revs_end

*/

// System includes.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/attributes.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// SpaceFOM includes.
#include "SpaceFOM/ExecutionConfiguration.hh"
#include "SpaceFOM/ExecutionConfiguration2.hh"
#include "SpaceFOM/Types.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/CTETimelineBase.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/utils/StringUtilities.hh"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

#ifdef __cplusplus
extern "C" {
#endif
// Include the Trick generated ATTRIBUTES for the ExecutionConfiguration2 class.
// This is used to set up Trick based simulation variable mapping into the
// TrickHLA::Object associated with this class.
extern ATTRIBUTES attrSpaceFOM__ExecutionConfiguration2[]; // NOLINT(bugprone-reserved-identifier)

#ifdef __cplusplus
}
#endif

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration2::ExecutionConfiguration2()
   : SpaceFOM::ExecutionConfiguration(),
     hla_base_time_multiplier( Int64BaseTime::get_base_time_multiplier() )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration2::ExecutionConfiguration2(
   string const &s_define_name )
   : SpaceFOM::ExecutionConfiguration( s_define_name ),
     hla_base_time_multiplier( Int64BaseTime::get_base_time_multiplier() )
{
   return;
}

/*!
 * @details Even though this is a pure virtual destructor, we provide a
 * default implementation that can be called from an inheriting class.
 * @job_class{shutdown}
 */
ExecutionConfiguration2::~ExecutionConfiguration2() // RETURN: -- None.
{
   return;
}

/*!
 * @details These can be overridden in the input.py file.
 * @job_class{default_data}
 */
void ExecutionConfiguration2::configure_attributes()
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( S_define_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionConfiguration2::configure_attributes():" << __LINE__
             << " ERROR: Unexpected empty S_define_name.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
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
   this->FOM_name = "ExecutionConfiguration.ExecutionConfiguration2";
   this->name     = "ExCO";
   this->packing  = this;
   // Allocate the attributes for the ExCO HLA object.
   this->attr_count = 8;
   this->attributes = MemoryServices::declare_var( this->attributes, this->attr_count );

   // FIXME:
   if ( this->attributes == nullptr ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionConfiguration2::configure_attributes():" << __LINE__
          << " ERROR: Memory allocation failed.\n";
      DebugHandler::terminate( msg.str() );
      return;
   }

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

   this->attributes[7].FOM_name     = "hla_base_time_multiplier";
   this->attributes[7].trick_name   = S_define_name + string( ".hla_base_time_multiplier" );
   this->attributes[7].config       = CONFIG_INITIALIZE_AND_INTERMITTENT;
   this->attributes[7].rti_encoding = ENCODING_LITTLE_ENDIAN;
}

/*!
@details This function is called before the data is sent to the RTI.
*/
void ExecutionConfiguration2::pack()
{
   // Set the ExCO base units by converting what the time base enum is set to.
   this->hla_base_time_multiplier = Int64BaseTime::get_base_time_multiplier();

   // Call the parent implementation.
   ExecutionConfiguration::pack();

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration2::pack():" << __LINE__ << "\n"
          << "-- Extended ExCO Attributes --\n"
          << "   hla_base_time_multiplier: " << hla_base_time_multiplier
          << " " << Int64BaseTime::get_base_unit() << "\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
@details This function is called after data is received from the RTI.
*/
void ExecutionConfiguration2::unpack()
{
   // TODO: Check if we are in freeze or init to do this!
   federate->get_time_management_service()->set_HLA_base_time_multiplier_and_scale_trick_tics( get_exco_base_time_multiplier() );

   // Call the parent implementation.
   ExecutionConfiguration::unpack();

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration2::unpack():" << __LINE__ << "\n"
          << "-- Extended ExCO Attributes --\n"
          << "   hla_base_time_multiplier: " << hla_base_time_multiplier
          << " " << Int64BaseTime::get_base_unit() << "\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*! @brief Get the value of the ExCO base time multiplier.
 *  @return Base time multiplier. */
int64_t ExecutionConfiguration2::get_exco_base_time_multiplier() const
{
   return hla_base_time_multiplier;
}

void ExecutionConfiguration2::print_execution_configuration() const
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "\n"
          << "=============================================================\n"
          << "SpaceFOM::ExecutionConfiguration2::print_exec_config():" << __LINE__ << "\n"
          << "              Object-Name: '" << get_name() << "'\n"
          << "          root_frame_name: '" << root_frame_name << "'\n"
          << "      scenario_time_epoch: " << StringUtilities::format_time( scenario_time_epoch ) << "\n"
          << "  next_mode_scenario_time: " << StringUtilities::format_time( next_mode_scenario_time ) << "\n"
          << "       next_mode_cte_time: " << StringUtilities::format_time( next_mode_cte_time ) << "\n";
      if ( execution_control->does_cte_timeline_exist() ) {
         msg << "         current-cte-time: " << StringUtilities::format_time( execution_control->cte_timeline->get_time() ) << "\n";
      } else {
         msg << "         current-cte-time: Not Enabled\n";
      }
      msg << "   current_execution_mode: " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( current_execution_mode ) ) << "\n"
          << "      next_execution_mode: " << SpaceFOM::execution_mode_enum_to_string( SpaceFOM::execution_mode_int16_to_enum( next_execution_mode ) ) << "\n"
          << "   least_common_time_step: " << least_common_time_step << " " << Int64BaseTime::get_base_unit() << "\n"
          << "-- Extended ExCO Attributes --\n"
          << " hla_base_time_multiplier: " << hla_base_time_multiplier << " " << Int64BaseTime::get_base_unit() << "\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
