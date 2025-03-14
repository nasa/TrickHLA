/**
@ingroup DistIf
@file    FluidDistributedIfLagBuffer.cpp
@brief   Fluid distributed interface lag buffer

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
@trick_link_dependency{Distributed2WayBusFluid.o}
@trick_link_dependency{FluidDistributedIfLagBuffer.o}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

*/

#include "../include/FluidDistributedIfLagBuffer.hh"

using namespace DistIf;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default constructs this FluidDistributedIfLagBuffer.
////////////////////////////////////////////////////////////////////////////////////////////////////
FluidDistributedIfLagBuffer::FluidDistributedIfLagBuffer()
   : mDelayFrames( 0 ),
     mHead1( 0 ),
     mHead2( 0 ),
     mTail1( 0 ),
     mTail2( 0 ),
     mBuffer1(),
     mBuffer2(),
     mHeadIndex( 0 ),
     mTailIndex( 0 )
{
   return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this FluidDistributedIfLagBuffer.
///////////////////////////////////////////////////////////////////////////////////////////////////
FluidDistributedIfLagBuffer::~FluidDistributedIfLagBuffer()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Initializes this FluidDistributedIfLagBuffer.
////////////////////////////////////////////////////////////////////////////////////////////////////
void FluidDistributedIfLagBuffer::initialize(
   unsigned int const nBulk,
   unsigned int const nTc )
{
   for ( unsigned int i = 0; i < 10; ++i ) {
      // TODO this creates a lot of duplicate named dynamic array warnings from Trick
      mBuffer1[i].initialize( nBulk, nTc );
      mBuffer2[i].initialize( nBulk, nTc );
   }
   if ( mDelayFrames > 9 ) {
      mDelayFrames = 9;
   }
   mHeadIndex = mDelayFrames;
   updatePointers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the head & tail indexes, does the index wrap-around for the ring buffer, and
///           calls the interface data pointers to be updated.
////////////////////////////////////////////////////////////////////////////////////////////////////
void FluidDistributedIfLagBuffer::step()
{
   mHeadIndex++;
   mTailIndex++;
   if ( mHeadIndex > 9 ) {
      mHeadIndex = 0;
   }
   if ( mTailIndex > 9 ) {
      mTailIndex = 0;
   }
   updatePointers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Sets the interface Head & Tail data pointers to the current head & tail of the ring
///           buffers.
////////////////////////////////////////////////////////////////////////////////////////////////////
void FluidDistributedIfLagBuffer::updatePointers()
{
   mHead1 = &mBuffer1[mHeadIndex];
   mHead2 = &mBuffer2[mHeadIndex];
   mTail1 = &mBuffer1[mTailIndex];
   mTail2 = &mBuffer2[mTailIndex];
}
