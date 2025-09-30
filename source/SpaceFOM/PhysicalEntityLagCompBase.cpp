/*!
@file SpaceFOM/DynamicalEntityLagCompBase.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
PhysicalEntity latency/lag compensation class.

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
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{PhysicalEntityLagCompBase.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/reference_frame.h"

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityBase.hh"
#include "SpaceFOM/PhysicalEntityData.hh"
#include "SpaceFOM/PhysicalEntityLagCompBase.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagCompBase::PhysicalEntityLagCompBase( PhysicalEntityBase &entity_ref ) // RETURN: -- None.
   : TrickHLA::LagCompensation( "PhysicalEntityLagCompBase" ),
     debug( false ),
     entity( entity_ref ),
     name_attr( NULL ),
     type_attr( NULL ),
     status_attr( NULL ),
     parent_frame_attr( NULL ),
     state_attr( NULL ),
     accel_attr( NULL ),
     ang_accel_attr( NULL ),
     cm_attr( NULL ),
     body_frame_attr( NULL ),
     compensate_dt( 0.0 )
{
   // Initialize the acceleration values.
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      this->accel[iinc]     = 0.0;
      this->ang_accel[iinc] = 0.0;
      this->cm[iinc]        = 0.0;
   }
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityLagCompBase::~PhysicalEntityLagCompBase() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagCompBase::initialize()
{
   // Make sure to call the base class initialize.
   LagCompensation::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::LagCompensation class. We override this function
 * so that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates.
 *
 * Use the initialize callback function as a way to setup TrickHLA::Attribute
 * references which are use to determine ownership or if data for an attribute
 * was received.
 *
 * @job_class{initialization}
 */
void PhysicalEntityLagCompBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   TrickHLA::LagCompensation::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr         = entity.name_attr;
   type_attr         = entity.type_attr;
   status_attr       = entity.status_attr;
   parent_frame_attr = entity.parent_frame_attr;
   state_attr        = entity.state_attr;
   accel_attr        = entity.accel_attr;
   ang_accel_attr    = entity.ang_accel_attr;
   cm_attr           = entity.cm_attr;
   body_frame_attr   = entity.body_frame_attr;

   return;
}

/*! @brief Initialization integration states. */
void PhysicalEntityLagCompBase::initialize_states()
{
   // Copy the current PhysicalEntity state over to the lag compensated state.
   this->lag_comp_data   = entity.pe_packing_data.state;
   this->body_wrt_struct = entity.pe_packing_data.body_wrt_struct;
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      this->accel[iinc]     = entity.pe_packing_data.accel[iinc];
      this->ang_accel[iinc] = entity.pe_packing_data.ang_accel[iinc];
      this->cm[iinc]        = entity.pe_packing_data.cm[iinc];
   }
   Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

   // Return to calling routine.
   return;
}

/*! @brief Sending side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void PhysicalEntityLagCompBase::send_lag_compensation()
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
      errmsg << "******* PhysicalEntityLagCompInteg::send_lag_compensation():" << __LINE__ << endl
             << " scenario-time:" << get_scenario_time() << endl
             << "     lookahead:" << this->compensate_dt << endl
             << " adjusted-time:" << end_t << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Copy the current PhysicalEntity state over to the lag compensated state.
   entity.pack_from_working_data();
   load_lag_comp_data();
   Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "Send data before compensation:" << endl;
      print_lag_comp_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Compensate the data
   compensate( begin_t, end_t );

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "Send data after compensation:" << endl;
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
void PhysicalEntityLagCompBase::receive_lag_compensation()
{
   double end_t  = get_scenario_time();
   double data_t = entity.get_time();

   // Save the compensation time step.
   this->compensate_dt = end_t - data_t;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream errmsg;
      errmsg << "******* PhysicalEntityLagCompInteg::receive_lag_compensation():" << __LINE__ << endl
             << "  scenario-time:" << end_t << endl
             << "      data-time:" << data_t << endl
             << " comp-time-step:" << this->compensate_dt << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received attribute data.
   if ( state_attr->is_received() ) {

      // Copy the current PhysicalEntity state over to the lag compensated state.
      load_lag_comp_data();
      Q_dot.derivative_first( lag_comp_data.att, lag_comp_data.ang_vel );

      // Print out debug information if desired.
      if ( debug ) {
         ostringstream msg;
         msg << "Receive data before compensation:" << endl;
         print_lag_comp_data( msg );
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Compensate the data
      compensate( data_t, end_t );

      // Print out debug information if desired.
      if ( debug ) {
         ostringstream msg;
         msg << "Receive data after compensation:" << endl;
         print_lag_comp_data( msg );
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   // Copy the compensated state to the packing data.
   unload_lag_comp_data();

   // Move the unpacked data into the working data.
   entity.unpack_into_working_data();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityLagCompBase::bypass_send_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the working data into the packing data.  This makes sure that the
   // current working state is packed.
   entity.pack_from_working_data();
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityLagCompBase::bypass_receive_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the packing data back into the working data.  This makes sure that the
   // working state is updated from the received packing data.
   entity.unpack_into_working_data();
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityLagCompBase::unload_lag_comp_data()
{
   // Copy the current PhysicalEntity state over to the lag compensated state.
   entity.pe_packing_data.state           = this->lag_comp_data;
   entity.pe_packing_data.body_wrt_struct = this->body_wrt_struct;
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      entity.pe_packing_data.accel[iinc]     = this->accel[iinc];
      entity.pe_packing_data.ang_accel[iinc] = this->ang_accel[iinc];
      entity.pe_packing_data.cm[iinc]        = this->cm[iinc];
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityLagCompBase::load_lag_comp_data()
{
   // Copy the current PhysicalEntity state over to the lag compensated state.
   this->lag_comp_data   = entity.pe_packing_data.state;
   this->body_wrt_struct = entity.pe_packing_data.body_wrt_struct;
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      this->accel[iinc]     = entity.pe_packing_data.accel[iinc];
      this->ang_accel[iinc] = entity.pe_packing_data.ang_accel[iinc];
      this->cm[iinc]        = entity.pe_packing_data.cm[iinc];
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityLagCompBase::print_lag_comp_data( std::ostream &stream ) const
{
   double euler_angles[3];

   // Set the print precision.
   stream.precision( 15 );

   // Compute the attitude Euler angles.
   lag_comp_data.att.get_Euler_deg( Roll_Pitch_Yaw, euler_angles );

   stream << "\tScenario time: " << get_scenario_time() << endl;
   stream << "\tLag comp time: " << this->lag_comp_data.time << endl;
   stream << "\tposition: "
          << "\t\t" << this->lag_comp_data.pos[0] << ", "
          << "\t\t" << this->lag_comp_data.pos[1] << ", "
          << "\t\t" << this->lag_comp_data.pos[2] << endl;
   stream << "\tvelocity: "
          << "\t\t" << this->lag_comp_data.vel[0] << ", "
          << "\t\t" << this->lag_comp_data.vel[1] << ", "
          << "\t\t" << this->lag_comp_data.vel[2] << endl;
   stream << "\tacceleration: "
          << "\t\t" << this->accel[0] << ", "
          << "\t\t" << this->accel[1] << ", "
          << "\t\t" << this->accel[2] << endl;
   stream << "\tattitude (s;v): "
          << "\t\t" << this->lag_comp_data.att.scalar << "; "
          << "\t\t" << this->lag_comp_data.att.vector[0] << ", "
          << "\t\t" << this->lag_comp_data.att.vector[1] << ", "
          << "\t\t" << this->lag_comp_data.att.vector[2] << endl;
   stream << "\tattitude (RPY){deg}: "
          << "\t\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << endl;
   stream << "\tangular velocity: "
          << "\t\t" << this->lag_comp_data.ang_vel[0] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[1] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[2] << endl;
   stream << "\tattitude rate (s;v): "
          << "\t\t" << this->Q_dot.scalar << "; "
          << "\t\t" << this->Q_dot.vector[0] << ", "
          << "\t\t" << this->Q_dot.vector[1] << ", "
          << "\t\t" << this->Q_dot.vector[2] << endl;
   stream << "\tangular acceleration: "
          << "\t\t" << this->ang_accel[0] << ", "
          << "\t\t" << this->ang_accel[1] << ", "
          << "\t\t" << this->ang_accel[2] << endl;
   stream << "\tcenter of mass: "
          << "\t\t" << this->cm[0] << ", "
          << "\t\t" << this->cm[1] << ", "
          << "\t\t" << this->cm[2] << endl;
   stream << "\tbody wrt. struct (s;v): "
          << "\t\t" << this->body_wrt_struct.scalar << "; "
          << "\t\t" << this->body_wrt_struct.vector[0] << ", "
          << "\t\t" << this->body_wrt_struct.vector[1] << ", "
          << "\t\t" << this->body_wrt_struct.vector[2] << endl;

   // Return to the calling routine.
   return;
}
