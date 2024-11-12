#ifndef CabinAtmoMixture_EXISTS
#define CabinAtmoMixture_EXISTS

/**
@ingroup DistIf
@file    CabinAtmoMixture.hh
@brief   Simple Cabin Atmosphere Mixture Data declarations

@details This models a mixture of compounds in a volume of the Simple Cabin
Atmosphere Model, and the mixture's associated specific heat.  This provides a
function to mix in another mixture and compute the resulting mixture.

\par<b>Assumptions and Limitations</b>
- Simplification: all chemical compounds in the air mixture (N2, O2, etc.) are
assumed calorically perfect with constant specific heat.
- Simplification: the specific heat of the air mixture is only a function of
the mixture.
- Trace compounds are not fully supported yet.  There are some place-holder
variables, but the interfaces and logic to model trace compounds are not
complete.

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
@trick_link_dependency{../src/CabinAtmoMixture.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

@{
*/

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Simple Cabin Atmosphere Mixture Data
///
/// @details  This models a mixture of compounds in a volume of the Simple Cabin Atmosphere Model,
///           and the mixture's associated specific heat.  This provides a function to mix in
///           another mixture and compute the resulting mixture.  This mixture has zero quantity,
///           and represents the state of an infinitesimal amount of air.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoMixture
{
  public:
   static unsigned int const NBULK    = 4;             /**< ** (1)       Number of modeled bulk compounds in the fluid mixture. */
   static unsigned int const NTC      = 0;             /**< ** (1)       Number of modeled trace compounds in the fluid mixture. */
   static unsigned int const NFOMBULK = 6;             /**< ** (1)       Number of mole fractions in the HLA FOM bulk fluid constituents array. */
   static unsigned int const NFOMTC   = 4;             /**< ** (1)       Number of mole fractions in the HLA FOM trace compound constituents array. */
   double const             *mCompoundSpecificHeats;   /**<    (J/mol/K) Specific heats of the chemical compounds in the air mixture. */
   double                    mMoleFractions[NFOMBULK]; /**<    (1)       Mole fractions of bulk fluid compounds in the air mixture. */
   double                    mTcMoleFractions[NFOMTC]; /**<    (1)       Mole fractions of trace compounds in the air mixture. */
   double                    mSpecificHeat;            /**<    (J/mol/K) Specific heat of this mixture. */
   /// @brief  Default constructs this Simple Cabin Atmosphere Air Mixture Data.
   CabinAtmoMixture();
   /// @brief  Default destructs this Simple Cabin Atmosphere Air Mixture Data.
   virtual ~CabinAtmoMixture();
   /// @brief  Assigns this Simple Cabin Atmosphere Air Mixture Data.
   CabinAtmoMixture &operator=( CabinAtmoMixture const &that );
   /// @brief  Initializes this Simple Cabin Atmosphere Air Mixture Data with initial mole fractions.
   void initialize( double const *moleFractions, double const *specificHeats );
   /// @brief  Updates the specific heat for the current mixture.
   void updateSpecificHeat();
   /// @brief  Copies these mole fractions to the given mole fraction array.
   void writeMoleFractions( double *moleFractions ) const;
   /// @brief  Copies mole fractions from the given mole fraction array.
   void readMoleFractions( double const *moleFractions );
   /// @brief  Mixes the given quantity and mole fraction with a relative quantity of the current mixture.
   void mix( double const oldMoles, double const addMoles, double const *addFractions );

  private:
   /// @brief Checks if the mole fractions array sums to 1.
   bool checkMoleFractionsSum() const;
   /// @brief Normalizes the mole fractions array to sum to 1.
   void normalize();
   /// @brief  Copy constructor unavailable since declared private and not implemented.
   CabinAtmoMixture( CabinAtmoMixture const & );
};

} // namespace DistIf

/// @}

#endif
