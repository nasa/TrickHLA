/*!
@file TrickHLA/time/TimeManagement.cpp
@ingroup TrickHLA
@brief This class provides basic services for HLA time management.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{TimeManagement.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{TrickThreadCoordinator.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../ExecutionControlBase.cpp}
@trick_link_dependency{../FedAmb.cpp}
@trick_link_dependency{../Manager.cpp}
@trick_link_dependency{../MutexLock.cpp}
@trick_link_dependency{../MutexProtection.cpp}
@trick_link_dependency{../SleepTimeout.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Titan Systems Corp., --, Initial investigation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/TimeManagement.hh"
#include "TrickHLA/time/TrickThreadCoordinator.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"
#include "RTI/RTIambassador.h"
#include "RTI/RTIambassadorFactory.h"
#include "RTI/time/HLAinteger64Time.h"

#if defined( IEEE_1516_2025 )
#   include "RTI/RtiConfiguration.h"
#else
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details NOTE: In most cases, we would allocate and set default names in
 * the constructor. However, since we want this class to be Input Processor
 * friendly, we cannot do that here since the Input Processor may not have
 * been initialized yet. So, we have to set the name information to NULL and
 * then allocate and set the defaults in the initialization job if not
 * already set in the input stream.
 *
 * @job_class{initialization}
 */
TimeManagement::TimeManagement(
   Federate *fed )
   : TrickThreadCoordinator( fed ),
     lookahead_time( 0.0 ),
     time_regulating( true ),
     time_constrained( true ),
     time_management( true ),
     lookahead( 0.0 ),
     HLA_cycle_time( 0.0 ),
     HLA_cycle_time_in_base_time( 0 ),
     granted_time( 0.0 ),
     requested_time( 0.0 ),
     HLA_time( 0.0 ),
     time_adv_state( TrickHLA::TIME_ADVANCE_RESET ),
     time_adv_state_mutex(),
     time_regulating_state( false ),
     time_constrained_state( false ),
     tag_wait_sum( 0 ),
     tag_wait_count( 0 )
#if defined( IEEE_1516_2010 )
     ,
     RTI_ambassador( NULL )
#endif
{
   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
TimeManagement::~TimeManagement()
{
   // Make sure we destroy the mutex.
   time_adv_state_mutex.destroy();
}

/*!
 * @brief Initialize the thread memory associated with the Trick child threads.
 */
void TimeManagement::initialize_thread_state(
   double const main_thread_data_cycle_time )
{
   this->HLA_cycle_time              = main_thread_data_cycle_time;
   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::initialize_thread_state():%d Trick main thread (id:0, data_cycle:%.3f).\n",
                       __LINE__, this->HLA_cycle_time );
   }

   // Make sure the Trick thread coordinator is initialized. This will
   // also associate the Trick main thread. TrickHLA will maintain data
   // coherency for the HLA object instances specified in the input file
   // over the data cycle time specified.
   initialize_thread_coordinator( this->HLA_cycle_time );

   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   Manager *manager = federate->get_manager();
   for ( int n = 0; n < manager->obj_count; ++n ) {
      manager->objects[n].set_core_job_cycle_time(
         Int64BaseTime::to_seconds(
            get_data_cycle_base_time_for_obj( n, get_HLA_cycle_time_in_base_time() ) ) );
   }
}

/*!
 * @job_class{initialization}
 */
void TimeManagement::restart_initialization()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::restart_initialization():%d \n",
                       __LINE__ );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Update the lookahead time in our HLA time line.
   set_lookahead( lookahead_time );

   // The lookahead time can not be negative.
   if ( lookahead_time < 0.0 ) {
      ostringstream errmsg;
      errmsg << "TimeManagement::restart_initialization():" << __LINE__
             << " ERROR: Invalid HLA lookahead time!"
             << " Lookahead time (" << lookahead_time << " seconds)"
             << " must be greater than or equal to zero and not negative. Make"
             << " sure 'lookahead_time' in your input.py or modified-data file is"
             << " not a negative number." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*! @brief Set the time advance as granted. */
void TimeManagement::set_time_advance_granted(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   Int64Time int64_time( time );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   // Ignore any granted time less than the requested time otherwise it will
   // break our concept of HLA time since we are using scheduled jobs for
   // processing HLA data sends, receives, etc and expected the next granted
   // time to match our requested time. Dan Dexter, 2/12/2007
   if ( int64_time >= this->requested_time ) {

      granted_time.set( int64_time );

      // Record the granted time in the HLA_time variable, so we can plot it
      // in Trick data products.
      this->HLA_time = granted_time.get_time_in_seconds();

      this->time_adv_state = TIME_ADVANCE_GRANTED;

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::set_time_advance_granted():%d Granted-time:%f, Requested-time:%f.\n",
                          __LINE__, this->HLA_time, requested_time.get_time_in_seconds() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "TimeManagement::set_time_advance_granted():%d WARNING: TimeManagement \"%s\" \
IGNORING GRANTED TIME %.12G because it is less then requested time %.12G.\n",
                          __LINE__, federate->get_federate_name().c_str(),
                          int64_time.get_time_in_seconds(),
                          requested_time.get_time_in_seconds() );
      }
   }
}

void TimeManagement::set_granted_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void TimeManagement::set_granted_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void TimeManagement::set_requested_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

void TimeManagement::set_requested_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

HLABaseTimeEnum TimeManagement::get_HLA_base_time_unit()
{
   return Int64BaseTime::get_base_unit_enum();
}

/*! @brief Sets the HLA base time unit.
 *  @param base_time_unit HLA base time unit. */
void TimeManagement::set_HLA_base_time_unit(
   HLABaseTimeEnum const base_time_unit )
{
   if ( base_time_unit != Int64BaseTime::get_base_unit_enum() ) {
      Int64BaseTime::set( base_time_unit );
      refresh_HLA_time_constants();
      verify_time_constraints();
   }
}

/*! @brief Sets the HLA base time unit and scale Trick tics multiplier.
 *  @param base_time_unit HLA base time unit. */
void TimeManagement::set_HLA_base_time_unit_and_scale_trick_tics(
   HLABaseTimeEnum const base_time_unit )
{
   if ( base_time_unit != Int64BaseTime::get_base_unit_enum() ) {
      Int64BaseTime::set( base_time_unit );
      refresh_HLA_time_constants();

      // Scale the Trick time tics value based on the HLA base time multiplier.
      scale_trick_tics_to_HLA_base_time_multiplier();

      verify_time_constraints();
   }
}

/*! @brief Sets the HLA base time multiplier.
 *  @param multiplier HLA base time multiplier. */
void TimeManagement::set_HLA_base_time_multiplier(
   int64_t const multiplier )
{
   if ( multiplier != Int64BaseTime::get_base_time_multiplier() ) {
      Int64BaseTime::set( multiplier );
      refresh_HLA_time_constants();
      verify_time_constraints();
   }
}

/*! @brief Sets the HLA base time multiplier and scale Trick tics multiplier.
 *  @param multiplier HLA base time multiplier. */
void TimeManagement::set_HLA_base_time_multiplier_and_scale_trick_tics(
   int64_t const multiplier )
{
   if ( multiplier != Int64BaseTime::get_base_time_multiplier() ) {
      Int64BaseTime::set( multiplier );
      refresh_HLA_time_constants();

      // Scale the Trick time tics value based on the HLA base time multiplier.
      scale_trick_tics_to_HLA_base_time_multiplier();

      verify_time_constraints();
   }
}

void TimeManagement::refresh_HLA_time_constants()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   refresh_lookahead();

   federate->get_execution_control()->refresh_least_common_time_step();

   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );

   refresh_thread_base_times();
}

void TimeManagement::scale_trick_tics_to_HLA_base_time_multiplier()
{
   int64_t const time_res  = Int64BaseTime::get_base_time_multiplier();
   int64_t       tic_value = exec_get_time_tic_value();

   // Scale up the Trick time Tic value to support the HLA base time units.
   // Trick Time Tics is limited to a value of 2^31.
   while ( ( tic_value < time_res ) && ( tic_value < std::numeric_limits< int >::max() ) ) {
      tic_value *= 10;
   }

   if ( tic_value <= std::numeric_limits< int >::max() ) {
      // Update the Trick Time Tic value only if we are increasing the resolution.
      if ( tic_value > exec_get_time_tic_value() ) {
         exec_set_time_tic_value( tic_value );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_INFO, "TimeManagement::scale_trick_tics_to_HLA_base_time_multiplier():%d New Trick time tics:%d.\n",
                             __LINE__, tic_value );
         }
      }
   } else {
      ostringstream errmsg;
      errmsg << "TimeManagement::scale_trick_tics_to_HLA_base_time_multiplier():" << __LINE__
             << " ERROR: Trick cannot represent the required time Tic value "
             << setprecision( 18 ) << time_res
             << " in order to support the HLA base units of '"
             << Int64BaseTime::get_base_unit()
             << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void TimeManagement::set_lookahead(
   double const value )
{
   // Determine if the lookahead time needs a resolution that exceeds the
   // configured HLA base time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value ) ) {
      ostringstream errmsg;
      errmsg << "TimeManagement::set_lookahead():" << __LINE__
             << " ERROR: The lookahead time specified (" << setprecision( 18 ) << value
             << " seconds) requires more resolution than whole "
             << Int64BaseTime::get_base_unit()
             << ". The HLA Logical Time is a 64-bit integer"
             << " representing " << Int64BaseTime::get_base_unit()
             << " and cannot represent a lookahead time of "
             << setprecision( 18 ) << ( value * Int64BaseTime::get_base_time_multiplier() )
             << " " << Int64BaseTime::get_base_unit() << ". You can adjust the"
             << " base HLA Logical Time resolution by setting"
             << "'federate.set_HLA_base_time_unit( "
             << Int64BaseTime::get_base_unit_enum_string( Int64BaseTime::best_base_time_resolution( value ) )
             << " )' in your input.py file. The current HLA base time resolution is "
             << Int64BaseTime::get_base_unit_enum_string( Int64BaseTime::get_base_unit_enum() )
             << ". You also need to update both the Federation Execution"
             << " Specific Federation Agreement (FESFA) and TimeManagement Compliance"
             << " Declaration (FCD) documents for your Federation to document"
             << " the change in timing class resolution." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if the Trick time Tic can represent the lookahead time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value, exec_get_time_tic_value() ) ) {
      ostringstream errmsg;
      errmsg << "TimeManagement::set_lookahead():" << __LINE__
             << " ERROR: The Trick time tic value (" << exec_get_time_tic_value()
             << ") does not have enough resolution to represent the HLA lookahead time ("
             << setprecision( 18 ) << value
             << " seconds). Please update the Trick time tic value in your"
             << " input.py file (i.e. by calling 'trick.exec_set_time_tic_value()')." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   lookahead.set( value );
   this->lookahead_time = value;
}

/*! @brief Update the HLA lookahead base time. */
void TimeManagement::refresh_lookahead()
{
   // Recalculate the lookahead HLA time in base time units.
   set_lookahead( this->lookahead_time );
}

void TimeManagement::time_advance_request_to_GALT()
{
   // Simply return if we are the master federate that created the federation,
   // or if time management is not enabled.
   if ( !this->time_management || ( federate->get_execution_control()->is_master() && !federate->get_execution_control()->is_late_joiner() ) ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time time;
      if ( RTI_ambassador->queryGALT( time ) ) {
         int64_t L = lookahead.get_base_time();
         if ( L > 0 ) {
            int64_t GALT = time.getTime();

            // Make sure the time is an integer multiple of the lookahead time.
            time.setTime( ( ( GALT / L ) + 1 ) * L );
         }
         set_requested_time( time );
      }
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: NotConnected\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::time_advance_request_to_GALT():%d Requested-Time:%lf\n",
                       __LINE__, requested_time.get_time_in_seconds() );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

void TimeManagement::time_advance_request_to_GALT_LCTS_multiple()
{
   // Simply return if we are the master federate that created the federation,
   // or if time management is not enabled.
   if ( !this->time_management
        || ( federate->get_execution_control()->is_master() && !federate->get_execution_control()->is_late_joiner() ) ) {
      return;
   }

   // Setup the Least-Common-Time-Step time value.
   int64_t LCTS = federate->get_execution_control()->get_least_common_time_step();
   if ( LCTS <= 0 ) {
      LCTS = lookahead.get_base_time();
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time time;
      if ( RTI_ambassador->queryGALT( time ) ) {
         if ( LCTS > 0 ) {
            int64_t GALT = time.getTime();

            // Make sure the time is an integer multiple of the LCTS time.
            time.setTime( ( ( GALT / LCTS ) + 1 ) * LCTS );
         }
         set_requested_time( time );
      }
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: NotConnected\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::time_advance_request_to_GALT_LCTS_multiple():%d Requested-Time:%lf\n",
                       __LINE__, requested_time.get_time_in_seconds() );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

/*!
 * @job_class{initialization}.
 */
void TimeManagement::setup_time_management()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::setup_time_management():%d time_management:%s time_regulating:%s time_constrained:%s \n",
                       __LINE__,
                       ( this->time_management ? "Yes" : "No" ),
                       ( this->time_regulating ? "Yes" : "No" ),
                       ( this->time_constrained ? "Yes" : "No" ) );
   }

   // Determine if HLA time management is enabled.
   if ( this->time_management ) {

      // Setup time constrained if the user wants to be constrained and our
      // current HLA time constrained state indicates we are not constrained.
      if ( this->time_constrained ) {
         if ( !this->time_constrained_state ) {
            setup_time_constrained();
         }
      } else {
         if ( this->time_constrained_state ) {
            // Disable time constrained if our current HLA state indicates we
            // are already constrained.
            shutdown_time_constrained();
         }
      }

      // Setup time regulation if the user wanted to be regulated and our
      // current HLA time regulating state indicates we are not regulated.
      if ( this->time_regulating ) {
         if ( !this->time_regulating_state ) {
            setup_time_regulation();
         }
      } else {
         if ( this->time_regulating_state ) {
            // Disable time regulation if our current HLA state indicates we
            // are already regulating.
            shutdown_time_regulating();
         }
      }
   } else {
      // HLA Time Management is disabled.

      // Disable time constrained and time regulation.
      if ( this->time_constrained_state ) {
         shutdown_time_constrained();
      }
      if ( this->time_regulating_state ) {
         shutdown_time_regulating();
      }
   }
}

void TimeManagement::set_time_constrained_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the debug show above to avoid a race condition
      // with the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_constrained_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::set_time_constrained_enabled():%d TimeManagement \
\"%s\" Time granted to: %.12G \n",
                       __LINE__, federate->get_federate_name().c_str(),
                       get_granted_time().get_time_in_seconds() );
   }
}

/*!
 * @job_class{initialization}.
 */
void TimeManagement::setup_time_constrained()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time constrained enabled, or if we are already constrained.
   if ( !this->time_management || !this->time_constrained || this->time_constrained_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      DebugHandler::terminate_with_message( "TimeManagement::setup_time_constrained() ERROR: NULL pointer to RTIambassador!" );
      return;
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::setup_time_constrained()%d \"%s\": ENABLING TIME CONSTRAINED \n",
                          __LINE__, federate->get_federation_name().c_str() );
      }

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state         = TIME_ADVANCE_RESET;
         this->time_constrained_state = false;
      }

      // Turn on constrained status so that regulating federates will control
      // our advancement in time.
      //
      // If we are constrained and sending federates specify the Class
      // attributes and Communication interaction with timestamp in the
      // simulation fed file we will receive TimeStamp Ordered messages.
      RTI_ambassador->enableTimeConstrained();

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->get_wait_status_time() );
      SleepTimeout sleep_timer;

      // This spin lock waits for the time constrained flag to be set from the RTI.
      while ( !this->time_constrained_state ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_constrained_state ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TimeManagement::setup_time_constrained():" << __LINE__
                         << " ERROR: Unexpectedly the TimeManagement is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TimeManagement::setup_time_constrained()%d \"%s\": ENABLING TIME CONSTRAINED, waiting...\n",
                                __LINE__, federate->get_federation_name().c_str() );
            }
         }
      }
   } catch ( RTI1516_NAMESPACE::TimeConstrainedAlreadyEnabled const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->time_constrained_state = true;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": Time Constrained Already Enabled : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: InTimeAdvancingState : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: RequestForTimeConstrainedPending : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: FederateNotExecutionMember : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TrickHLAFderate::setup_time_constrained():%d \"%s\": ERROR: SaveInProgress : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: RestoreInProgress : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: NotConnected : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": ERROR: RTIinternalError : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_constrained():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*! @brief Enable time regulating.
 *  @param time the granted HLA Logical time */
void TimeManagement::set_time_regulation_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the show above to avoid a race condition with
      // the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_regulation_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::set_time_regulation_enabled():%d TimeManagement \
\"%s\" Time granted to: %.12G \n",
                       __LINE__, federate->get_federate_name().c_str(),
                       get_granted_time().get_time_in_seconds() );
   }
}

/*!
 * @job_class{initialization}.
 */
void TimeManagement::setup_time_regulation()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time regulation enabled, or if we are already regulating.
   if ( !this->time_management || !this->time_regulating || this->time_regulating_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      DebugHandler::terminate_with_message( "TimeManagement::setup_time_regulation() ERROR: NULL pointer to RTIambassador!" );
      return;
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::setup_time_regulation():%d \"%s\": ENABLING TIME REGULATION WITH LOOKAHEAD = %G seconds.\n",
                          __LINE__, federate->get_federation_name().c_str(),
                          lookahead.get_time_in_seconds() );
      }

      // RTI_amb->enableTimeRegulation() is an implicit
      // RTI_amb->timeAdvanceRequest() so clear the flags since we will get a
      // FedAmb::timeRegulationEnabled() callback which will set the
      // time-adv state and time_regulating_state flags to true/granted.

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state        = TIME_ADVANCE_RESET;
         this->time_regulating_state = false;
      }

      // Turn on regulating status so that constrained federates will be
      // controlled by our time.
      //
      // If we are regulating and our object attributes and interaction
      // parameters are specified with timestamp in the FOM we will send
      // TimeStamp Ordered messages.
      RTI_ambassador->enableTimeRegulation( lookahead.get() );

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->get_wait_status_time() );
      SleepTimeout sleep_timer;

      // This spin lock waits for the time regulation flag to be set from the RTI.
      while ( !this->time_regulating_state ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_regulating_state ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TimeManagement::setup_time_regulation():" << __LINE__
                         << " ERROR: Unexpectedly the TimeManagement is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TimeManagement::setup_time_regulation():%d \"%s\": ENABLING TIME REGULATION WITH LOOKAHEAD = %G seconds, waiting...\n",
                                __LINE__, federate->get_federation_name().c_str(), lookahead.get_time_in_seconds() );
            }
         }
      }

   } catch ( RTI1516_NAMESPACE::TimeRegulationAlreadyEnabled const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->time_regulating_state = true;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": Time Regulation Already Enabled: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InvalidLookahead const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: InvalidLookahead: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: InTimeAdvancingState: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: RequestForTimeRegulationPending: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: FederateNotExecutionMember: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: SaveInProgress: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: RestoreInProgress: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: NotConnected : '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": ERROR: RTIinternalError: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TimeManagement::setup_time_regulation():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n",
                       __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void TimeManagement::time_advance_request()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   // Do not ask for a time advance on an initialization pass.
   if ( exec_get_mode() == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::time_advance_request():%d exec_init_pass() == true so returning.\n",
                          __LINE__ );
      }
      return;
   }

   // -- start of checkpoint additions --
   // TEMP   this->save_completed = false; // reset ONLY at the bottom of the frame...
   //  -- end of checkpoint additions --

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Build the requested HLA logical time for the next time step.
      if ( is_zero_lookahead_time() ) {
         // Use the TAR job cycle time for the time-step.
         this->requested_time += this->HLA_cycle_time_in_base_time;
      } else {
         // Use the lookahead time for the time-step.
         // Requested time = granted time + lookahead
         this->requested_time += this->lookahead;
      }
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

/*!
 * @job_class{scheduled}
 */
void TimeManagement::perform_time_advance_request()
{
   // -- start of checkpoint additions --
   // TEMP   this->save_completed = false; // reset ONLY at the bottom of the frame...
   //  -- end of checkpoint additions --

   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      if ( is_zero_lookahead_time() ) {
         message_publish( MSG_NORMAL, "TimeManagement::perform_time_advance_request():%d Time Advance Request Available (TARA) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      } else {
         message_publish( MSG_NORMAL, "TimeManagement::perform_time_advance_request():%d Time Advance Request (TAR) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      }
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d WARNING: Already in time requested state!\n",
                          __LINE__ );
      }

      // Clear the TAR flag before we make our request.
      this->time_adv_state = TIME_ADVANCE_RESET;

      try {
         if ( is_zero_lookahead_time() ) {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );
         } else {
            // Request that time be advanced to the new time.
            RTI_ambassador->timeAdvanceRequest( requested_time.get() );
         }

         // Indicate we issued a TAR since we successfully made the request
         // without an exception.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;

      } catch ( RTI1516_NAMESPACE::InvalidLogicalTime const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: InvalidLogicalTime\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::LogicalTimeAlreadyPassed const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: LogicalTimeAlreadyPassed\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
         // A time advance request is still being processed by the RTI so show
         // a message and treat this as a successful time advance request.
         //
         // Indicate we are in the time advance requested state.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;

         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d WARNING: Ignoring InTimeAdvancingState HLA Exception.\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: RequestForTimeRegulationPending\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: RequestForTimeConstrainedPending\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "TimeManagement::perform_time_advance_request():%d \"%s\": Unexpected RTI exception!\n RTI Exception: RTIinternalError: '%s'\n",
                          __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void TimeManagement::wait_for_zero_lookahead_TARA_TAG()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d WARNING: Already in time requested state!\n",
                          __LINE__ );
      } else {

         // Clear the TAR flag before we make our request.
         this->time_adv_state = TIME_ADVANCE_RESET;

         // Macro to save the FPU Control Word register value.
         TRICKHLA_SAVE_FPU_CONTROL_WORD;

         // Time Advance Request Available (TARA)
         try {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );

            // Indicate we issued a TAR since we successfully made the request
            // without an exception.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;

         } catch ( RTI1516_NAMESPACE::InvalidLogicalTime const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: InvalidLogicalTime\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::LogicalTimeAlreadyPassed const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: LogicalTimeAlreadyPassed\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
            // A time advance request is still being processed by the RTI so show
            // a message and treat this as a successful time advance request.
            //
            // Indicate we are in the time advance requested state.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;

            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d WARNING: Ignoring InTimeAdvancingState HLA Exception.\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RequestForTimeRegulationPending\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RequestForTimeConstrainedPending\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: FederateNotExecutionMember\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: SaveInProgress\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RestoreInProgress\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: NotConnected\n",
                             __LINE__ );
         } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d \"%s\": Unexpected RTI exception!\n RTI Exception: RTIinternalError: '%s'\n",
                             __LINE__, federate->get_federation_name().c_str(),
                             rti_err_msg.c_str() );
         }

         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         // We had an error if we are not in the time advance requested state.
         if ( this->time_adv_state != TIME_ADVANCE_REQUESTED ) {
            if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               message_publish( MSG_WARNING, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d WARNING: No Time Advance Request Available call made!\n",
                                __LINE__ );
            }
            return;
         }
      }
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   // Wait for Time Advance Grant (TAG)
   if ( state != TIME_ADVANCE_GRANTED ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->get_wait_status_time() );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TimeManagement::wait_for_zero_lookahead_TARA_TAG():" << __LINE__
                         << " ERROR: Unexpectedly the TimeManagement is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TimeManagement::wait_for_zero_lookahead_TARA_TAG():%d Waiting...\n",
                                __LINE__ );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }
}

/*
 * @brief Verify the time constraints (i.e. Lookahead, LCTS, RT and dt).
 */
bool const TimeManagement::verify_time_constraints()
{
   // Determine if the Trick time Tic resolution can support the HLA base time.
   // Constraint: Me >= Mhla
   if ( exec_get_time_tic_value() < Int64BaseTime::get_base_time_multiplier() ) {
      ostringstream errmsg;
      errmsg << "TimeManagement::verify_time_constraints():" << __LINE__
             << " ERROR: The Trick executive time tic value (" << exec_get_time_tic_value()
             << ") cannot support the HLA base time multiplier resolution ("
             << Int64BaseTime::get_base_time_multiplier() << ")";
      if ( Int64BaseTime::get_base_unit_enum() != HLA_BASE_TIME_NOT_DEFINED ) {
         errmsg << " corresponding to THLA.federate.set_HLA_base_time_unit( trick."
                << Int64BaseTime::get_base_unit_enum_string(
                      Int64BaseTime::get_base_unit_enum() )
                << " )";
      }
      errmsg << ". Please update the Trick time tic value in your input.py file"
             << " (i.e. by calling 'trick.exec_set_time_tic_value( "
             << Int64BaseTime::get_base_time_multiplier() << " )')." << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Constraint: Me % Mhla == 0
   if ( ( exec_get_time_tic_value() % Int64BaseTime::get_base_time_multiplier() ) != 0 ) {
      ostringstream errmsg;
      errmsg << "TimeManagement::verify_time_constraints():" << __LINE__
             << " ERROR: The Trick executive time tic value (" << exec_get_time_tic_value()
             << ") must be an integer multiple of the HLA base time multiplier ("
             << Int64BaseTime::get_base_time_multiplier() << ")";
      if ( Int64BaseTime::get_base_unit_enum() != HLA_BASE_TIME_NOT_DEFINED ) {
         errmsg << " corresponding to THLA.federate.set_HLA_base_time_unit( trick."
                << Int64BaseTime::get_base_unit_enum_string(
                      Int64BaseTime::get_base_unit_enum() )
                << " )";
      }
      errmsg << ". Please update the Trick time tic value in your input.py file"
             << " (i.e. by calling 'trick.exec_set_time_tic_value( )')." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   return TrickThreadCoordinator::verify_time_constraints();
}

/*!
 *  @job_class{scheduled}
 */
void TimeManagement::wait_for_time_advance_grant()
{
#if defined( TRICKHLA_COLLECT_TAG_STATS )
   int64_t const tag_wait_start_time = clock_wall_time();
#endif // TRICKHLA_COLLECT_TAG_STATS

   // Skip requesting time-advancement if time management is not enabled.
   if ( !this->time_management ) {
      return;
   }

   // Do not ask for a time advance on an initialization pass.
   if ( exec_get_mode() == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::wait_for_time_advance_grant():%d In Initialization mode so returning.\n",
                          __LINE__ );
      }
      return;
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   if ( state == TIME_ADVANCE_RESET ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "TimeManagement::wait_for_time_advance_grant():%d WARNING: No Time Advance Requested!\n",
                          __LINE__ );
      }
      return;
   }

   if ( state != TIME_ADVANCE_GRANTED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::wait_for_time_advance_grant():%d Waiting for Time Advance Grant (TAG) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      }

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->get_wait_status_time() );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TimeManagement::wait_for_time_advance_grant():" << __LINE__
                         << " ERROR: Unexpectedly the TimeManagement is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TimeManagement::wait_for_time_advance_grant():%d Waiting...\n",
                                __LINE__ );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }

#if defined( TRICKHLA_COLLECT_TAG_STATS )
   tag_wait_sum += ( clock_wall_time() - tag_wait_start_time );
   ++tag_wait_count;
#endif // TRICKHLA_COLLECT_TAG_STATS

   // Add the line number for a higher trace level.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "TimeManagement::wait_for_time_advance_grant():%d Time Advance Grant (TAG) to %.12G seconds.\n",
                       __LINE__, granted_time.get_time_in_seconds() );
   }
}

/*!
 *  @details Shutdown this federate's time management by shutting down time
 *  constraint management and time regulating management.
 *  @job_class{shutdown}
 */
void TimeManagement::shutdown_time_management()
{
   shutdown_time_constrained();
   shutdown_time_regulating();
}

/*!
 *  @job_class{shutdown}
 */
void TimeManagement::shutdown_time_constrained()
{
   if ( !this->time_constrained_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::shutdown_time_constrained():%d HLA Time Constrained Already Disabled.\n",
                          __LINE__ );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( RTI_ambassador.get() == NULL ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::shutdown_time_constrained():%d Disabling HLA Time Constrained.\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->disableTimeConstrained();
         this->time_constrained_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": TimeConstrainedIsNotEnabled EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": FederateNotExecutionMember EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": SaveInProgress EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": RestoreInProgress EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": NotConnected EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": RTIinternalError EXCEPTION: '%s'\n",
                          __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::Exception const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_constrained():%d \"%s\": Unexpected RTI EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 *  @job_class{shutdown}
 */
void TimeManagement::shutdown_time_regulating()
{
   if ( !this->time_regulating_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::shutdown_time_regulating():%d HLA Time Regulation Already Disabled.\n",
                          __LINE__ );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( RTI_ambassador.get() == NULL ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "TimeManagement::shutdown_time_regulating():%d Disabling HLA Time Regulation.\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->disableTimeRegulation();
         this->time_regulating_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_regulating_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": TimeConstrainedIsNotEnabled EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_regulating_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": FederateNotExecutionMember EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": SaveInProgress EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": RestoreInProgress EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": NotConnected EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": RTIinternalError EXCEPTION: '%s'\n",
                          __LINE__, federate->get_federation_name().c_str(), rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::Exception const &e ) {
         message_publish( MSG_WARNING, "TimeManagement::shutdown_time_regulating():%d \"%s\": Unexpected RTI EXCEPTION!\n",
                          __LINE__, federate->get_federation_name().c_str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}
