/**
@ingroup DistIf
@file    CabinAtmoMixture.cpp
@brief   Simple Cabin Atmosphere Mixture Data implementation

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

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

*/

#include <cfloat>
#include <iostream>

#include "../include/CabinAtmoMixture.hh"

using namespace DistIf;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Constructs this Simple Cabin Atmosphere Air Mixture Data object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoMixture::CabinAtmoMixture()
   : mCompoundSpecificHeats( 0 ),
     mMoleFractions(),
     mTcMoleFractions(),
     mSpecificHeat( 0.0 )
{
   for ( unsigned int i = 0; i < NFOMBULK; ++i ) {
      mMoleFractions[i] = 0.0;
   }
   for ( unsigned int i = 0; i < NFOMTC; ++i ) {
      mTcMoleFractions[i] = 0.0;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Destructs this Simple Cabin Atmosphere Air Mixture Data object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoMixture::~CabinAtmoMixture()
{
   return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] that (--) The object to be assigned equal to.
///
/// @returns  CabinAtmoMixture& (--) Reference to this object.
///
/// @details  Assigns the values of this object equal to the values of the given object.
////////////////////////////////////////////////////////////////////////////////////////////////////
CabinAtmoMixture &CabinAtmoMixture::operator=( // cppcheck-suppress [operatorEqVarError]
   CabinAtmoMixture const &that )
{
   if ( this != &that ) {
      for ( unsigned int i = 0; i < NFOMBULK; ++i ) {
         mMoleFractions[i] = that.mMoleFractions[i];
      }
      for ( unsigned int i = 0; i < NFOMTC; ++i ) {
         mTcMoleFractions[i] = that.mTcMoleFractions[i];
      }
      mSpecificHeat = that.mSpecificHeat;
   }
   return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moleFractions (--)      The initial mole fractions array.
/// @param[in] compoundCp    (J/mol/K) Specific heat constants for the compounds in air.
///
/// @details  Initializes this object with the given initial mole fractions.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::initialize(
   double const *moleFractions,
   double const *compoundCp )
{
   mCompoundSpecificHeats = compoundCp;
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      if ( mCompoundSpecificHeats[i] < DBL_EPSILON ) {
         std::cout << "ERROR: a compound specific heat constant < DBL_EPSILON!" << '\n';
      }
      mMoleFractions[i] = moleFractions[i];
   }
   if ( checkMoleFractionsSum() ) {
      std::cout << "WARNING: normalized a CabinAtmoMixture initial mole fractions that did not sum to 1." << '\n';
      normalize();
   }
   updateSpecificHeat();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @returns bool (--) True if the mole fractions do not sum to 1.
///
/// @details  Sums the mMoleFractions and checks the sum is equal to within numerical precision of
///           1.  Returns true if it does not sum to 1.
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CabinAtmoMixture::checkMoleFractionsSum() const
{
   double sum = 0.0;
   for ( unsigned int i = 0; i < NFOMBULK; ++i ) {
      sum += mMoleFractions[i];
   }
   return ( std::abs( 1.0 - sum ) > DBL_EPSILON );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Computes and sets the specific heat of this object as the average of the compound
///           specific heats, weighted by their mole fraction.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::updateSpecificHeat()
{
   double result = 0.0;
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      result += mCompoundSpecificHeats[i] * mMoleFractions[i];
   }
   mSpecificHeat = result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[out] moleFractions (--) The mole fractions array to be written to.
///
/// @details  Write this object's mixture into the given mole fractions array.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::writeMoleFractions(
   double *moleFractions ) const
{
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      moleFractions[i] = mMoleFractions[i];
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] moleFractions (--) The mole fractions array to be read from.
///
/// @details  Reads the given mole fractions array into this object's mixture.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::readMoleFractions(
   double const *moleFractions )
{
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      mMoleFractions[i] = moleFractions[i];
   }
   for ( unsigned int i = NBULK; i < NFOMBULK; ++i ) {
      mMoleFractions[i] = 0.0;
   }
   for ( unsigned int i = 0; i < NFOMTC; ++i ) {
      mTcMoleFractions[i] = 0.0;
   }
   if ( checkMoleFractionsSum() ) {
      double sum = 0.0;
      for ( unsigned int i = 0; i < NFOMBULK; ++i ) {
         sum += mMoleFractions[i];
      }
      if ( std::abs( sum - 1.0 ) > 5.0e-16 ) {
         std::cout << "WARNING: normalized a CabinAtmoMixture mole fractions that did not sum to 1." << '\n';
      }
      normalize();
   }
   updateSpecificHeat();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] oldMoles     (mol) Existing mole quantity to be added to.
/// @param[in] addMoles     (mol) Quantity of moles that are to be added.
/// @param[in] addFractions (--)  Mixture of the moles that are to be added.
///
/// @details  Computes the resulting mixture (mole fractions) of adding the given amount of moles
///           with given mole fractions to the given amount of moles with this object's existing
///           mole fractions.  Stores the result as this object's new mole fractions, and updates
///           the specific heat for the new mixture.
///
/// @note     A negative value of addMoles can be used to removed the specified mixture from the
///           previous mixture.  Any negative mole fractions in the resulting mixture are zeroed.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::mix(
   double const  oldMoles,
   double const  addMoles,
   double const *addFractions )
{
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      mMoleFractions[i] = oldMoles * mMoleFractions[i] + addMoles * addFractions[i];
   }
   normalize();
   updateSpecificHeat();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Normalizes this object's mole fractions array so the sum equals exactly 1.  If the
///           mole fractions currently sum to zero, then the mixture is reset to all the first
///           compound.  Negative fractions are zeroed.
////////////////////////////////////////////////////////////////////////////////////////////////////
void CabinAtmoMixture::normalize()
{
   double total = 0.0;
   for ( unsigned int i = 0; i < NBULK; ++i ) {
      mMoleFractions[i] = std::max( 0.0, mMoleFractions[i] );
      total += mMoleFractions[i];
   }
   if ( total > 0.0 ) {
      for ( unsigned int i = 0; i < NBULK; ++i ) {
         mMoleFractions[i] /= total;
      }
   } else {
      mMoleFractions[0] = 1.0;
      for ( unsigned int i = 1; i < NBULK; ++i ) {
         mMoleFractions[i] = 0.0;
      }
   }
}
