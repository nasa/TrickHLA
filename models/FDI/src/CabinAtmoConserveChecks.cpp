/**
@file     CabinAtmoConserveChecks.cpp
@brief    Simple Cabin Atmosphere Conservation Checks implementation

@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

LIBRARY DEPENDENCY:
   ()
*/

#include <cfloat>

#include "../include/CabinAtmo.hh"
#include "../include/CabinAtmoConserveChecks.hh"

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Constructs this Simple Cabin Atmosphere Conservation Check Parameters object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConserveParameters::CabinAtmoConserveParameters()
   : energy( 0.0 ),
     moles( 0.0 ),
     molesN2( 0.0 ),
     molesO2( 0.0 ),
     molesH2O( 0.0 ),
     molesCO2( 0.0 )
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Destructs this Simple Cabin Atmosphere Conservation Check Parameters object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConserveParameters::~CabinAtmoConserveParameters()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] that (--) Reference to the object to be read from.
///
/// @returns  CabinAtmoConserveParameters& (--) Reference to this object.
///
/// @details  Assigns the values of this Simple Cabin Atmosphere Conservation Check Parameters
///           object equal to the values of the given object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConserveParameters &CabinAtmoConserveParameters::operator=( const CabinAtmoConserveParameters &that )
{
   if ( this != &that ) {
      energy   = that.energy;
      moles    = that.moles;
      molesN2  = that.molesN2;
      molesO2  = that.molesO2;
      molesH2O = that.molesH2O;
      molesCO2 = that.molesCO2;
   }
   return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] a    (--) Reference to the first CabinAtmo model's conservation parameters.
/// @param[in] b    (--) Reference to the second CabinAtmo model's conservation parameters.
///
/// @details  Constructs this Simple Cabin Atmosphere Conservation Checks object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConserveChecks::CabinAtmoConserveChecks( const CabinAtmoConserveParameters &a, const CabinAtmoConserveParameters &b )
   : modelA( a ),
     modelB( b ),
     modelAConserveParams(),
     modelBConserveParams(),
     isBsideHla( false ),
     setReference( false ),
     current(),
     reference(),
     error()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Destructs this Simple Cabin Atmosphere Conservation Checks object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoConserveChecks::~CabinAtmoConserveChecks()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Sums the current parameter totals, sets the new reference values on command, and
///           computes error between the current values and their reference values.
///
/// @note  Because of lag in the data interface between models A & B, the computed errors are only
///        are only accurate when flows between A and B are zero or A and B are completely mixed.
///        These errors will also momentarily spike and be incorrect during Supply/Demand role swap.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoConserveChecks::update()
{
   /// - Update inputs from the local models when we're not in HLA mode.  When in HLA mode, HLA
   ///   will provide the B side data, and A side data will be input at the end of this function.
   if ( not isBsideHla ) {
      modelAConserveParams = modelA;
      modelBConserveParams = modelB;
   }

   /// - Update the combined A and B side conservation parameter totals.
   current.energy   = modelAConserveParams.energy + modelBConserveParams.energy;
   current.moles    = modelAConserveParams.moles + modelBConserveParams.moles;
   current.molesN2  = modelAConserveParams.molesN2 + modelBConserveParams.molesN2;
   current.molesO2  = modelAConserveParams.molesO2 + modelBConserveParams.molesO2;
   current.molesH2O = modelAConserveParams.molesH2O + modelBConserveParams.molesH2O;
   current.molesCO2 = modelAConserveParams.molesCO2 + modelBConserveParams.molesCO2;

   /// - Reset the reference totals on command.
   if ( setReference ) {
      setReference = false;
      reference    = current;
   }

   /// - Wait for the reference to be set before computing errors.
   if ( reference.moles > 0.0 ) {
      error.energy   = 100.0 * ( current.energy - reference.energy ) / std::max( DBL_EPSILON, reference.energy );
      error.moles    = 100.0 * ( current.moles - reference.moles ) / std::max( DBL_EPSILON, reference.moles );
      error.molesN2  = 100.0 * ( current.molesN2 - reference.molesN2 ) / std::max( DBL_EPSILON, reference.molesN2 );
      error.molesO2  = 100.0 * ( current.molesO2 - reference.molesO2 ) / std::max( DBL_EPSILON, reference.molesO2 );
      error.molesH2O = 100.0 * ( current.molesH2O - reference.molesH2O ) / std::max( DBL_EPSILON, reference.molesH2O );
      error.molesCO2 = 100.0 * ( current.molesCO2 - reference.molesCO2 ) / std::max( DBL_EPSILON, reference.molesCO2 );
   }

   /// - When in HLA mode, update A side data inputs at the end, after our conservation
   ///   computations.  This lags the A side data by 1 frame to match the nominal transport lag of
   ///   the B side data.
   if ( isBsideHla ) {
      modelAConserveParams = modelA;
   }
}
