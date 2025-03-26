/*!
@file SpaceFOM/RefFrameLagCompInteg.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
reference frame latency/lag compensation class that uses integration to
compensate the reference frame state.

@copyright Copyright 2023 United States Government as represented by the
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
@trick_link_dependency{../TrickHLA/LagCompensationInteg.cpp}
@trick_link_dependency{RefFrameLagCompInteg.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <float.h>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameLagCompInteg.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameLagCompInteg::RefFrameLagCompInteg( RefFrameBase &ref_frame_ref ) // RETURN: -- None.
   : RefFrameLagCompBase( ref_frame_ref ),
     TrickHLA::LagCompensationInteg()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameLagCompInteg::~RefFrameLagCompInteg() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameLagCompInteg::initialize()
{
   if ( this->integ_dt < this->integ_tol ) {
      ostringstream errmsg;

      errmsg << "SpaceFOM::RefFrameLagCompInteg::initialize():" << __LINE__ << '\n'
             << " ERROR: Tolerance must be less that the dt!: dt = "
             << this->integ_dt << "; tolerance = " << this->integ_tol << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Call the base class initialize routine.
   RefFrameLagCompBase::initialize();

   // Return to calling routine.
   return;
}

/*! @brief Sending side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void RefFrameLagCompInteg::send_lag_compensation()
{
   double begin_t = get_scenario_time();
   double end_t;

   // Save the compensation time step.
   this->compensate_dt = get_lookahead().get_time_in_seconds();
   end_t               = begin_t + this->compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream errmsg;
      errmsg << "****** RefFrameLagCompInteg::send_lag_compensation():" << __LINE__ << '\n'
             << " scenario-time:" << get_scenario_time() << '\n'
             << "     lookahead:" << this->compensate_dt << '\n'
             << " adjusted-time:" << end_t << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Copy the current RefFrame state over to the lag compensated state.
   ref_frame.pack_from_working_data();
   load_lag_comp_data();
   Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "Send data before compensation: \n";
      print_lag_comp_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Compensate the data
   compensate( begin_t, end_t );

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "Send data after compensation: \n";
      print_lag_comp_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Copy the compensated state to the packing data.
   unload_lag_comp_data();

   // Return to calling routine.
   return;
}

/*! @brief Receive side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void RefFrameLagCompInteg::receive_lag_compensation()
{
   double end_t  = get_scenario_time();
   double data_t = ref_frame.get_time();

   // Save the compensation time step.
   this->compensate_dt = end_t - data_t;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream errmsg;
      errmsg << "******* RefFrameLagCompInteg::receive_lag_compensation():" << __LINE__ << '\n'
             << "  scenario-time:" << end_t << '\n'
             << "      data-time:" << data_t << '\n'
             << " comp-time-step:" << this->compensate_dt << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received attribute data.
   if ( state_attr->is_received() ) {

      // Copy the current RefFrame state over to the lag compensated state.
      load_lag_comp_data();
      Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

      // Print out debug information if desired.
      if ( debug ) {
         ostringstream msg;
         msg << "Receive data before compensation: \n";
         print_lag_comp_data( msg );
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Compensate the data
      compensate( data_t, end_t );

      // Print out debug information if desired.
      if ( debug ) {
         ostringstream msg;
         msg << "Receive data after compensation: \n";
         print_lag_comp_data( msg );
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   // Copy the compensated state to the packing data.
   unload_lag_comp_data();

   // Move the unpacked data into the working data.
   ref_frame.unpack_into_working_data();

   // Return to calling routine.
   return;
}
