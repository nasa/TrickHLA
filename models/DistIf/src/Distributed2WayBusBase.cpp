/**
@ingroup DistIf
@file    Distributed2WayBusBase.cpp
@brief   Distributed 2-Way Bus Base Interface implementation

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Distributed2WayBusBase.o}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

*/

#include "DistIf/include/Distributed2WayBusBase.hh"

using namespace DistIf;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] level   (--) Severity level of the message.
/// @param[in] message (--) Detailed message string.
///
/// @details  Constructs this notification message with the given values.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusNotification::Distributed2WayBusNotification(
   NotificationLevel const level,
   std::string const      &message )
   : mLevel( level ),
     mMessage( message )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Notification message default destructor.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusNotification::~Distributed2WayBusNotification()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in]  that  (--)  Object to be copied.
///
/// @details  Notification message copy constructor.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusNotification::Distributed2WayBusNotification(
   Distributed2WayBusNotification const &that )
   : mLevel( that.mLevel ),
     mMessage( that.mMessage )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in]  that  (--)  Object that this is to be assigned equal to.
///
/// @details  Notification message assignment operator.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusNotification &Distributed2WayBusNotification::operator=(
   Distributed2WayBusNotification const &that )
{
   if ( this != &that ) {
      this->mLevel   = that.mLevel;
      this->mMessage = that.mMessage;
   }
   return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default constructs this Distributed 2-Way Bus Base interface data.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusBaseInterfaceData::Distributed2WayBusBaseInterfaceData()
   : mFrameCount( 0 ),
     mFrameLoopback( 0 ),
     mDemandMode( false )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this Distributed 2-Way Bus Base interface data.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusBaseInterfaceData::~Distributed2WayBusBaseInterfaceData()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in]  that  (--)  Object that this is to be assigned equal to.
///
/// @details  Assigns values of this object's attributes to the given object's values.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusBaseInterfaceData &Distributed2WayBusBaseInterfaceData::operator=(
   Distributed2WayBusBaseInterfaceData const &that )
{
   if ( this != &that ) {
      mFrameCount    = that.mFrameCount;
      mFrameLoopback = that.mFrameLoopback;
      mDemandMode    = that.mDemandMode;
   }
   return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] inDataPtr  (--) Pointer to the interface incoming data object.
/// @param[in] outDataPtr (--) Pointer to the interface outgoing data object.
///
/// @details  Default constructs this Distributed 2-Way Bus Base Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusBase::Distributed2WayBusBase(
   Distributed2WayBusBaseInterfaceData *inDataPtr,
   Distributed2WayBusBaseInterfaceData *outDataPtr )
   : mIsPairMaster( false ),
     mInDataLastDemandMode( false ),
     mFramesSinceFlip( 0 ),
     mLoopLatency( 0 ),
     mForcedRole( NONE ),
     mNotifications(),
     mInDataPtr( inDataPtr ),
     mOutDataPtr( outDataPtr )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this Distributed 2-Way Bus Base Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
Distributed2WayBusBase::~Distributed2WayBusBase()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] isPairMaster (--) True if this is the master side of the interface, for tie-breaking.
///
/// @details  Initializes this Distributed 2-Way Bus Base Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
void Distributed2WayBusBase::initialize(
   bool const isPairMaster )
{
   /// - Initialize remaining state variables.  mForcedRole is not initialized, assuming the user
   ///   may have already set it.
   mIsPairMaster         = isPairMaster;
   mInDataLastDemandMode = false;
   mFramesSinceFlip      = 0;
   mLoopLatency          = 0;

   /// - Both sides start out in Supply mode by default.  Upon going to run, the sides will
   ///   negotiate one of them taking over the Demand role.
   mOutDataPtr->mDemandMode = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Update frame counters and loop latency measurement.
////////////////////////////////////////////////////////////////////////////////////////////////////
void Distributed2WayBusBase::updateFrameCounts()
{
   mOutDataPtr->mFrameCount++;
   mFramesSinceFlip++;
   mLoopLatency                = mOutDataPtr->mFrameCount - mInDataPtr->mFrameLoopback;
   mOutDataPtr->mFrameLoopback = mInDataPtr->mFrameCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[out] notification (--) Reference to the caller's message object to copy the message into.
///
/// @returns  unsigned int (--) Number of notifications remaining in the queue.
///
/// @details  Set the caller's supplied notification object equal to the tail of the queue and pops
///           that message off of the queue, reducing the queue size by one.  If the queue size is
///           already zero, then returns an empty message.
////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int Distributed2WayBusBase::popNotification(
   Distributed2WayBusNotification &notification )
{
   if ( mNotifications.size() > 0 ) {
      notification = mNotifications.back();
      mNotifications.pop_back();
   } else {
      notification.mLevel   = Distributed2WayBusNotification::NONE;
      notification.mMessage = "";
   }
   return mNotifications.size();
}
