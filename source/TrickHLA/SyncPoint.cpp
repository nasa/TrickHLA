/*!
@file TrickHLA/SyncPoint.cpp
@ingroup TrickHLA
@brief This class provides a sync-point implementation for storing and managing
TrickHLA synchronization points.

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
@trick_link_dependency{SyncPoint.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA includes.
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPoint::SyncPoint(
   std::wstring const &l )
   : label( l ),
     state( SYNC_PT_STATE_EXISTS ),
     label_chkpt( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPoint::~SyncPoint()
{
   this->clear_checkpoint_data_structures();
}

bool const SyncPoint::is_valid() const
{
   return ( ( this->state == SYNC_PT_STATE_EXISTS )
            || ( this->state == SYNC_PT_STATE_REGISTERED )
            || ( this->state == SYNC_PT_STATE_ANNOUNCED )
            || ( this->state == SYNC_PT_STATE_ACHIEVED )
            || ( this->state == SYNC_PT_STATE_SYNCHRONIZED ) );
}

bool const SyncPoint::exists() const
{
   return ( this->state == SYNC_PT_STATE_EXISTS );
}

bool const SyncPoint::is_registered() const
{
   return ( this->state == SYNC_PT_STATE_REGISTERED );
}

bool const SyncPoint::is_announced() const
{
   return ( this->state == SYNC_PT_STATE_ANNOUNCED );
}

bool const SyncPoint::is_achieved() const
{
   return ( this->state == SYNC_PT_STATE_ACHIEVED );
}

bool const SyncPoint::is_synchronized() const
{
   return ( this->state == SYNC_PT_STATE_SYNCHRONIZED );
}

bool const SyncPoint::is_error() const
{
   return ( ( this->state != SYNC_PT_STATE_EXISTS )
            && ( this->state != SYNC_PT_STATE_REGISTERED )
            && ( this->state != SYNC_PT_STATE_ANNOUNCED )
            && ( this->state != SYNC_PT_STATE_ACHIEVED )
            && ( this->state != SYNC_PT_STATE_SYNCHRONIZED ) );
}

std::string SyncPoint::to_string()
{
   string label_str;
   StringUtilities::to_string( label_str, this->label );

   string result = "[" + label_str + "] -- ";
   switch ( this->state ) {

      case SYNC_PT_STATE_ERROR:
         result += "SYNC_PT_STATE_ERROR";
         break;

      case SYNC_PT_STATE_EXISTS:
         result += "SYNC_PT_STATE_EXISTS";
         break;

      case SYNC_PT_STATE_REGISTERED:
         result += "SYNC_PT_STATE_REGISTERED";
         break;

      case SYNC_PT_STATE_ANNOUNCED:
         result += "SYNC_PT_STATE_ANNOUNCED";
         break;

      case SYNC_PT_STATE_ACHIEVED:
         result += "SYNC_PT_STATE_ACHIEVED";
         break;

      case SYNC_PT_STATE_SYNCHRONIZED:
         result += "SYNC_PT_STATE_SYNCHRONIZED";
         break;

      default:
         result += "SYNC_PT_STATE_UNKNOWN";
         break;
   }
   return result;
}

void SyncPoint::convert_to_checkpoint_data_structures()
{
   clear_checkpoint_data_structures();

   // Checkpointable copy of the label.
   this->label_chkpt = StringUtilities::ip_strdup_wstring( this->label );
}

void SyncPoint::restore_from_checkpoint_data_structures()
{
   // Update the label from the checkpointable c-string.
   StringUtilities::to_wstring( this->label, this->label_chkpt );
}

void SyncPoint::clear_checkpoint_data_structures()
{
   if ( this->label_chkpt != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->label_chkpt ) ) ) {
         send_hs( stderr, "SyncPoint::clear_checkpoint_data_structures():%d ERROR deleting Trick Memory for 'label_chkpt'\n", __LINE__ );
      }
      this->label_chkpt = NULL;
   }
}
