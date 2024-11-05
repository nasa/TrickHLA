#ifndef CabinAtmoMixture_EXISTS
#define CabinAtmoMixture_EXISTS

/**
@file     CabinAtmoMixture.hh
@brief    Simple Cabin Atmosphere Mixture Data declarations

@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

@details
PURPOSE:
- (This models a mixture of compounds in a volume of the Simple Cabin Atmosphere Model, and the
   mixture's associated specific heat.  This provides a function to mix in another mixture and
   compute the resulting mixture.)

REFERENCE:
- (TBD)

ASSUMPTIONS AND LIMITATIONS:
- ((Simplification: all chemical compounds in the air mixture (N2, O2, etc.) are assumed calorically
    perfect with constant specific heat.)
   (Simplification: the specific heat of the air mixture is only a function of the mixture.)
   (Trace compounds are not fully supported yet.  There are some place-holder variables, but the
    interfaces and logic to model trace compounds are not complete.))

LIBRARY DEPENDENCY:
- (CabinAtmoMixture.o)

PROGRAMMERS:
- ((Jason Harvey) (CACI) (Initial) (2024-10))

@{
*/

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
        static const unsigned int NBULK      = 4;           /**< ** (1)       Number of modeled bulk compounds in the fluid mixture. */
        static const unsigned int NTC        = 0;           /**< ** (1)       Number of modeled trace compounds in the fluid mixture. */
        static const unsigned int NFOMBULK   = 6;           /**< ** (1)       Number of mole fractions in the HLA FOM bulk fluid constituents array. */
        static const unsigned int NFOMTC     = 4;           /**< ** (1)       Number of mole fractions in the HLA FOM trace compound constituents array. */
        const double*             mCompoundSpecificHeats;   /**<    (J/mol/K) Specific heats of the chemical compounds in the air mixture. */
        double                    mMoleFractions[NFOMBULK]; /**<    (1)       Mole fractions of bulk fluid compounds in the air mixture. */
        double                    mTcMoleFractions[NFOMTC]; /**<    (1)       Mole fractions of trace compounds in the air mixture. */
        double                    mSpecificHeat;            /**<    (J/mol/K) Specific heat of this mixture. */
        /// @brief  Default constructs this Simple Cabin Atmosphere Air Mixture Data.
        CabinAtmoMixture();
        /// @brief  Default destructs this Simple Cabin Atmosphere Air Mixture Data.
        virtual ~CabinAtmoMixture();
        /// @brief  Assigns this Simple Cabin Atmosphere Air Mixture Data.
        CabinAtmoMixture& operator =(const CabinAtmoMixture& that);
        /// @brief  Initializes this Simple Cabin Atmosphere Air Mixture Data with initial mole fractions.
        void initialize(const double* moleFractions, const double* specificHeats);
        /// @brief  Updates the specific heat for the current mixture.
        void updateSpecificHeat();
        /// @brief  Copies these mole fractions to the given mole fraction array.
        void writeMoleFractions(double* moleFractions) const;
        /// @brief  Copies mole fractions from the given mole fraction array.
        void readMoleFractions(const double* moleFractions);
        /// @brief  Mixes the given quantity and mole fraction with a relative quantity of the current mixture.
        void mix(const double oldMoles, const double addMoles, const double* addFractions);

    private:
        /// @brief Checks if the mole fractions array sums to 1.
        bool checkMoleFractionsSum() const;
        /// @brief Normalizes the mole fractions array to sum to 1.
        void normalize();
        /// @brief  Copy constructor unavailable since declared private and not implemented.
        CabinAtmoMixture(const CabinAtmoMixture&);
};

/// @}

#endif
