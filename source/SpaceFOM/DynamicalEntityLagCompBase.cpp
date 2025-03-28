/*!
@file SpaceFOM/DynamicalEntityLagCompBase.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
DynamicalEntity latency/lag compensation class.

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
@trick_link_dependency{DynamicalEntityLagCompBase.cpp}


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
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/trick_math.h"
#include "trick/trick_math_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagCompBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityLagCompBase::DynamicalEntityLagCompBase( DynamicalEntityBase &entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompBase( entity_ref ),
     de_entity( entity_ref ),
     force_attr( NULL ),
     torque_attr( NULL ),
     mass_attr( NULL ),
     mass_rate_attr( NULL ),
     inertia_attr( NULL ),
     inertia_rate_attr( NULL ),
     mass( 1.0 ),
     mass_rate( 0.0 )
{
   // Initialize the working parameters.
   for ( int iinc = 0; iinc < 3; ++iinc ) {

      this->force[iinc]  = 0.0;
      this->torque[iinc] = 0.0;

      for ( int jinc = 0; jinc < 3; ++jinc ) {
         this->inertia[iinc][jinc]      = 0.0;
         this->inertia_rate[iinc][jinc] = 0.0;
         this->inertia_inv[iinc][jinc]  = 0.0;
      }
      this->inertia[iinc][iinc] = 1.0;

      this->accel_env[iinc]          = 0.0;
      this->ang_accel_env[iinc]      = 0.0;
      this->ang_accel_inertial[iinc] = 0.0;
   }
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityLagCompBase::~DynamicalEntityLagCompBase() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntityLagCompBase::initialize()
{
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
void DynamicalEntityLagCompBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   PhysicalEntityLagCompBase::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   force_attr        = this->de_entity.force_attr;
   torque_attr       = this->de_entity.torque_attr;
   mass_attr         = this->de_entity.mass_attr;
   mass_rate_attr    = this->de_entity.mass_rate_attr;
   inertia_attr      = this->de_entity.inertia_attr;
   inertia_rate_attr = this->de_entity.inertia_rate_attr;

   return;
}

/*! @brief Initialization integration states. */
void DynamicalEntityLagCompBase::initialize_states()
{
   // Call the base class implementation.
   PhysicalEntityLagCompBase::initialize_states();

   // Return to calling routine.
   return;
}

/*! @brief Sending side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void DynamicalEntityLagCompBase::send_lag_compensation()
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
      errmsg << "******* DynamicalEntityLagCompInteg::send_lag_compensation():" << __LINE__ << '\n'
             << " scenario-time:" << get_scenario_time() << '\n'
             << "     lookahead:" << this->compensate_dt << '\n'
             << " adjusted-time:" << end_t << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Copy the current DynamicalEntity state over to the lag compensated state.
   entity.pack_from_working_data();
   load_lag_comp_data();
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

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
void DynamicalEntityLagCompBase::receive_lag_compensation()
{
   double end_t  = get_scenario_time();
   double data_t = entity.get_time();

   // Save the compensation time step.
   this->compensate_dt = end_t - data_t;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream errmsg;
      errmsg << "******* DynamicalEntityLagCompInteg::receive_lag_compensation():" << __LINE__ << '\n'
             << "  scenario-time:" << end_t << '\n'
             << "      data-time:" << data_t << '\n'
             << " comp-time-step:" << this->compensate_dt << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received attribute data.
   if ( state_attr->is_received() ) {

      // Copy the current DynamicalEntity state over to the lag compensated state.
      load_lag_comp_data();
      Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

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

   } else {
      if ( debug ) {
         ostringstream errmsg;
         errmsg << "DynamicalEntityLagCompInteg::receive_lag_compensation(): No state data received.\n"
                << "\tvalue_changed: " << state_attr->is_changed()
                << "; locally owned: " << state_attr->locally_owned << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }
   if ( inertia_attr->is_received() ) {
      // Compute the inverse of the inertia matrix.  If this fails, the
      // inverse matrix will be set to all zeros.  This will zero out any
      // torque affects in the lag compensation dynamics.
      if ( dm_invert_symm( this->inertia_inv, this->inertia ) != TM_SUCCESS ) {
         message_publish( MSG_WARNING,
                          "SpaceFOM::DynamicalEntityLagCompInteg::receive_lag_compensation():%d ERROR: Singular inertia matrix! Inversion failed!\n",
                          __LINE__ );
         M_INIT( this->inertia_inv );
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
void DynamicalEntityLagCompBase::bypass_send_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the working data into the packing data.  This makes sure that the
   // current working state is packed.
   de_entity.pack_from_working_data();
   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityLagCompBase::bypass_receive_lag_compensation()
{
   // When lag compensation is present but disabled, we still need to copy
   // the packing data back into the working data.  This makes sure that the
   // working state is updated from the received packing data.
   de_entity.unpack_into_working_data();
   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityLagCompBase::unload_lag_comp_data()
{

   // Call the base class method.
   PhysicalEntityLagCompBase::unload_lag_comp_data();

   // Copy the current DynamicalEntity state over to the lag compensated state.
   this->de_entity.de_packing_data.mass      = this->mass;
   this->de_entity.de_packing_data.mass_rate = this->mass_rate;
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      this->de_entity.de_packing_data.force[iinc]  = this->force[iinc];
      this->de_entity.de_packing_data.torque[iinc] = this->torque[iinc];
      for ( int jinc = 0; jinc < 3; ++jinc ) {
         this->de_entity.de_packing_data.inertia[iinc][jinc]      = this->inertia[iinc][jinc];
         this->de_entity.de_packing_data.inertia_rate[iinc][jinc] = this->inertia_rate[iinc][jinc];
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityLagCompBase::load_lag_comp_data()
{
   double force_bdy[3];
   double torque_bdy[3];
   double accel_force_bdy[3];
   double ang_accel_torque_bdy[3];
   double I_omega[3];

   // Call the base class implementation.
   PhysicalEntityLagCompBase::load_lag_comp_data();

   // Copy the current DynamicalEntity state over to the lag compensated state.
   this->mass      = this->de_entity.de_packing_data.mass;
   this->mass_rate = this->de_entity.de_packing_data.mass_rate;
   for ( int iinc = 0; iinc < 3; ++iinc ) {
      this->force[iinc]  = this->de_entity.de_packing_data.force[iinc];
      this->torque[iinc] = this->de_entity.de_packing_data.torque[iinc];
      for ( int jinc = 0; jinc < 3; ++jinc ) {
         this->inertia[iinc][jinc]      = this->de_entity.de_packing_data.inertia[iinc][jinc];
         this->inertia_rate[iinc][jinc] = this->de_entity.de_packing_data.inertia_rate[iinc][jinc];
      }
   }

   //
   // Compute the translational dynamics.
   //
   // Transform the force into the body frame.
   body_wrt_struct.transform_vector( this->force, force_bdy );

   // Compute the force contribution to the translational acceleration.
   V_SCALE( accel_force_bdy, force_bdy, 1.0 / this->mass );

   // Compute the residual environmental acceleration.
   V_SUB( this->accel_env, this->accel, accel_force_bdy );

   //
   // Compute the rotational dynamics.
   //
   // Transform the torque into the body frame.
   body_wrt_struct.transform_vector( this->torque, torque_bdy );

   // External torque acceleration.
   MxV( ang_accel_torque_bdy, this->inertia_inv, torque_bdy );

   // Inertial rotational accelerations (omega X I omega).
   MxV( I_omega, this->inertia, this->lag_comp_data.ang_vel );
   V_CROSS( this->ang_accel_inertial, this->lag_comp_data.ang_vel, I_omega );

   // Compute the residual environmental angular acceleration.
   this->ang_accel_env[0] = this->ang_accel[0] - ang_accel_torque_bdy[0] - this->ang_accel_inertial[0];
   this->ang_accel_env[1] = this->ang_accel[1] - ang_accel_torque_bdy[1] - this->ang_accel_inertial[1];
   this->ang_accel_env[2] = this->ang_accel[2] - ang_accel_torque_bdy[2] - this->ang_accel_inertial[2];

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityLagCompBase::print_lag_comp_data( std::ostream &stream ) const
{

   PhysicalEntityLagCompBase::print_lag_comp_data( stream );

   // Set the print precision.
   stream.precision( 15 );

   // Print out the DynamicalEntity data.
   stream << "\tmass: " << this->mass << '\n';
   stream << "\tmass_rate: " << this->mass_rate << '\n';
   stream << "\tinertia: \n"
          << "\t\t" << this->inertia[0][0] << ", "
          << this->inertia[0][1] << ", "
          << this->inertia[0][2] << '\n'
          << "\t\t" << this->inertia[1][0] << ", "
          << this->inertia[1][1] << ", "
          << this->inertia[1][2] << '\n'
          << "\t\t" << this->inertia[2][0] << ", "
          << this->inertia[2][1] << ", "
          << this->inertia[2][2] << '\n';
   stream << "\tinertia rate: \n"
          << "\t\t" << this->inertia_rate[0][0] << ", "
          << this->inertia_rate[0][1] << ", "
          << this->inertia_rate[0][2] << '\n'
          << "\t\t" << this->inertia_rate[1][0] << ", "
          << this->inertia_rate[1][1] << ", "
          << this->inertia_rate[1][2] << '\n'
          << "\t\t" << this->inertia_rate[2][0] << ", "
          << this->inertia_rate[2][1] << ", "
          << this->inertia_rate[2][2] << '\n';
   stream << "\tforce: "
          << this->force[0] << ", "
          << this->force[1] << ", "
          << this->force[2] << '\n';
   stream << "\ttorque: "
          << this->torque[0] << ", "
          << this->torque[1] << ", "
          << this->torque[2] << '\n';

   // Return to the calling routine.
   return;
}
