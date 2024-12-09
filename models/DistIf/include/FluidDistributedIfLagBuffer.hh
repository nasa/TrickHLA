#ifndef FluidDistributedIfLagBuffer_EXISTS
#define FluidDistributedIfLagBuffer_EXISTS

/**
@ingroup DistIf
@file    FluidDistributedIfLagBuffer.hh
@brief   Distributed2WayBusFluid Data Lag Buffer Pair

@details This is a ring buffer of Distributed2WayBusFluidInterfaceData for both
data directions between a pair of Distributed2WayBusFluid interfaces.  This is
used to create a desired amount of round-trip data lag in the interface, for
testing stability, mass conservation, etc.  This shouldn't be used in an actual
project.

Use the sim bus to write data to the Head data pointers, and read from the Tail
pointers.

Set mDelayFrames in the input file to dial in the desired total lag.  The total
round-trip loop lag, as measured by the pair master link, will be
2 * mDelayFrames frames.

The step function should be called before the models that interface with this.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{DistIf}

@tldh
@trick_link_dependency{../src/FluidDistributedIfLagBuffer.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

@{
*/

#include <string>

#include "Distributed2WayBusFluid.hh"

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed2WayBusFluid Data Lag Buffer Pair
///
/// @details  Refer to class Purpose.
////////////////////////////////////////////////////////////////////////////////////////////////////
class FluidDistributedIfLagBuffer
{
  public:
   unsigned int                          mDelayFrames; /**< (1) Number of frames to delay in each direction */
   Distributed2WayBusFluidInterfaceData *mHead1;       /**< (1) Head of buffer 1 for writing by the interface. */
   Distributed2WayBusFluidInterfaceData *mHead2;       /**< (1) Head of buffer 2 for writing by the interface. */
   Distributed2WayBusFluidInterfaceData *mTail1;       /**< (1) Tail of buffer 1 for reading by the interface. */
   Distributed2WayBusFluidInterfaceData *mTail2;       /**< (1) Tail of buffer 2 for reading by the interface. */
   /// @brief  Default constructs this FluidDistributedIfLagBuffer.
   FluidDistributedIfLagBuffer();
   /// @brief  Default destructs this FluidDistributedIfLagBuffer.
   virtual ~FluidDistributedIfLagBuffer();
   /// @brief  Initializes this FluidDistributedIfLagBuffer.
   void initialize( unsigned int const nBulk = 6, unsigned int const nTc = 0 );
   /// @brief  Updates the head and tail indeces for the buffers.
   void step();

  private:
   Distributed2WayBusFluidInterfaceData mBuffer1[10]; /**< (1) Buffer for direction 1. */
   Distributed2WayBusFluidInterfaceData mBuffer2[10]; /**< (1) Buffer for direction 2. */
   unsigned int                         mHeadIndex;   /**< (1) Index of the head of the buffers. */
   unsigned int                         mTailIndex;   /**< (1) Index of the tail of the buffers. */
   /// @brief  Sets the interface Head & Tail data pointers to the correct buffer locations.
   void updatePointers();
   /// @brief  Copy constructor unavailable since declared private and not implemented.
   FluidDistributedIfLagBuffer( FluidDistributedIfLagBuffer const & );
   /// @brief  Assignment operator unavailable since declared private and not implemented.
   FluidDistributedIfLagBuffer &operator=( FluidDistributedIfLagBuffer const & );
};

} // namespace DistIf

/// @}

#endif
