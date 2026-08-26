/*!
@file TrickHLA/time/TimeManagementServices.cpp
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
@trick_link_dependency{TimeManagementServices.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{TrickThreadCoordinator.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../ExecutionControlBase.cpp}
@trick_link_dependency{../FedAmb.cpp}
@trick_link_dependency{../Federate.cpp}
@trick_link_dependency{../ObjectServices.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../utils/MutexLock.cpp}
@trick_link_dependency{../utils/MutexProtection.cpp}
@trick_link_dependency{../utils/SleepTimeout.cpp}
@trick_link_dependency{../utils/Utilities.cpp}

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
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/TimeManagementServices.hh"
#include "TrickHLA/time/TrickThreadCoordinator.hh"
#include "TrickHLA/utils/MutexProtection.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/Utilities.hh"

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

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details NOTE: In most cases, we would allocate and set default names in
 * the constructor. However, since we want this class to be Input Processor
 * friendly, we cannot do that here since the Input Processor may not have
 * been initialized yet. So, we have to set the name information to nullptr and
 * then allocate and set the defaults in the initialization job if not
 * already set in the input stream.
 *
 * @job_class{initialization}
 */
TimeManagementServices::TimeManagementServices(
   Federate &fed )
   : TrickThreadCoordinator( &fed ),
     lookahead_time( 0.0 ),
     time_regulating( true ),
     time_constrained( true ),
     time_management( true ),
     lookahead( (int64_t)0 ),
     HLA_cycle_time( 0.0 ),
     HLA_cycle_time_in_base_time( 0 ),
     granted_time( (int64_t)0 ),
     requested_time( (int64_t)0 ),
     HLA_time( 0.0 ),
     time_adv_state( TrickHLA::TIME_ADVANCE_RESET ),
     time_adv_state_mutex(),
     time_regulating_state( false ),
     time_constrained_state( false ),
     tag_wait_sum( 0 ),
     tag_wait_count( 0 )
{
   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
TimeManagementServices::~TimeManagementServices()
{
   // Make sure we destroy the mutex.
   time_adv_state_mutex.destroy();
}

/*!
 * @brief Initialize the thread memory associated with the Trick child threads.
 */
void TimeManagementServices::initialize_thread_state(
   double const main_thread_data_cycle_time )
{
   this->HLA_cycle_time              = main_thread_data_cycle_time;
   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                   string( "Trick main thread (id:0, data_cycle:" )
                                      .append( std::to_string( this->HLA_cycle_time ) )
                                      .append( ")\n" ) );
   }

   // Make sure the Trick thread coordinator is initialized. This will
   // also associate the Trick main thread. TrickHLA will maintain data
   // coherency for the HLA object instances specified in the input file
   // over the data cycle time specified.
   initialize_thread_coordinator( this->HLA_cycle_time );

   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   ObjectServices const *object_service = federate->get_object_service();
   for ( int n = 0; n < object_service->obj_count; ++n ) {
      object_service->objects[n].set_core_job_cycle_time(
         Int64BaseTime::to_seconds(
            get_data_cycle_base_time_for_obj( n, get_HLA_cycle_time_in_base_time() ) ) );
   }
}

/*!
 * @job_class{initialization}
 */
void TimeManagementServices::restart_initialization()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "\n" );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Update the lookahead time in our HLA time line.
   set_lookahead( lookahead_time );

   // The lookahead time can not be negative.
   if ( lookahead_time < 0.0 ) {
      ostringstream errmsg;
      errmsg << "Invalid HLA lookahead time!"
             << " Lookahead time (" << lookahead_time << " seconds)"
             << " must be greater than or equal to zero and not negative. Make"
             << " sure 'lookahead_time' in your input.py or modified-data file is"
             << " not a negative number.\n";
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*! @brief Set the time advance as granted. */
void TimeManagementServices::set_time_advance_granted(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   Int64Time const int64_time( time );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

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

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      string( "Granted-time:" )
                                         .append( std::to_string( this->HLA_time ) )
                                         .append( ", Requested-time:" )
                                         .append( std::to_string( requested_time.get_time_in_seconds() ) )
                                         .append( "\n" ) );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         ostringstream errmsg;
         errmsg << "'" << federate->get_federate_name()
                << "': IGNORING GRANTED TIME " << setprecision( 18 )
                << int64_time.get_time_in_seconds()
                << " seconds because it is less than the requested time "
                << requested_time.get_time_in_seconds() << " seconds\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, errmsg.str(), MSG_WARNING );
      }
   }
}

void TimeManagementServices::set_granted_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void TimeManagementServices::set_granted_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void TimeManagementServices::set_requested_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

void TimeManagementServices::set_requested_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

/*! @brief Sets the requested time to the granted time. */
void TimeManagementServices::set_requested_time_to_granted_time()
{
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( granted_time );
}

HLABaseTimeEnum TimeManagementServices::get_HLA_base_time_unit()
{
   return Int64BaseTime::get_base_unit_enum();
}

/*! @brief Sets the HLA base time unit.
 *  @param base_time_unit HLA base time unit. */
void TimeManagementServices::set_HLA_base_time_unit(
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
void TimeManagementServices::set_HLA_base_time_unit_and_scale_trick_tics(
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
void TimeManagementServices::set_HLA_base_time_multiplier(
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
void TimeManagementServices::set_HLA_base_time_multiplier_and_scale_trick_tics(
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

void TimeManagementServices::refresh_HLA_time_constants()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &mutex );

   refresh_lookahead();

   federate->get_execution_control()->refresh_least_common_time_step();

   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );

   refresh_thread_base_times();
}

void TimeManagementServices::scale_trick_tics_to_HLA_base_time_multiplier()
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
         exec_set_time_tic_value( (int)tic_value );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                         string( "New Trick time tics:" )
                                            .append( std::to_string( tic_value ) )
                                            .append( "\n" ) );
         }
      }
   } else {
      ostringstream errmsg;
      errmsg << "Trick cannot represent the required time Tic value "
             << setprecision( 18 ) << time_res
             << " in order to support the HLA base unit of '"
             << Int64BaseTime::get_base_unit()
             << "'.\n";
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
   }
}

void TimeManagementServices::set_lookahead(
   double const value )
{
   // Determine if the lookahead time needs a resolution that exceeds the
   // configured HLA base time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value ) ) {
      ostringstream errmsg;
      errmsg << "The lookahead time specified (" << setprecision( 18 ) << value
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
             << " Specific Federation Agreement (FESFA) and TimeManagementServices Compliance"
             << " Declaration (FCD) documents for your Federation to document"
             << " the change in timing class resolution.\n";
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
   }

   // Determine if the Trick time Tic can represent the lookahead time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value, exec_get_time_tic_value() ) ) {
      ostringstream errmsg;
      errmsg << "The Trick time tic value (" << exec_get_time_tic_value()
             << ") does not have enough resolution to represent the HLA lookahead time ("
             << setprecision( 18 ) << value
             << " seconds). Please update the Trick time tic value in your"
             << " input.py file (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
   lookahead.set( value );
   this->lookahead_time = value;
}

/*! @brief Update the HLA lookahead base time. */
void TimeManagementServices::refresh_lookahead()
{
   // Recalculate the lookahead HLA time in base time units.
   set_lookahead( this->lookahead_time );
}

void TimeManagementServices::time_advance_request_to_GALT()
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
      if ( federate->RTI_ambassador->queryGALT( time ) ) {
         int64_t const L = lookahead.get_base_time();
         if ( L > 0 ) {
            int64_t const GALT = time.getTime();

            // Make sure the time is an integer multiple of the lookahead time.
            time.setTime( ( ( GALT / L ) + 1 ) * L );
         }
         set_requested_time( time );
      }
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      if ( federate != nullptr ) {
         federate->set_connection_lost();
      }
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                   string( "Requested-Time:" )
                                      .append( std::to_string( requested_time.get_time_in_seconds() ) )
                                      .append( " seconds\n" ) );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

void TimeManagementServices::time_advance_request_to_GALT_LCTS_multiple()
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
      if ( federate->RTI_ambassador->queryGALT( time ) ) {
         if ( LCTS > 0 ) {
            int64_t const GALT = time.getTime();

            // Make sure the time is an integer multiple of the LCTS time.
            time.setTime( ( ( GALT / LCTS ) + 1 ) * LCTS );
         }
         set_requested_time( time );
      }
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      if ( federate != nullptr ) {
         federate->set_connection_lost();
      }
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                   string( "Requested-Time:" )
                                      .append( std::to_string( requested_time.get_time_in_seconds() ) )
                                      .append( " seconds\n" ) );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

/*!
 * @job_class{initialization}.
 */
void TimeManagementServices::setup_time_management()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                   string( "time_management:" )
                                      .append( this->time_management ? "Yes" : "No" )
                                      .append( " time_regulating:" )
                                      .append( this->time_regulating ? "Yes" : "No" )
                                      .append( " time_constrained:" )
                                      .append( this->time_constrained ? "Yes" : "No" )
                                      .append( "\n" ) );
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

void TimeManagementServices::set_time_constrained_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the debug show above to avoid a race condition
      // with the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_constrained_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      ostringstream msg;
      msg << "'" << federate->get_federate_name()
          << "': Time granted to: " << setprecision( 18 )
          << get_granted_time().get_time_in_seconds() << " seconds\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }
}

/*!
 * @job_class{initialization}.
 */
void TimeManagementServices::setup_time_constrained()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time constrained enabled, or if we are already constrained.
   if ( !this->time_management || !this->time_constrained || this->time_constrained_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( federate->RTI_ambassador.get() == nullptr ) {
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__,
                               "nullptr pointer to RTIambassador!" );
      return;
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      string( "'" )
                                         .append( federate->get_federation_name() )
                                         .append( "': ENABLING TIME CONSTRAINED\n" ) );
      }

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state         = TIME_ADVANCE_RESET;
         this->time_constrained_state = false;
      }

      // Turn on constrained status so that regulating federates will control
      // our advancement in time.
      //
      // If we are constrained and sending federates specify the Class
      // attributes and Communication interaction with timestamp in the
      // simulation fed file we will receive TimeStamp Ordered messages.
      federate->RTI_ambassador->enableTimeConstrained();

      SleepTimeout print_timer;
      SleepTimeout sleep_timer;

      // This spin lock waits for the time constrained flag to be set from the RTI.
      while ( !this->time_constrained_state ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_constrained_state ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Unexpectedly the TimeManagementServices is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                            string( "'" )
                                               .append( federate->get_federation_name() )
                                               .append( "': ENABLING TIME CONSTRAINED, waiting...\n" ) );
            }
         }
      }
   } catch ( RTI1516_NAMESPACE::TimeConstrainedAlreadyEnabled const &e ) {
      this->time_constrained_state = true;
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      federate->set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*! @brief Enable time regulating.
 *  @param time the granted HLA Logical time */
void TimeManagementServices::set_time_regulation_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the show above to avoid a race condition with
      // the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_regulation_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      ostringstream msg;
      msg << "'" << federate->get_federate_name()
          << "': Time granted to: " << setprecision( 18 )
          << get_granted_time().get_time_in_seconds() << " seconds\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }
}

/*!
 * @job_class{initialization}.
 */
void TimeManagementServices::setup_time_regulation()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time regulation enabled, or if we are already regulating.
   if ( !this->time_management || !this->time_regulating || this->time_regulating_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( federate->RTI_ambassador.get() == nullptr ) {
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, "nullptr pointer to RTIambassador!" );
      return;
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         ostringstream msg;
         msg << "'" << federate->get_federation_name()
             << "': ENABLING TIME REGULATION WITH LOOKAHEAD = "
             << setprecision( 18 ) << lookahead.get_time_in_seconds()
             << " seconds.\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
      }

      // RTI_amb->enableTimeRegulation() is an implicit
      // RTI_amb->timeAdvanceRequest() so clear the flags since we will get a
      // FedAmb::timeRegulationEnabled() callback which will set the
      // time-adv state and time_regulating_state flags to true/granted.

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state        = TIME_ADVANCE_RESET;
         this->time_regulating_state = false;
      }

      // Turn on regulating status so that constrained federates will be
      // controlled by our time.
      //
      // If we are regulating and our object attributes and interaction
      // parameters are specified with timestamp in the FOM we will send
      // TimeStamp Ordered messages.
      federate->RTI_ambassador->enableTimeRegulation( lookahead.get() );

      SleepTimeout print_timer;
      SleepTimeout sleep_timer;

      // This spin lock waits for the time regulation flag to be set from the RTI.
      while ( !this->time_regulating_state ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_regulating_state ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Unexpectedly the TimeManagementServices is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               ostringstream msg;
               msg << "'" << federate->get_federation_name()
                   << "': ENABLING TIME REGULATION WITH LOOKAHEAD = "
                   << setprecision( 18 ) << lookahead.get_time_in_seconds()
                   << " seconds, waiting...\n";
               DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
            }
         }
      }

   } catch ( RTI1516_NAMESPACE::TimeRegulationAlreadyEnabled const &e ) {
      this->time_regulating_state = true;
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::InvalidLookahead const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      federate->set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void TimeManagementServices::time_advance_request()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   // Do not ask for a time advance on an initialization pass.
   if ( exec_get_mode() == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "exec_get_mode() == Initialization so returning.\n" );
      }
      return;
   }

   // -- start of checkpoint additions --
   // TEMP   federate->set_save_completed( false ); // reset ONLY at the bottom of the frame...
   //  -- end of checkpoint additions --

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

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
void TimeManagementServices::perform_time_advance_request()
{
   // -- start of checkpoint additions --
   // TEMP   federate->set_save_completed( false ); // reset ONLY at the bottom of the frame...
   //  -- end of checkpoint additions --

   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      ostringstream msg;
      if ( is_zero_lookahead_time() ) {
         msg << "Time Advance Request Available (TARA) to " << setprecision( 18 )
             << requested_time.get_time_in_seconds() << " seconds.\n";
      } else {
         msg << "Time Advance Request (TAR) to " << setprecision( 18 )
             << requested_time.get_time_in_seconds() << " seconds.\n";
      }
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "Already in time requested state!\n",
                                      MSG_WARNING );
      }

      // Clear the TAR flag before we make our request.
      this->time_adv_state = TIME_ADVANCE_RESET;

      try {
         if ( is_zero_lookahead_time() ) {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            federate->RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );
         } else {
            // Request that time be advanced to the new time.
            federate->RTI_ambassador->timeAdvanceRequest( requested_time.get() );
         }

         // Indicate we issued a TAR since we successfully made the request
         // without an exception.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;

      } catch ( RTI1516_NAMESPACE::InvalidLogicalTime const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::LogicalTimeAlreadyPassed const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
         // A time advance request is still being processed by the RTI so show
         // a message and treat this as a successful time advance request.
         //
         // Indicate we are in the time advance requested state.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         federate->set_connection_lost();
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void TimeManagementServices::wait_for_zero_lookahead_TARA_TAG()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "Already in time requested state!\n",
                                      MSG_WARNING );
      } else {

         // Clear the TAR flag before we make our request.
         this->time_adv_state = TIME_ADVANCE_RESET;

         // Macro to save the FPU Control Word register value.
         TRICKHLA_SAVE_FPU_CONTROL_WORD;

         // Time Advance Request Available (TARA)
         try {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            federate->RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );

            // Indicate we issued a TAR since we successfully made the request
            // without an exception.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;

         } catch ( RTI1516_NAMESPACE::InvalidLogicalTime const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::LogicalTimeAlreadyPassed const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
            // A time advance request is still being processed by the RTI so show
            // a message and treat this as a successful time advance request.
            //
            // Indicate we are in the time advance requested state.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
            federate->set_connection_lost();
         } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
            DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         }

         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         // We had an error if we are not in the time advance requested state.
         if ( this->time_adv_state != TIME_ADVANCE_REQUESTED ) {
            if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
               DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                            "No Time Advance Request Available call made!\n",
                                            MSG_WARNING );
            }
            return;
         }
      }
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   // Wait for Time Advance Grant (TAG)
   if ( state != TIME_ADVANCE_GRANTED ) {

      SleepTimeout print_timer;
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Unexpectedly the TimeManagementServices is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "Waiting...\n" );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }
}

/*
 * @brief Verify the time constraints (i.e. Lookahead, LCTS, Me, Mhla, RT, and dt).
 */
bool TimeManagementServices::verify_time_constraints()
{
   // Determine if the Trick time Tic resolution can support the HLA base time.
   // Constraint: Me >= Mhla
   if ( exec_get_time_tic_value() < Int64BaseTime::get_base_time_multiplier() ) {
      ostringstream errmsg;
      errmsg << "The Trick executive time tic value (" << exec_get_time_tic_value()
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
             << Int64BaseTime::get_base_time_multiplier() << " )').\n";

      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
      return false;
   }

   // Constraint: Me % Mhla == 0
   if ( ( exec_get_time_tic_value() % Int64BaseTime::get_base_time_multiplier() ) != 0 ) {
      ostringstream errmsg;
      errmsg << "The Trick executive time tic value (" << exec_get_time_tic_value()
             << ") must be an integer multiple of the HLA base time multiplier ("
             << Int64BaseTime::get_base_time_multiplier() << ")";
      if ( Int64BaseTime::get_base_unit_enum() != HLA_BASE_TIME_NOT_DEFINED ) {
         errmsg << " corresponding to THLA.federate.set_HLA_base_time_unit( trick."
                << Int64BaseTime::get_base_unit_enum_string(
                      Int64BaseTime::get_base_unit_enum() )
                << " )";
      }
      errmsg << ". Please update the Trick time tic value in your input.py file"
             << " (i.e. by calling 'trick.exec_set_time_tic_value( )').\n";
      DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
      return false;
   }

   return TrickThreadCoordinator::verify_time_constraints();
}

/*!
 *  @job_class{scheduled}
 */
void TimeManagementServices::wait_for_time_advance_grant()
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
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "In Initialization mode so returning.\n" );
      }
      return;
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   if ( state == TIME_ADVANCE_RESET ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "No Time Advance Requested!\n", MSG_WARNING );
      }
      return;
   }

   if ( state != TIME_ADVANCE_GRANTED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         ostringstream msg;
         msg << "Waiting for Time Advance Grant (TAG) to " << setprecision( 18 )
             << requested_time.get_time_in_seconds() << " seconds.\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
      }

      SleepTimeout print_timer;
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection const auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Unexpectedly the TimeManagementServices is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "Waiting...\n" );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }

#if defined( TRICKHLA_COLLECT_TAG_STATS )
   tag_wait_sum += ( clock_wall_time() - tag_wait_start_time );
   ++tag_wait_count;
#endif // TRICKHLA_COLLECT_TAG_STATS

   // Add the line number for a higher trace level.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
      ostringstream msg;
      msg << "Time Advance Grant (TAG) to " << setprecision( 18 )
          << granted_time.get_time_in_seconds() << " seconds.\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }
}

/*!
 *  @details Shutdown this federate's time management by shutting down time
 *  constraint management and time regulating management.
 *  @job_class{shutdown}
 */
void TimeManagementServices::shutdown_time_management()
{
   shutdown_time_constrained();
   shutdown_time_regulating();
}

/*!
 *  @job_class{shutdown}
 */
void TimeManagementServices::shutdown_time_constrained()
{
   if ( !this->time_constrained_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "HLA Time Constrained Already Disabled.\n" );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( federate->RTI_ambassador.get() == nullptr ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "Disabling HLA Time Constrained.\n" );
      }

      try {
         federate->RTI_ambassador->disableTimeConstrained();
         this->time_constrained_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_constrained_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_constrained_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         federate->set_connection_lost();
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::Exception const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 *  @job_class{shutdown}
 */
void TimeManagementServices::shutdown_time_regulating()
{
   if ( !this->time_regulating_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "HLA Time Regulation Already Disabled.\n" );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( federate->RTI_ambassador.get() == nullptr ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_TIME_MGMT_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "Disabling HLA Time Regulation.\n" );
      }

      try {
         federate->RTI_ambassador->disableTimeRegulation();
         this->time_regulating_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_regulating_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_regulating_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
         federate->set_connection_lost();
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      } catch ( RTI1516_NAMESPACE::Exception const &e ) {
         DebugHandler::print_exception( __PRETTY_FUNCTION__, __LINE__, e );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}
