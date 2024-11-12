#ifndef CabinAtmoConserveChecks_EXISTS
#define CabinAtmoConserveChecks_EXISTS

/**
@ingroup DistIf
@file    CabinAtmoConserveChecks.hh
@brief   Simple Cabin Atmosphere Conservation Checks declarations

@details This tracks conservation of total mass and energy between two CabinAtmo models.

@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

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
@trick_link_dependency{../src/CabinAtmoConserveChecks.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

*/

// TODO needed improves:
//  - work with HLA as well as standalone:
//    + lives on modelA side, retains local connection to modelA and modelB, but ignores
//      modelB reference in HLA (it belongs to the otehr pair) and instead get B side from
//      HLA
//    + new HLA object class for conservation data, publish by B sides: ConservationParams
//      - THLA object/attribute configuration input file, maps FOM ConservationParams to model's
//        CabinAtmoConserveParameters
//    + this subscribes to conservation data HLA and FluidDistIf out data from B side
//    + this lags A-side data by 1 frame (conservation and FluidDistIf out data) to match
//      timing of HLA data from B side
//  ? Include transported ndot in conserve calcs:
//    - FluidDistIf out data of the Demand side, when HLA
//    - when not HLA, lag buffer of the Demand side
//    - must handle FluidDistIf out data energy as temperature
//      - when A side is Demand role, can just use specific heat from the A side i/f volume.
//      - what if B side is Demand role?  No way to get B side specific heat unless we model it
//        - this is a show-stopper, so maybe just give up on the whole transport ndot inclusion

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Simple Cabin Atmosphere Conservation Check Parameters
///
/// @details  These are the parameters that are checked for conservation.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoConserveParameters
{
  public:
   /// These are unit-less so they can apply to actual values or error ratios, etc.
   double energy;   /**< (1) Parameter for energy. */
   double moles;    /**< (1) Parameter for total moles. */
   double molesN2;  /**< (1) Parameter for moles of N2. */
   double molesO2;  /**< (1) Parameter for moles of O2. */
   double molesH2O; /**< (1) Parameter for moles of H2O. */
   double molesCO2; /**< (1) Parameter for moles of CO2. */
   /// @brief Construct the conservation parameters object and initialize values to zero.
   CabinAtmoConserveParameters();
   /// @brief Destruct the conservation parameters object.
   virtual ~CabinAtmoConserveParameters();
   /// @brief Assignment operator for the conservation parameters object.
   CabinAtmoConserveParameters &operator=( CabinAtmoConserveParameters const &that );

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   CabinAtmoConserveParameters( CabinAtmoConserveParameters const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Simple Cabin Atmosphere Conservation Checks
///
/// @details  For each of the conservation parameters, this tracks their total values between two
///           CabinAtmo models, and computes their percent error relative to a set of reference
///           values.  This can be used to verify conservation of mass and energy is being modeled.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoConserveChecks
{
  public:
   CabinAtmoConserveParameters const &modelA;               /**< (1) Reference to model instance A conservation parameters. */
   CabinAtmoConserveParameters const &modelB;               /**< (1) Reference to model instance B conservation parameters. */
   CabinAtmoConserveParameters        modelAConserveParams; /**< (1) Conservation parameters input from model instance A. */
   CabinAtmoConserveParameters        modelBConserveParams; /**< (1) Conservation parameters input from model instance B. */
   bool                               isBsideHla;           /**< (1) True if the B side model is across the HLA interface. */
   bool                               setReference;         /**< (1) Set the reference values equal to the current values. */
   CabinAtmoConserveParameters        current;              /**< (1) Current total values of the conserved parameters between instance A and B. */
   CabinAtmoConserveParameters        reference;            /**< (1) Reference total values of the conserved parameters between instance A and B. */
   CabinAtmoConserveParameters        error;                /**< (1) Percent error in the current total values versus the reference. */
   /// @brief Construct the conservation checks object.
   CabinAtmoConserveChecks( CabinAtmoConserveParameters const &a, CabinAtmoConserveParameters const &b );
   /// @brief Destruct the conservation checks object.
   virtual ~CabinAtmoConserveChecks();
   /// @brief Updates the conservation checks and computes error values.
   void update();

  private:
   // Copy constructor unavailable since declared private and not implemented.
   CabinAtmoConserveChecks( CabinAtmoConserveChecks const & );
   // Assignment operator unavailable since declared private and not implemented.
   CabinAtmoConserveChecks &operator=( CabinAtmoConserveChecks const & );
};

} // namespace DistIf


#endif
