/*!
@ingroup Sine
@file models/sine/src/SineData.cpp
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
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{sine/src/SineData.cpp}
@trick_link_dependency{sine/src/SinePacking.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan-AEU, DSES, January 2003, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdlib>
#include <math.h>
#include <sstream>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/integrator_c_intf.h"
#include "trick/message_type.h"
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "sine/include/SineData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SineData::SineData()
   : time( 0.0 ),
     value( 0.0 ),
     dvdt( 0.0 ),
     phase( 0.0 ),
     freq( 2.0 * M_PI ),
     amp( 1.0 ),
     tol( 0.001 ),
     name( nullptr )
{
   // We don't want a nullptr name by default (Trick Memory Manager allocated).
   set_name( "" );

   // Compute the value.
   compute_value( time );

   // Compute the initial derivative value.
   compute_derivative( time );
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
     name( nullptr )
{
   // We don't want a nullptr name by default (Trick Memory Manager allocated).
   set_name( "" );

   // Compute the value.
   compute_value( time );

   // Compute the initial derivative value.
   compute_derivative( time );
}

/*!
 * @job_class{shutdown}
 */
SineData::~SineData()
{
   // Make sure we free the memory used by the name.
   if ( name != nullptr ) {
      if ( trick_MM->delete_var( static_cast< void * >( name ) ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, "Failed to delete Trick Memory for 'name'\n", MSG_WARNING );
      }
      name = nullptr;
   }
}

/*!
 * @brief Set the name of the sine wave object.
 * @param new_name The name of the sine wave object.
 */
void SineData::set_name( char const *new_name )
{
   if ( new_name != this->name ) {
      if ( this->name != nullptr ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
            DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, "Failed to delete Trick Memory for 'name'\n" );
         }
      }
      if ( new_name != nullptr ) {
         this->name = trick_MM->mm_strdup( new_name );
         if ( this->name == nullptr ) {
            DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, "Could not allocate Trick Memory for 'name'\n" );
         }
      } else {
         this->name = nullptr;
      }
   }

   // We don't want a nullptr name by default (Trick Memory Manager allocated).
   if ( this->name == nullptr ) {
      this->name = trick_MM->mm_strdup( "" );
      if ( this->name == nullptr ) {
         DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, "Could not allocate Trick Memory for 'name'\n" );
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void SineData::copy_data(
   SineData const *orig ) // IN: -- Original source data to copy from.
{
   set_name( orig->get_name() );
   set_time( orig->get_time() );
   set_value( orig->get_value() );
   set_derivative( orig->get_derivative() );
   set_phase( orig->get_phase() );
   set_frequency( orig->get_frequency() );
   set_amplitude( orig->get_amplitude() );
   set_tolerance( orig->get_tolerance() );
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
   double const t )
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
   double const t )
{
   // Set the model time.
   time = t;

   // Compute the derivative.
   compute_derivative();
}

void SineData::adjust_phase() // RETURN: -- None.
{
   double       test_value;
   double       test_deriv;
   double const old_phase = this->phase;

   // Compute the test_value.
   test_value = amp * sin( ( freq * time ) + phase );

   // Compute the test_value.
   test_deriv = freq * amp * cos( ( freq * time ) + phase );

   // Check for phase adjustment.
   if ( ( M_ABS( test_value - value ) > ( tol / amp ) )
        || ( ( ( test_deriv > 0.0 ) ? 1 : 0 ) != ( ( dvdt > 0.0 ) ? 1 : 0 ) ) ) {

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

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream msg;
         msg << "Adjusting phase, old:" << old_phase << ", new:" << phase << "\n";
         DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__, msg.str() );
      }
   }
}

void SineData::adjust_phase(
   double const t )
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
   // Load the current sine state.
   load_state( &value, nullptr ); // cppcheck-suppress [varFuncNullUB,unmatchedSuppression]

   // Load the current state derivative.
   load_deriv( &dvdt, nullptr ); // cppcheck-suppress [varFuncNullUB,unmatchedSuppression]

   // Call the Trick integration service.
   int const ipass = integrate();

   // Unload the new propagated state value.
   unload_state( &value, nullptr ); // cppcheck-suppress [varFuncNullUB,unmatchedSuppression]

   // Return the intermediate step ID.
   return ( ipass );
}
