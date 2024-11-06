/**
@file     CabinAtmo.cpp
@brief    Simple Cabin Atmosphere Model implementation

@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

LIBRARY DEPENDENCY:
   ((CabinAtmoVolume.o))
*/

#include <cfloat>
#include <iostream>

#include "../include/CabinAtmo.hh"

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default constructs this CabinAtmo Configuration Data.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConfigData::CabinAtmoConfigData()
   : mCompoundCp(),
     mCabinMixture(),
     mVestibuleMixture(),
     mImvDuctMixture(),
     mCabin( 10.0, 294.261, 101325.0, mCabinMixture, mCompoundCp, true ),
     mVestibule( 1.0, 294.261, 103325.0, mVestibuleMixture, mCompoundCp, true ),
     mImvDuct( 0.05, 294.261, 102325.0, mImvDuctMixture, mCompoundCp, true ),
     // ISS IMV fan:
     // dead-head     413 Pa @ 0 mol/s
     // design point  250 Pa @ 2.737 mol/s
     // max (linear)    0 Pa @ 6.935 mol/s
     mImvFanMaxQ( 6.935 ),
     mImvFanMaxDp( 413.0 ),
     mHatchG( 1.0 ),
     mMpevG( 1.0e-5 ),
     mGrillValveG( 0.05 ),
     mImvValveG( 0.0475 ), // tuned to make 2.78 mol/s when combined, with either IMV fan on
     mHatchOpen( true ),
     mMpevOpen( true ),
     mImvValveOpen( true ),
     mGrillValveOpen( true ),
     mImvFanOn( true )
{
   /// - Default specific heats of N2, O2, H2O, CO2, calculated from Cp = h/T, with h as the NIST
   ///   value for typical partial pressure in air at 294.261 K (70 F).  These values of Cp differ
   ///   from the NIST values, but allow us to model the ideal calorically perfect gas as h = Cp*T.
   mCompoundCp[0] = 29.0613;
   mCompoundCp[1] = 29.1038;
   mCompoundCp[2] = 155.515;
   mCompoundCp[3] = 75.3039;

   /// - Default initial air mixtures
   mCabinMixture[0] = 0.787; // N2
   mCabinMixture[1] = 0.20;  // O2
   mCabinMixture[2] = 0.01;  // H2O
   mCabinMixture[3] = 0.003; // CO2

   mVestibuleMixture[0] = 0.787; // N2
   mVestibuleMixture[1] = 0.20;  // O2
   mVestibuleMixture[2] = 0.01;  // H2O
   mVestibuleMixture[3] = 0.003; // CO2

   mImvDuctMixture[0] = 0.787; // N2
   mImvDuctMixture[1] = 0.20;  // O2
   mImvDuctMixture[2] = 0.01;  // H2O
   mImvDuctMixture[3] = 0.003; // CO2
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Destructs this CabinAtmo Configuration Data.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConfigData::~CabinAtmoConfigData()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default constructs this CabinAtmo.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmo::CabinAtmo(
   std::string const         &name,
   CabinAtmoConfigData const *config )
   : mConfig( config ),
     mCabin( name + ".mCabin" ),
     mVestibule( name + ".mVestibule" ),
     mImvDuct( name + ".mImvDuct" ),
     mHatchOpen( false ),
     mMpevOpen( false ),
     mImvValveOpen( false ),
     mGrillValveOpen( false ),
     mImvFanOn( false ),
     mTimestep( 0.0 ),
     mImvFanSourceP( 0.0 ),
     mImvFanDp( 0.0 ),
     mConserveParams(),
     mHatchFlow( 0.0 ),
     mMpevFlow( 0.0 ),
     mImvFlow( 0.0 ),
     mGrillValveFlow( 0.0 ),
     mA(),
     mSourceVector(),
     mSolutionVector(),
     mAinv(),
     mName( name ),
     mInitFlag( false )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this CabinAtmo model.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmo::~CabinAtmo()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Initializes this CabinAtmo model.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::initialize()
{
   mInitFlag = false;
   validateConfig();
   mCabin.initialize( mConfig->mCabin );
   mVestibule.initialize( mConfig->mVestibule );
   mImvDuct.initialize( mConfig->mImvDuct );
   mHatchOpen      = mConfig->mHatchOpen;
   mMpevOpen       = mConfig->mMpevOpen;
   mImvValveOpen   = mConfig->mImvValveOpen;
   mGrillValveOpen = mConfig->mGrillValveOpen;
   mImvFanOn       = mConfig->mImvFanOn;
   mTimestep       = 0.0;
   mImvFanDp       = 0.0;
   updateConservation();
   mInitFlag = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Does some valid range checks on the configuration data, outputs error messages.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::validateConfig() const
{
   if ( mConfig->mCabin.mVolume < mConfig->mVestibule.mVolume ) {
      std::cout << "ERROR: " << mName << " cabin volume < vestibule volume!" << std::endl;
   }
   if ( mConfig->mVestibule.mVolume < mConfig->mImvDuct.mVolume ) {
      std::cout << "ERROR: " << mName << " vestibule volume < IMV duct volume!" << std::endl;
   }
   if ( mConfig->mImvFanMaxQ < DBL_EPSILON ) {
      std::cout << "ERROR: " << mName << " IMV fan max Q < DBL_EPSILON!" << std::endl;
   }
   if ( mConfig->mImvFanMaxDp < DBL_EPSILON ) {
      std::cout << "ERROR: " << mName << " IMV fan max dP < DBL_EPSILON!" << std::endl;
   }
   if ( mConfig->mHatchG < 0.0 ) {
      std::cout << "ERROR: " << mName << " Hatch conductance < zero!" << std::endl;
   }
   if ( mConfig->mMpevG < 0.0 ) {
      std::cout << "ERROR: " << mName << " MPEV conductance < zero!" << std::endl;
   }
   if ( mConfig->mGrillValveG < 0.0 ) {
      std::cout << "ERROR: " << mName << " Grill valve conductance < zero!" << std::endl;
   }
   if ( mConfig->mImvValveG < 0.0 ) {
      std::cout << "ERROR: " << mName << " IMV valve conductance < zero!" << std::endl;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] dt (s) Timestep of integration.
///
/// @details  This is the main model step.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::step(
   double const dt )
{
   if ( not mInitFlag ) {
      std::cout << "ERROR: " << mName << " updated without being initialized." << std::endl;
      mTimestep = 0.0;
      return;
   }
   mTimestep = dt;

   /// - Fluid Distributed Interface, local model operations Steps 2-6:
   ///   For each of the interface volumes (vestibule and IMV duct), compute the source/sink
   ///   pressure and call their function to update their Fluid Distributed Interface prior to the
   ///   main model update.
   ///
   ///   Vestibule source/sink is the cabin if connected, else the IMV duct if connected, else
   ///   the vestibule itself.
   double demandSideP = 0.0;
   if ( mHatchOpen or mMpevOpen ) {
      demandSideP = mCabin.mPressure;
   } else if ( mGrillValveOpen ) {
      demandSideP = mImvDuct.mPressure;
   } else {
      demandSideP = mVestibule.mPressure;
   }
   mVestibule.updateIfPre( mTimestep, demandSideP );

   /// - IMV duct source/sink is the cabin if connected, else the vestibule if connected, else
   ///   the IMV duct itself.
   demandSideP = mImvDuct.mPressure;
   if ( mImvValveOpen ) {
      demandSideP = mCabin.mPressure;
   } else if ( mGrillValveOpen ) {
      demandSideP = mVestibule.mPressure;
   }
   mImvDuct.updateIfPre( mTimestep, demandSideP );

   /// - The cabin volume's interface isn't used and not updated.

   /// - Fluid Distributed Interface, local model operations Step 7: model does its main update.
   updateModel();

   /// - Fluid Distributed Interface, local model operations Step 8: model calculates its
   ///   capacitance at the interfaces.
   computeCapacitance();

   /// - Fluid Distributed Interface, local model operations Steps 9-12:
   ///   Update the interface volume's distributed interface steps after the main model update.
   mImvDuct.updateIfPost();
   mVestibule.updateIfPost();

   /// - Compute some mole and energy totals for verifying conservation laws.
   updateConservation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  This is the local model update, propagates the volume states forward by one
///           integration time step.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::updateModel()
{
   updateFan();
   buildSOE();
   invertMatrix();
   solvePressures( mSolutionVector, mSourceVector );
   computeFlows();
   transportFlows();
   updatePressures();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the IMV fan model, which we model as a pressure source in the SOE from which
///           flow results in the SOE solution.  The fan source pressure is modeled as a line
///           its dead-head (stall) pressure at zero flow, to a maximum flow at zero pressure.  The
///           default configuration should match the dead-head and nominal flow points for a typical
///           ISS fan, but the maximum flow rate will be unrealistically high because of this linear
///           fan curve simplification.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::updateFan()
{
   double newFanP = 0.0;
   if ( mImvFanOn and mImvValveOpen ) {
      newFanP = mConfig->mImvFanMaxDp * ( 1.0 - limitRange( 0.0, mImvFlow / mConfig->mImvFanMaxQ, 1.0 ) );
   }

   /// - To help stability and model fan acceleration, put a filter on fan pressure change.
   mImvFanSourceP += 0.1 * ( newFanP - mImvFanSourceP );
   if ( mImvFanSourceP < FLT_EPSILON ) {
      mImvFanSourceP = 0.0;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  This builds the system of equations for the model.  The system is [A]{x} = {b}, where
///           [A] is the admittance matrix, {x} is the new pressures to be solved for, and {b} is
///           the source vector.  Each of the 3 model volumes has a row in the system.  The units of
///           this system is flow rate (mol/s) = {b} = A (mol/s/Pa) * P (Pa).
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::buildSOE()
{
   /// - Local capacitance of cabin volumes (cabin, vestibule, duct).  Capacitance describes the
   ///   change in moles (flow in/out) over the change in pressure, C = dN/dP (mol/Pa).  We can
   ///   write an equation for the flow in/out of the volume as dN/dt (mol/s) = C * dP/dt.  Now
   ///   here we compute the C/dt term, which goes into [A], and C/dt * P(t-1) goes into {b}, then
   ///   the new P(t0) will be solved for by {x} = [A]^-1 * {b}.  In matrix form for the SOE:
   ///     [ C/dt ] * { P(t0) } = { P(t-1)*C/dt }   (volume's row)
   double C0dt = mCabin.computeCapacitance() / mTimestep;
   double C1dt = mVestibule.computeCapacitance() / mTimestep;
   double C2dt = mImvDuct.computeCapacitance() / mTimestep;

   /// - Interfaces in Demand role have their pressure constrained, so we replace the local volume
   ///   capacitance with the capacitance value from the Supply model.
   if ( mVestibule.mIf.isInDemandRole() and mVestibule.mIfDataValid ) {
      C1dt = mVestibule.mIf.mInData.mCapacitance / mTimestep;
   }
   if ( mImvDuct.mIf.isInDemandRole() and mImvDuct.mIfDataValid ) {
      C2dt = mImvDuct.mIf.mInData.mCapacitance / mTimestep;
   }

   /// - Conductance of flow paths between volumes (cabin-vest, cabin-duct, vest-duct).  We model
   ///   flow through these paths as directly proportional to the delta-pressure, and we call the
   ///   constant of this proportionality as 'conductance': dN/dt = G * dP.  In matrix form for
   ///   the SOE:
   ///     [ G  -G] {P0} = {0}   (inlet volume row for positive flow direction)
   ///     [-G   G] {P1} = {0}   (outlet volume row)
   double G01;
   if ( mHatchOpen and mMpevOpen ) {
      G01 = mConfig->mHatchG + mConfig->mMpevG;
   } else if ( mHatchOpen ) {
      G01 = mConfig->mHatchG;
   } else if ( mMpevOpen ) {
      G01 = mConfig->mMpevG;
   } else {
      G01 = 0.0;
   }

   double G02;
   if ( mImvValveOpen ) {
      G02 = mConfig->mImvValveG;
   } else {
      G02 = 0.0;
   }

   double G12;
   if ( mGrillValveOpen ) {
      G12 = mConfig->mGrillValveG;
   } else {
      G12 = 0.0;
   }

   /// - Load the admittance matrix with the above capacitances and conductances.
   mA[0][0] = C0dt + G01 + G02;
   mA[1][1] = C1dt + G01 + G12;
   mA[2][2] = C2dt + G02 + G12;
   mA[0][1] = -G01;
   mA[0][2] = -G02;
   mA[1][2] = -G12;
   mA[1][0] = mA[0][1];
   mA[2][0] = mA[0][2];
   mA[2][1] = mA[1][2];

   /// - Add the above capacitance terms to the source vector.
   mSourceVector[0] = C0dt * mCabin.mPressure;
   mSourceVector[1] = C1dt * mVestibule.mPressure;
   mSourceVector[2] = C2dt * mImvDuct.mPressure;

   /// - Add the fan's flow source to source vector.  The fan's pressure source creates a flow
   ///   through the IMV valve's conductance (valve and fan are in series) as
   ///   dN/dt = G * dP between the cabin and IMV duct volumes.  In matrix form for the SOE:
   ///     [0  0] {P0} = {-dP*G}     (cabin row)
   ///     [0  0] {P2} = { dP*G}     (IMV duct row)
   mSourceVector[0] -= mImvFanSourceP * G02;
   mSourceVector[2] += mImvFanSourceP * G02;

   /// - Add the Fluid Distributed Interface Demand flow rates to the volumes when they are in
   ///   the Supply role.  We flip the Demand flow sign because positive Demand flow pulls from
   ///   the Supply volume to send to the Demand side.
   if ( not mVestibule.mIf.isInDemandRole() ) {
      mSourceVector[1] -= mVestibule.mIfFlow.mFlowRate;
   }
   if ( not mImvDuct.mIf.isInDemandRole() ) {
      mSourceVector[2] -= mImvDuct.mIfFlow.mFlowRate;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Computes the inverse of the admittance matrix.  The admittance matrix is 3x3,
///           symmetric, positive-definite.  The determinant is always non-zero because A[0][0]
///           is always non-zero (from non-zero cabin volume).
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::invertMatrix()
{
   mAinv[0][0] = mA[2][2] * mA[1][1] - mA[1][2] * mA[1][2];
   mAinv[0][1] = mA[0][2] * mA[1][2] - mA[2][2] * mA[0][1];
   mAinv[0][2] = mA[0][1] * mA[1][2] - mA[0][2] * mA[1][1];
   mAinv[1][1] = mA[2][2] * mA[0][0] - mA[0][2] * mA[0][2];
   mAinv[1][2] = mA[0][1] * mA[0][2] - mA[0][0] * mA[1][2];
   mAinv[2][2] = mA[0][0] * mA[1][1] - mA[0][1] * mA[0][1];

   double det = mA[0][0] * mAinv[0][0]
                + mA[0][1] * mAinv[0][1]
                + mA[0][2] * mAinv[0][2];

   mAinv[0][0] /= det;
   mAinv[0][1] /= det;
   mAinv[0][2] /= det;
   mAinv[1][1] /= det;
   mAinv[1][2] /= det;
   mAinv[2][2] /= det;

   mAinv[1][0] = mAinv[0][1];
   mAinv[2][0] = mAinv[0][2];
   mAinv[2][1] = mAinv[1][2];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[out] x (Pa)    The computed pressures of the solution.
/// @param[in]  b (mol/s) The source vector.
///
/// @details  Computes {x} = [A]^-1 * {b}, which is the solution of the SOE, with volume pressures
///           propagated to the next time step as the integral of flows to/from the volume's
///           capacitance.  The pressure and source vectors are arguments, rather than being
///           operated on directly, so this function can compute alternate solutions for {x} such as
///           for modified {b} used to compute capacitances, etc.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::solvePressures(
   double       *x,
   double const *b )
{
   x[0] = mAinv[0][0] * b[0] + mAinv[0][1] * b[1] + mAinv[0][2] * b[2];
   x[1] = mAinv[1][0] * b[0] + mAinv[1][1] * b[1] + mAinv[1][2] * b[2];
   x[2] = mAinv[2][0] * b[0] + mAinv[2][1] * b[1] + mAinv[2][2] * b[2];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Computes the flows between the volumes corresponding to the latest pressure solution.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::computeFlows()
{
   /// - From the conductive flow equations described in buildSOE(), dN/dt = G*dP for the valves
   ///   and hatch.  The IMV flow is special since it includes the inline fan - its source pressure
   ///   contributes to the flow.
   if ( mHatchOpen ) {
      mHatchFlow = mConfig->mHatchG * ( mSolutionVector[0] - mSolutionVector[1] );
      if ( std::abs( mHatchFlow ) < DBL_EPSILON ) {
         mHatchFlow = 0.0;
      }
   } else {
      mHatchFlow = 0.0;
   }
   if ( mMpevOpen ) {
      mMpevFlow = mConfig->mMpevG * ( mSolutionVector[0] - mSolutionVector[1] );
      if ( std::abs( mMpevFlow ) < DBL_EPSILON ) {
         mMpevFlow = 0.0;
      }
   } else {
      mMpevFlow = 0.0;
   }
   if ( mImvValveOpen ) {
      mImvFlow = mConfig->mImvValveG * ( mSolutionVector[0] - mSolutionVector[2] + mImvFanSourceP );
      if ( std::abs( mImvFlow ) < DBL_EPSILON ) {
         mImvFlow = 0.0;
      }
   } else {
      mImvFlow = 0.0;
   }

   if ( mGrillValveOpen ) {
      mGrillValveFlow = mConfig->mGrillValveG * ( mSolutionVector[1] - mSolutionVector[2] );
      if ( std::abs( mGrillValveFlow ) < DBL_EPSILON ) {
         mGrillValveFlow = 0.0;
      }
   } else {
      mGrillValveFlow = 0.0;
   }

   /// - Sum the net flows into the volumes, first with the internal flows:
   mCabin.mInflowRate     = -mHatchFlow - mMpevFlow - mImvFlow;
   mVestibule.mInflowRate = mHatchFlow + mMpevFlow - mGrillValveFlow;
   mImvDuct.mInflowRate   = mGrillValveFlow + mImvFlow;

   /// - Add flows to/from the distributed interfaces.  In Demand role when constraining the
   ///   volume state to valid Supply data, the above internal flow into the volume is limited to
   ///   the interface's Demand flow rate limit, but only when we don't have a complete IMV
   ///   circulation path - this avoids the Demand rate limit from constraining the IMV fan
   ///   circulation flow too much.  In Supply role, subtract the Demand's flow rate from the
   ///   volume inflow.
   bool const imvCircPath = mGrillValveOpen or ( mImvValveOpen and mHatchOpen );
   if ( mVestibule.mIfDataValid and mVestibule.mIf.isInDemandRole() ) {
      if ( not imvCircPath ) {
         if ( std::abs( mVestibule.mInflowRate ) > mVestibule.mIfDemandLim ) {
            double const limRatio = mVestibule.mIfDemandLim / std::abs( mVestibule.mInflowRate );
            mHatchFlow *= limRatio;
            mMpevFlow *= limRatio;
            mGrillValveFlow *= limRatio;
            mVestibule.mInflowRate *= limRatio;
         }
      }
   } else {
      mVestibule.mInflowRate -= mVestibule.mIfFlow.mFlowRate;
   }
   if ( mImvDuct.mIfDataValid and mImvDuct.mIf.isInDemandRole() ) {
      if ( not imvCircPath ) {
         if ( std::abs( mImvDuct.mInflowRate ) > mImvDuct.mIfDemandLim ) {
            double const limRatio = mImvDuct.mIfDemandLim / std::abs( mImvDuct.mInflowRate );
            mImvFlow *= limRatio;
            mGrillValveFlow *= limRatio;
            mImvDuct.mInflowRate *= limRatio;
         }
      }
   } else {
      mImvDuct.mInflowRate -= mImvDuct.mIfFlow.mFlowRate;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Transports flow between the volumes and the Fluid Distributed Interfaces.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::transportFlows()
{
   /// - Zero inflows to the Demand role interfaces
   mCabin.mIfInflowH     = 0.0;
   mImvDuct.mIfInflowH   = 0.0;
   mVestibule.mIfInflowH = 0.0;
   mCabin.mIfInflowN     = 0.0;
   mImvDuct.mIfInflowN   = 0.0;
   mVestibule.mIfInflowN = 0.0;

   /// - Start with any interface flow into (negative rate) Supply role volumes.
   if ( not mVestibule.mIf.isInDemandRole() and mVestibule.mIfFlow.mFlowRate < 0.0 ) {
      mVestibule.addMixture( -mVestibule.mIfFlow.mFlowRate * mTimestep,
                             mVestibule.computeIfEnthalpy( mVestibule.mIfFlow.mEnergy, mVestibule.mIfMixIn.mSpecificHeat ),
                             mVestibule.mIfMixIn );
   }
   if ( not mImvDuct.mIf.isInDemandRole() and mImvDuct.mIfFlow.mFlowRate < 0.0 ) {
      mImvDuct.addMixture( -mImvDuct.mIfFlow.mFlowRate * mTimestep,
                           mImvDuct.computeIfEnthalpy( mImvDuct.mIfFlow.mEnergy, mImvDuct.mIfMixIn.mSpecificHeat ),
                           mImvDuct.mIfMixIn );
   }

   /// - Next do internal outflows from the cabin.  This requires cabin to be the largest volume.
   if ( mImvFlow > 0.0 ) {
      mImvDuct.addMixture( mImvFlow * mTimestep, mCabin.mEnthalpy, mCabin.mMixture );
      mCabin.removeMoles( mImvFlow * mTimestep );
   }
   if ( ( mHatchFlow + mMpevFlow ) > 0.0 ) {
      mVestibule.addMixture( ( mHatchFlow + mMpevFlow ) * mTimestep, mCabin.mEnthalpy, mCabin.mMixture );
      mCabin.removeMoles( ( mHatchFlow + mMpevFlow ) * mTimestep );
   }

   /// - Next do internal outflows from the vestibule.  This requires vestibule to be larger than IMV duct volume.
   if ( mGrillValveFlow > 0.0 ) {
      mImvDuct.addMixture( mGrillValveFlow * mTimestep, mVestibule.mEnthalpy, mVestibule.mMixture );
      mVestibule.removeMoles( mGrillValveFlow * mTimestep );
   }
   if ( ( mHatchFlow + mMpevFlow ) < 0.0 ) {
      mCabin.addMixture( -( mHatchFlow + mMpevFlow ) * mTimestep, mVestibule.mEnthalpy, mVestibule.mMixture );
      mVestibule.removeMoles( -( mHatchFlow + mMpevFlow ) * mTimestep );
   }

   /// - Next do internal outflows from the IMV duct.
   if ( mImvFlow < 0.0 ) {
      mCabin.addMixture( -mImvFlow * mTimestep, mImvDuct.mEnthalpy, mImvDuct.mMixture );
      mImvDuct.removeMoles( -mImvFlow * mTimestep );
   }
   if ( mGrillValveFlow < 0.0 ) {
      mVestibule.addMixture( -mGrillValveFlow * mTimestep, mImvDuct.mEnthalpy, mImvDuct.mMixture );
      mImvDuct.removeMoles( -mGrillValveFlow * mTimestep );
   }

   // - Finally do Supply role flows out (positive rate) to interface
   if ( not mVestibule.mIf.isInDemandRole() and mVestibule.mIfFlow.mFlowRate > 0.0 ) {
      mVestibule.addMixture( -mVestibule.mIfFlow.mFlowRate * mTimestep,
                             mVestibule.computeIfEnthalpy( mVestibule.mIfFlow.mEnergy, mVestibule.mIfMixIn.mSpecificHeat ),
                             mVestibule.mIfMixIn );
   }
   if ( not mImvDuct.mIf.isInDemandRole() and mImvDuct.mIfFlow.mFlowRate > 0.0 ) {
      mImvDuct.addMixture( -mImvDuct.mIfFlow.mFlowRate * mTimestep,
                           mImvDuct.computeIfEnthalpy( mImvDuct.mIfFlow.mEnergy, mImvDuct.mIfMixIn.mSpecificHeat ),
                           mImvDuct.mIfMixIn );
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the volume pressures to match the final mole quantity and temperature after
///           transport.  These can differ from the solution of the SOE when the volume temperature
///           has changed due to mixing of inflows, because the SOE solution is based on the last-
///           pass temperature.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::updatePressures()
{
   mCabin.updatePressure();
   mVestibule.updatePressure();
   mImvDuct.updatePressure();
   mImvFanDp = mImvDuct.mPressure - mCabin.mPressure;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Computes the effective capacitance of the Fluid Distributed Interface volumes.  This
///           uses the 'mock solution' approach recommended by the interface documentation, where we
///           compute what the volume pressures would be if we added an extra mole of air to it, by
///           re-solving the SOE with a modified source vector that adds the extra mole.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::computeCapacitance()
{
   /// - For each interface that is constrained in Demand role and applying the Supply capacitance
   ///   to our volume, record that supplied capacitance so its effect can be removed below.
   double vestSuppliedC = 0.0;
   if ( mVestibule.mIfDataValid and mVestibule.mIf.isInDemandRole() ) {
      vestSuppliedC = mVestibule.mIf.mInData.mCapacitance;
   }
   double imvSuppliedC = 0.0;
   if ( mImvDuct.mIfDataValid and mImvDuct.mIf.isInDemandRole() ) {
      imvSuppliedC = mImvDuct.mIf.mInData.mCapacitance;
   }

   {
      /// - For the Vestibule:
      /// - Add 1 mol/s to the vestibule's source vector.
      double const sources[3] = { mSourceVector[0], mSourceVector[1] + 1.0, mSourceVector[2] };
      /// - Solve for new pressures.
      double pressures[3] = { 0.0, 0.0, 0.0 };
      solvePressures( pressures, sources );
      /// - Compute 1/dP.
      double const vestCapDp  = pressures[1] - mSolutionVector[1];
      double const imvCapDp   = pressures[2] - mSolutionVector[2];
      mVestibule.mCapacitance = mTimestep / vestCapDp;
      /// - Subtract the Supplied capacitance when we are in Demand role.
      mVestibule.mCapacitance -= vestSuppliedC;
      /// - Subtract the effect that the Supplied capacitance of the other (IMV duct) interface
      ///   has at the vestibule location when it is in Demand role
      if ( imvSuppliedC > DBL_EPSILON ) { // IMV is in Demand role
         if ( imvCapDp > DBL_EPSILON ) {  // IMV affects vestibule thru conductive paths
            double const capDpRatio = imvCapDp / vestCapDp;
            mVestibule.mCapacitance -= ( imvSuppliedC * capDpRatio );
         }
      }
      /// - Capacitance cannot be less than the local volume's capacitance.
      mVestibule.mCapacitance = std::max( mVestibule.computeCapacitance(), mVestibule.mCapacitance );
   }
   {
      /// - For the IMV duct, repeat similar calculations as vestibule above.
      double const sources[3]   = { mSourceVector[0], mSourceVector[1], mSourceVector[2] + 1.0 };
      double       pressures[3] = { 0.0, 0.0, 0.0 };
      solvePressures( pressures, sources );
      double const vestCapDp = pressures[1] - mSolutionVector[1];
      double const imvCapDp  = pressures[2] - mSolutionVector[2];
      mImvDuct.mCapacitance  = mTimestep / imvCapDp;
      mImvDuct.mCapacitance -= imvSuppliedC;
      if ( vestSuppliedC > DBL_EPSILON ) { // Vestibule is in Demand role
         if ( vestCapDp > DBL_EPSILON ) {  // Vestibule affects IMV thru conductive paths
            double const capDpRatio = vestCapDp / imvCapDp;
            mImvDuct.mCapacitance -= ( vestSuppliedC * capDpRatio );
         }
      }
      mImvDuct.mCapacitance = std::max( mImvDuct.computeCapacitance(), mImvDuct.mCapacitance );
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the total moles and energy within the volumes that this model owns.  This can
///           be used to check for conservation of mass & energy.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmo::updateConservation()
{
   mConserveParams.moles    = mCabin.mMoles;
   mConserveParams.molesN2  = mCabin.mMoles * mCabin.mMixture.mMoleFractions[0];
   mConserveParams.molesO2  = mCabin.mMoles * mCabin.mMixture.mMoleFractions[1];
   mConserveParams.molesH2O = mCabin.mMoles * mCabin.mMixture.mMoleFractions[2];
   mConserveParams.molesCO2 = mCabin.mMoles * mCabin.mMixture.mMoleFractions[3];
   mConserveParams.energy   = mCabin.mMoles * mCabin.mEnthalpy;
   // We don't include the moles of volumes that are in Demand role, because their moles are
   // being included on the Supply side.
   if ( not( mVestibule.mIfDataValid and mVestibule.mIf.isInDemandRole() ) ) {
      mConserveParams.moles += mVestibule.mMoles;
      mConserveParams.molesN2 += mVestibule.mMoles * mVestibule.mMixture.mMoleFractions[0];
      mConserveParams.molesO2 += mVestibule.mMoles * mVestibule.mMixture.mMoleFractions[1];
      mConserveParams.molesH2O += mVestibule.mMoles * mVestibule.mMixture.mMoleFractions[2];
      mConserveParams.molesCO2 += mVestibule.mMoles * mVestibule.mMixture.mMoleFractions[3];
      mConserveParams.energy += mVestibule.mMoles * mVestibule.mEnthalpy;
   }
   if ( not( mImvDuct.mIfDataValid and mImvDuct.mIf.isInDemandRole() ) ) {
      mConserveParams.moles += mImvDuct.mMoles;
      mConserveParams.molesN2 += mImvDuct.mMoles * mImvDuct.mMixture.mMoleFractions[0];
      mConserveParams.molesO2 += mImvDuct.mMoles * mImvDuct.mMixture.mMoleFractions[1];
      mConserveParams.molesH2O += mImvDuct.mMoles * mImvDuct.mMixture.mMoleFractions[2];
      mConserveParams.molesCO2 += mImvDuct.mMoles * mImvDuct.mMixture.mMoleFractions[3];
      mConserveParams.energy += mImvDuct.mMoles * mImvDuct.mEnthalpy;
   }
}
