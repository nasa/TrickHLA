/**
@ingroup DistIf
@file    CabinAtmoVolume.cpp
@brief   Simple Cabin Atmosphere Volume Model implementation

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
@trick_link_dependency{CabinAtmoMixture.o}
@trick_link_dependency{Distributed2WayBusFluid.o}
@trick_link_dependency{CabinAtmoVolume.o}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

*/

#include <cfloat>
#include <iostream>

#include "../include/CabinAtmoVolume.hh"

// Trick include for TMM
#include "sim_services/MemoryManager/include/memorymanager_c_intf.h"

using namespace DistIf;

/// @details Universal gas constant, J/mol/K.
double const CabinAtmoVolume::R_UNIV = 8.314472;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] volume        (m3)      Air volume.
/// @param[in] temperature   (K)       Air temperature.
/// @param[in] pressure      (Pa)      Air pressure.
/// @param[in] moleFractions (--)      Compound mole fractions of the air mixture.
/// @param[in] compoundCp    (J/mol/K) Specific heats of the chemical compounds in the air mixture.
/// @param[in] isIfMaster    (--)      This is the master side of the Distributed Fluid Interface pairing.
/// @param[in] isIfEnthalpy  (--)      The Fluid Distributed Interface transports energy as specific enthalpy instead of temperature.
///
/// @details  Default constructs this Simple Cabin Atmosphere Volume Model Configuration Data.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoVolumeConfigData::CabinAtmoVolumeConfigData(
   double const  volume,
   double const  temperature,
   double const  pressure,
   double const *moleFractions,
   double const *compoundCp,
   bool const    isIfMaster,
   bool const    isIfEnthalpy )
   : mVolume( volume ),
     mTemperature( temperature ),
     mPressure( pressure ),
     mMoleFractions( moleFractions ),
     mCompoundCp( compoundCp ),
     mIsIfMaster( isIfMaster ),
     mIsIfEnthalpy( isIfEnthalpy )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this Simple Cabin Atmosphere Volume Model Configuration Data.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoVolumeConfigData::~CabinAtmoVolumeConfigData()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] name (--) Name of this object for output messages.
///
/// @details  Default constructs this Simple Cabin Atmosphere Volume Model.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoVolume::CabinAtmoVolume(
   std::string const &name )
   : mConfig( 0 ),
     mTemperature( 0.0 ),
     mPressure( 0.0 ),
     mMoles( 0.0 ),
     mMixture(),
     mEnthalpy( 0.0 ),
     mCapacitance( 0.0 ),
     mIf(),
     mIfFluid(),
     mIfFlow(),
     mIfDataValid( false ),
     mIfDemandLim( 1.0e15 ),
     mInflowRate( 0.0 ),
     mIfInflowN( 0.0 ),
     mIfInflowH( 0.0 ),
     mIfMixIn(),
     mIfMixOut(),
     mName( name )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Default destructs this Simple Cabin Atmosphere Volume Model.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoVolume::~CabinAtmoVolume()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] config (--) The configuration data for this object.
///
/// @details  Initializes this Simple Cabin Atmosphere Volume Model with its configuration data.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::initialize(
   CabinAtmoVolumeConfigData const &config )
{
   mConfig = &config;
   validateConfig();
   mTemperature = config.mTemperature;
   mPressure    = config.mPressure;

   mMixture.initialize( config.mMoleFractions, config.mCompoundCp );
   mIfMixIn.initialize( config.mMoleFractions, config.mCompoundCp );
   mIfMixOut.initialize( config.mMoleFractions, config.mCompoundCp );

   updateMoles();
   updateEnthalpy();
   mCapacitance = computeCapacitance();

   mIfFluid.initialize( CabinAtmoMixture::NFOMBULK, CabinAtmoMixture::NFOMTC, mName + ".mIfFluid" );
   mIfFlow.initialize( CabinAtmoMixture::NFOMBULK, CabinAtmoMixture::NFOMTC, mName + ".mIfFluid" );
   mIf.initialize( config.mIsIfMaster, CabinAtmoMixture::NFOMBULK, CabinAtmoMixture::NFOMTC );

   /// - Declare the dynamic mole fraction arrays in the Fluid Distributed Interface objects to
   ///   Trick Memory Manager, so that they can be targeted by TrickHLA.  TrickHLA will ignore
   ///   dynamic arrays as target variables for a FOM if TMM doesn't know about them.
   std::string allocSpec = "double " + mName + ".mIf.mInData.mMoleFractions";
   convertNameForTmm( allocSpec );
   TMM_declare_ext_var_1d( mIf.mInData.mMoleFractions, allocSpec.c_str(), CabinAtmoMixture::NFOMBULK );

   allocSpec = "double " + mName + ".mIf.mInData.mTcMoleFractions";
   convertNameForTmm( allocSpec );
   TMM_declare_ext_var_1d( mIf.mInData.mTcMoleFractions, allocSpec.c_str(), CabinAtmoMixture::NFOMTC );

   allocSpec = "double " + mName + ".mIf.mOutData.mMoleFractions";
   convertNameForTmm( allocSpec );
   TMM_declare_ext_var_1d( mIf.mOutData.mMoleFractions, allocSpec.c_str(), CabinAtmoMixture::NFOMBULK );

   allocSpec = "double " + mName + ".mIf.mOutData.mTcMoleFractions";
   convertNameForTmm( allocSpec );
   TMM_declare_ext_var_1d( mIf.mOutData.mTcMoleFractions, allocSpec.c_str(), CabinAtmoMixture::NFOMTC );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in, out] name (--) The given name string to convert.
///
/// @details  Replaces any '.' with '__', so the name string can be given to Trick Memory Manager as
///           the allocation name.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::convertNameForTmm(
   std::string &name ) const
{
   std::string tokFrom;
   std::string tokTo;
   tokFrom = ".";
   tokTo   = "__";
   if ( !name.empty() ) {
      std::string::size_type toLen = tokTo.length();
      std::string::size_type frLen = tokFrom.length();
      std::string::size_type loc   = 0;
      while ( std::string::npos != ( loc = name.find( tokFrom, loc ) ) ) {
         name.replace( loc, frLen, tokTo );
         loc += toLen;
         if ( loc >= name.length() ) {
            break;
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Does some valid range checks on the configuration data, outputs error messages.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::validateConfig() const
{
   if ( mConfig->mVolume < DBL_EPSILON ) {
      std::cout << "ERROR: " << mName << " volume < DBL_EPSILON!\n";
   }
   if ( mConfig->mTemperature < DBL_EPSILON ) {
      std::cout << "ERROR: " << mName << " initial temperature < DBL_EPSILON!\n";
   }
   if ( mConfig->mPressure < 0.0 ) {
      std::cout << "ERROR: " << mName << " initial pressure < zero!\n";
   }
   double sum = 0.0;
   for ( unsigned int i = 0; i < CabinAtmoMixture::NBULK; ++i ) {
      sum += mConfig->mMoleFractions[i];
   }
   if ( std::abs( 1.0 - sum ) > DBL_EPSILON ) {
      std::cout << "ERROR: " << mName << " initial mixture doesn't sum to 1!\n";
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Computes and stores the air moles from the air volume, pressure and temperature by the
///           Ideal Gas Law.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updateMoles()
{
   mMoles = mPressure * mConfig->mVolume / R_UNIV / mTemperature;
   limitMoles();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  To avoid divide-by-zero and arithmetic underflows, prevent moles from reaching zero.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::limitMoles()
{
   if ( mMoles < DBL_EPSILON ) {
      mMoles = DBL_EPSILON;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moles (--) Moles of fluid to compute the pressure for.
///
/// @returns  double (Pa) Pressure.
///
/// @details  Computes and returns the air pressure from the given moles and the current volume and
///           temperature, by the Ideal Gas Law.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computePressure(
   double const moles ) const
{
   return ( moles * R_UNIV * mTemperature / mConfig->mVolume );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the volume pressure to match its current state.  This is skipped if the
///           Distributed Interface is in Demand role with valid received Supply data, since this
///           volume will constrain its pressure to match the Supply value.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updatePressure()
{
   if ( not( mIfDataValid and mIf.isInDemandRole() ) ) {
      mPressure = computePressure( mMoles );
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] enthalpy (J/mol) Specific enthalpy of the air to calculate the temperature for.
///
/// @returns  double (K) Temperature.
///
/// @details  Computes and returns the air temperature for the given specific enthalpy and the
///           current mixture specific heat, by T = h/Cp assuming calorically perfect gas.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computeTemperature(
   double const enthalpy ) const
{
   return ( enthalpy / mMixture.mSpecificHeat );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates this volume's air temperature for the current specific enthalpy and mixture.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updateTemperature()
{
   mTemperature = computeTemperature( mEnthalpy );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] temperature (K) Temperature of the air to calculate the specific enthalpy for.
///
/// @returns  double (J/mol) Specific enthalpy.
///
/// @details  Computes and returns the air specific enthalpy for the given temperature and the
///           current mixture specific heat, by h = Cp*T assuming calorically perfect gas.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computeEnthalpy(
   double const temperature ) const
{
   return ( temperature * mMixture.mSpecificHeat );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates this volume's air specific enthalpy for the current temperature and mixture.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updateEnthalpy()
{
   mEnthalpy = computeEnthalpy( mTemperature );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @returns  doubld (mol/Pa) Capacitance.
///
/// @details  Computes and returns this volume's local capacitance for its current volume and
///           temperature, by the Ideal Gas Law.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computeCapacitance() const
{
   return ( mConfig->mVolume / R_UNIV / std::max( DBL_EPSILON, mTemperature ) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moles          (mol)   The quantity of moles to be added or removed.
/// @param[in] enthalpy       (J/mol) Specific enthalpy of the moles being added or removed.
/// @param[in] moleFractions  (--)    The mole fractions of the mixture being added or removed.
///
/// @details  Adds the given amount of moles with given specific enthalpy and mixture to the inflow
///           collection state when this volume is constrained to the Supply state (Demand role), or
///           to this volume's contents when not constrained (Supply role).  A negative value for
///           moles causes the given mixture to be removed instead.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::addMixture(
   double const  moles,
   double const  enthalpy,
   double const *moleFractions )
{
   double addedEnergy = moles * enthalpy;

   if ( mIfDataValid and mIf.isInDemandRole() ) {
      double previousEnergy = mIfInflowN * mIfInflowH;
      mIfMixOut.mix( mIfInflowN, moles, moleFractions );
      mIfInflowN += moles;
      mIfInflowH = ( previousEnergy + addedEnergy ) / std::max( mIfInflowN, DBL_EPSILON );
   } else {
      double previousEnergy = mMoles * mEnthalpy;
      mMixture.mix( mMoles, moles, moleFractions );
      mMoles += moles;
      limitMoles();
      mEnthalpy = ( previousEnergy + addedEnergy ) / std::max( mMoles, DBL_EPSILON );
      updateTemperature();
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moles    (mol)   The quantity of moles to be added or removed.
/// @param[in] enthalpy (J/mol) Specific enthalpy of the moles being added or removed.
/// @param[in] mixture  (--)    The mixture data object for the moles being added or removed.
///
/// @details  This overloaded function calls addMixture with the mole fractions array from the given
///           mixture data object.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::addMixture(
   double const            moles,
   double const            enthalpy,
   CabinAtmoMixture const &mixture )
{
   addMixture( moles, enthalpy, mixture.mMoleFractions );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moles (mol) The quantity of moles to be removed.
///
/// @details  Removes the given quantity of moles from this volume's contents.  We don't remove the
///           moles when in Demand role because the contents of this volume are constrained to the
///           values from the Supply side.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::removeMoles(
   double const moles )
{
   if ( not mIf.isInDemandRole() ) {
      mMoles -= moles;
      limitMoles();
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] energy       (--)      Energy term in the Fluid Distributed Interface data (K) or (J/mol).
/// @param[in] specificHeat (J/mol/K) Specific heat of the mixture for converting to temperature.
///
/// @returns  double (J/mol) Specific enthalpy represented by the energy term.
///
/// @details  Returns the specific enthalpy represented by the given energy value from this volume's
///           Fluid Distributed Interface.  If the interface is configured to represent energy as
///           specific enthalpy, then the given energy value is simply returned.  Otherwise, the
///           energy value represents temperature, and is converted to specific enthalpy by the
///           given specific heat.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computeIfEnthalpy(
   double const energy,
   double const specificHeat ) const
{
   double result = energy;
   if ( not mConfig->mIsIfEnthalpy ) {
      result *= std::max( DBL_EPSILON, specificHeat );
   }
   return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] temperature  (K)       Temperature for the Fluid Distributed Interface energy term.
/// @param[in] specificHeat (J/mol/K) Specific heat of the mixture for converting to specific enthalpy.
///
/// @returns  double (--) Specific enthalpy (J/mol) or temperature (K) represented by the energy term.
///
/// @details  Returns the energy term for this volume's Fluid Distributed Interface represented by
///           the given temperature value.  If the interface is configured to represent energy as
///           temperature, then the given energy value is simply returned.  Otherwise, the energy
///           value represents specific enthalpy, and is converted from temperature by the given
///           specific heat.
////////////////////////////////////////////////////////////////////////////////////////////////////
double CabinAtmoVolume::computeIfEnergy(
   double const temperature,
   double const specificHeat ) const
{
   double result = temperature;
   if ( mConfig->mIsIfEnthalpy ) {
      result *= specificHeat;
   }
   return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] dt          (s)  Time step of integration.
/// @param[in] demandSideP (Pa) Source or sink pressure on our side of the interface driving flow.
///
/// @details  Updates the Fluid Distributed Interface before the main CabinAtmo model update.
///           This does steps 2-6 of the local model interfaces to the Fluid Distributed Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updateIfPre(
   double const dt,
   double const demandSideP )
{
   /// - Fluid Distributed Interface, local model operations Step 2: model calls the interface to
   ///   process its inputs, the incoming data from the other side of the HLA interface.
   mIf.processInputs();

   /// - Fluid Distributed Interface, local model operations Step 3: model fins which role the
   ///   interface is in (Supply or Demand) and responds to role swap as needed.  The interface can
   ///   swap from Demand to Supply role in the processInputs call.  Flipping to Supply role needs
   ///   no response here since our volume still represents the last-pass Supply state from the
   ///   other side.
   bool const isDemandRole = mIf.isInDemandRole();

   /// - Fluid Distributed Interface, local model operations Step 4: model calls getFluidState() or
   ///   getFlowState() based on role.  In Demand role, we get the state of the fluid contents of
   ///   the volume from the Supply values.  In Supply role, we get the state of the fluid flow
   ///   across the interface from the Demand values.  In both roles, we store whether the received
   ///   data is valid.  During role swaps there is a brief period where the incoming data from the
   ///   other side is still in the same role we are already in, so we can't use it.
   if ( isDemandRole ) {
      mIfDataValid = mIf.getFluidState( mIfFluid );
   } else {
      mIfDataValid = mIf.getFlowState( mIfFlow );
   }

   /// - The interface doesn't update mIfFlow if data isn't valid, so we zero out any stale value
   ///   left in the interface flow rate from before.
   if ( not mIfDataValid ) {
      mIfFlow.mFlowRate = 0.0;
   }

   /// - Fluid Distributed Interface, local model operations Step 5: model applies the returned
   ///   fluid or flow state boundary conditions to its interface volume.
   if ( isDemandRole ) {
      /// - In Demand role with valid Supply data from the interface, we set our volume state
      ///   equal to the Supply state.
      if ( mIfDataValid ) {
         mMixture.readMoleFractions( mIfFluid.mMoleFractions );
         mPressure = mIfFluid.mPressure;
         mEnthalpy = computeIfEnthalpy( mIfFluid.mEnergy, mMixture.mSpecificHeat );
         updateTemperature();
         updateMoles();
      }

      /// - Fluid Distributed Interface, local model operations Step 6: model gets the demand flow
      ///   rate limit from the interface.
      mIfDemandLim = mIf.computeDemandLimit( dt, demandSideP );

      /// - Reset the demand flow rate, so flows can be added by the model later in the update.
      mIfFlow.mFlowRate = 0.0;

   } else {
      /// - In Supply role, load a mixture data object with the Demand flow's mixture, for adding
      ///   to or removing from this volume later in the model update.
      mIfMixIn.mix( 0.0, std::abs( mIfFlow.mFlowRate ), mIfFlow.mMoleFractions );
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Updates the Fluid Distributed Interface after the main CabinAtmo model update.
///           This does steps 9-12 of the local model interfaces to the Fluid Distributed Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoVolume::updateIfPost()
{
   /// - Fluid Distributed Interface, local model operations Step 9: model calls setFluidState() or
   ///   setFlowState() based on role.
   if ( mIf.isInDemandRole() ) {
      /// - In Demand role, copy the mixture and energy of the flows to the volume from our local
      ///   model to the interface demand flow object, mIfFlow, and then give that to the
      ///   interface by calling setFlowState().  We flip the sign of the flow rate because
      ///   positive interface flow is to the Demand side from the Supply side.
      mIfFlow.mFlowRate = -mInflowRate;
      if ( mIfFlow.mFlowRate >= 0.0 ) {
         mIfFlow.mEnergy = computeIfEnergy( mTemperature, mMixture.mSpecificHeat );
         mMixture.writeMoleFractions( mIfFlow.mMoleFractions );
      } else {
         mIfFlow.mEnergy = computeIfEnergy( mIfInflowH / mIfMixOut.mSpecificHeat, mIfMixOut.mSpecificHeat );
         mIfMixOut.writeMoleFractions( mIfFlow.mMoleFractions );
      }
      mIf.setFlowState( mIfFlow );
   } else {
      /// - In Supply role, load our fluid state of the volume to the interface supply fluid
      ///   object, mIfFluid, and then give that to the interface by calling setFluidState().
      mIfFluid.mPressure = mPressure;
      mIfFluid.mEnergy   = computeIfEnergy( mTemperature, mMixture.mSpecificHeat );
      mMixture.writeMoleFractions( mIfFluid.mMoleFractions );
      mIf.setFluidState( mIfFluid );
   }

   /// - Fluid Distributed Interface, local model operations Step 10: model calls processOutputs(),
   ///   passing our side's capacitance, determined by the local model, to the interface.
   mIf.processOutputs( mCapacitance );

   /// - Fluid Distributed Interface, local model operations Step 11: model calls isInDemandRole(),
   ///   responds to role swap as needed.  At this point the interface may have flipped from Supply
   ///   to Demand role because our capacitance has gone too low relative to the other side's.
   ///   We don't need to do anything, since the interface has already zeroed the outgoing source
   ///   term, so there will be zero Demand flow sent to the other side on this pass, and our next
   ///   pass will run the Demand role as normal.
   mIf.isInDemandRole();

   /// - Fluid Distributed Interface, local model operations Step 12: model calls popNotification()
   ///   recursively until no notifications left, transfers notification to the sim's messaging
   ///   system as desired.  For this simple model, just send it to standard out.
   Distributed2WayBusNotification notification;
   unsigned int                   numNotifs = 0;
   do {
      numNotifs = mIf.popNotification( notification );
      if ( Distributed2WayBusNotification::NONE != notification.mLevel ) {
         std::cout << mName << " from mIf: " << notification.mMessage << '\n';
      }
   } while ( numNotifs > 0 );
}
