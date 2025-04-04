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
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPoint::SyncPoint()
   : label(),
     state( TrickHLA::SYNC_PT_STATE_KNOWN ),
     user_supplied_tag( NULL, 0 ),
     label_chkpt( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPoint::SyncPoint(
   wstring const &lbl )
   : label( lbl ),
     state( TrickHLA::SYNC_PT_STATE_KNOWN ),
     user_supplied_tag( NULL, 0 ),
     label_chkpt( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPoint::~SyncPoint()
{
   return;
}

bool const SyncPoint::is_valid() const
{
   return ( ( this->state == TrickHLA::SYNC_PT_STATE_KNOWN )
            || ( this->state == TrickHLA::SYNC_PT_STATE_REGISTERED )
            || ( this->state == TrickHLA::SYNC_PT_STATE_ANNOUNCED )
            || ( this->state == TrickHLA::SYNC_PT_STATE_ACHIEVED )
            || ( this->state == TrickHLA::SYNC_PT_STATE_SYNCHRONIZED ) );
}

bool const SyncPoint::is_known() const
{
   return ( this->state == TrickHLA::SYNC_PT_STATE_KNOWN );
}

bool const SyncPoint::is_registered() const
{
   return ( this->state == TrickHLA::SYNC_PT_STATE_REGISTERED );
}

bool const SyncPoint::is_announced() const
{
   return ( this->state == TrickHLA::SYNC_PT_STATE_ANNOUNCED );
}

bool const SyncPoint::is_achieved() const
{
   return ( this->state == TrickHLA::SYNC_PT_STATE_ACHIEVED );
}

bool const SyncPoint::is_synchronized() const
{
   return ( this->state == TrickHLA::SYNC_PT_STATE_SYNCHRONIZED );
}

bool const SyncPoint::is_error() const
{
   return ( ( this->state != TrickHLA::SYNC_PT_STATE_KNOWN )
            && ( this->state != TrickHLA::SYNC_PT_STATE_REGISTERED )
            && ( this->state != TrickHLA::SYNC_PT_STATE_ANNOUNCED )
            && ( this->state != TrickHLA::SYNC_PT_STATE_ACHIEVED )
            && ( this->state != TrickHLA::SYNC_PT_STATE_SYNCHRONIZED ) );
}

RTI1516_USERDATA const SyncPoint::encode_user_supplied_tag()
{
   return this->user_supplied_tag;
}

void SyncPoint::decode_user_supplied_tag(
   RTI1516_USERDATA const &supplied_tag )
{
   this->user_supplied_tag = supplied_tag;
}

std::string SyncPoint::to_string()
{
   string label_str;
   StringUtilities::to_string( label_str, this->label );

   string result = "[" + label_str + "] -- ";
   switch ( this->state ) {

      case TrickHLA::SYNC_PT_STATE_ERROR:
         result += "SYNC_PT_STATE_ERROR";
         break;

      case TrickHLA::SYNC_PT_STATE_KNOWN:
         result += "SYNC_PT_STATE_KNOWN";
         break;

      case TrickHLA::SYNC_PT_STATE_REGISTERED:
         result += "SYNC_PT_STATE_REGISTERED";
         break;

      case TrickHLA::SYNC_PT_STATE_ANNOUNCED:
         result += "SYNC_PT_STATE_ANNOUNCED";
         break;

      case TrickHLA::SYNC_PT_STATE_ACHIEVED:
         result += "SYNC_PT_STATE_ACHIEVED";
         break;

      case TrickHLA::SYNC_PT_STATE_SYNCHRONIZED:
         result += "SYNC_PT_STATE_SYNCHRONIZED";
         break;

      default:
         result += "SYNC_PT_STATE_UNKNOWN";
         break;
   }
   return result;
}

void SyncPoint::encode_checkpoint()
{
   free_checkpoint();

   // Checkpointable copy of the label.
   this->label_chkpt = StringUtilities::ip_strdup_wstring( this->label );
}

void SyncPoint::decode_checkpoint()
{
   // Update the label from the checkpointable c-string.
   StringUtilities::to_wstring( this->label, this->label_chkpt );
}

void SyncPoint::free_checkpoint()
{
   if ( this->label_chkpt != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->label_chkpt ) ) ) {
         message_publish( MSG_WARNING, "SyncPoint::free_checkpoint():%d WARNING failed to delete Trick Memory for 'label_chkpt'\n", __LINE__ );
      }
      this->label_chkpt = NULL;
   }
}
