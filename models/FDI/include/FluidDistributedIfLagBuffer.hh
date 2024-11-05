#ifndef FluidDistributedIfLagBuffer_EXISTS
#define FluidDistributedIfLagBuffer_EXISTS

/**
@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

PURPOSE: (This is a ring buffer of Distributed2WayBusFluidInterfaceData for both data directions between a
          pair of Distributed2WayBusFluid interfaces.  This is used to create a desired amount of round-
          trip data lag in the interface, for testing stability, mass conservation, etc.  This
          shouldn't be used in an actual project.

          Use the sim bus to write data to the Head data pointers, and read from the Tail pointers.

          Set mDelayFrames in the input file to dial in the desired total lag.  The total round-trip
          loop lag, as measured by the pair master link, will be 2 * mDelayFrames frames.

          The step function should be called before the models that interface with this.)

REFERENCE:
- (TBD)

ASSUMPTIONS AND LIMITATIONS:
- (TBD)

LIBRARY DEPENDENCY:
- ((FluidDistributedIfLagBuffer.o))

PROGRAMMERS:
- ((Jason Harvey) (CACI) (March 2019) (Initial))

@{
*/

#include <string>

#include "Distributed2WayBusFluid.hh"

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
   void initialize( const unsigned int nBulk = 6, const unsigned int nTc = 0 );
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
   FluidDistributedIfLagBuffer( const FluidDistributedIfLagBuffer & );
   /// @brief  Assignment operator unavailable since declared private and not implemented.
   FluidDistributedIfLagBuffer &operator=( const FluidDistributedIfLagBuffer & );
};

/// @}

#endif
