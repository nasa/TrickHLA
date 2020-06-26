/*!
@file models/sine/src/SineData.cpp
@ingroup TrickHLAModel
@brief This class is the working class for the Sine HLA/RTI example problem.

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
@trick_link_dependency{sine/src/SineData.o}
@trick_link_dependency{sine/src/SinePacking.o}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan-AEU, DSES, January 2003, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/integrator_c_intf.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

// Model include files.
#include "../include/SineData.hh"

using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SineData::SineData()
   : time( 0.0 ),
     value( 0.0 ),
     dvdt( 0.0 ),
     phase( 0.0 ),
     freq( 2 * M_PI ),
     amp( 1.0 ),
     tol( 0.001 ),
     name( NULL )
{
   // Compute the value.
   this->compute_value( time );

   // Compute the initial derivative value.
   this->compute_derivative( time );
}

/*!
 * @job_class{initialization}
 */
SineData::SineData(
   double phi,
   double omega,
   double mag )
   : time( 0.0 ),
     value( 0.0 ),
     dvdt( 0.0 ),
     phase( phi ),
     freq( omega ),
     amp( mag ),
     tol( 0.001 ),
     name( NULL )
{
   // Compute the value.
   this->compute_value( time );

   // Compute the initial derivative value.
   this->compute_derivative( time );
}

/*!
 * @job_class{shutdown}
 */
SineData::~SineData()
{
   // Make sure we free the memory used by the name.
   if ( name != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( name ) ) {
         TMM_delete_var_a( name );
      }
      name = static_cast< char * >( NULL );
   }
}

/*!
 * @job_class{scheduled}
 */
void SineData::copy_data(
   SineData *orig ) // IN:     -- Orginal source data to copy.
{
   // Use the default assignment operator to copy.
   *this = *orig;
}

/*!
 * @job_class{scheduled}
 */
void SineData::compute_value()
{
   // Compute the value.
   value = amp * sin( ( freq * time ) + phase );
}

/*!
 * @job_class{scheduled}
 */
void SineData::compute_value(
   double t )
{
   // Set the current model time.
   time = t;

   // Compute the value.
   compute_value();
}

/*!
 * @job_class{derivative}
 */
void SineData::compute_derivative()
{
   // Compute the derivative.
   dvdt = freq * amp * cos( ( freq * time ) + phase );
}

/*!
 * @job_class{derivative}
 */
void SineData::compute_derivative(
   double t )
{

   // Set the model time.
   time = t;

   // Compute the derivative.
   compute_derivative();
}

void SineData::adjust_phase() // RETURN: -- None.
{
   double test_value;
   double test_deriv;
   double old_phase = this->phase;

   // Compute the test_value.
   test_value = amp * sin( ( freq * time ) + phase );

   // Compute the test_value.
   test_deriv = freq * amp * cos( ( freq * time ) + phase );

   // Check for phase adjustment.
   if ( ( M_ABS( test_value - value ) > ( tol / amp ) ) || ( ( ( test_deriv > 0.0 ) ? 1 : 0 ) != ( ( dvdt > 0.0 ) ? 1 : 0 ) ) ) {

      // Sine value does not match expected value so adjust phase to match.
      phase = asin( value / amp );
      if ( dvdt < 0.0 ) {
         if ( phase > 0.0 ) {
            phase = M_PI - phase;
         } else {
            phase = -M_PI - phase;
         }
      }
      phase -= fmod( ( freq * time ), ( 2.0 * M_PI ) );
      send_hs( stdout, "Adjusting phase, old=%f, new=%f\n", old_phase, phase );
   }
}

void SineData::adjust_phase(
   double t )
{
   // Set the model time.
   time = t;

   // Compute the derivative.
   adjust_phase();
}

/*!
 * @job_class{integration}
 */
int SineData::integration()
{
   int ipass;

   // Load the current sine state.
   load_state( &value, NULL ); // cppcheck-suppress [varFuncNullUB]

   // Load the current state derivative.
   load_deriv( &dvdt, NULL ); // cppcheck-suppress [varFuncNullUB]

   // Call the Trick integration service.
   ipass = integrate();

   // Unload the new propagated state value.
   unload_state( &value, NULL ); // cppcheck-suppress [varFuncNullUB]

   // Return the intermediate step ID.
   return ( ipass );
}
