#ifndef CabinAtmo_EXISTS
#define CabinAtmo_EXISTS

/**
@ingroup DistIf
@file    CabinAtmo.hh
@brief   Simple Cabin Atmosphere Model declarations

@details This implements a simple cabin atmosphere model for demonstrating the
Fluid Distributed Interface design.

\par<b>Assumptions and Limitations</b>
- Simplification: valve, hatch & fan flows are linear with pressure.
- Ideal Gas Law is assumed.
- All air compounds are assumed calorically perfect with constant specific heat.
- Specific enthalpy is only a function of temperature, as h = Cp*T

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
@trick_link_dependency{../src/CabinAtmo.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

@{
*/

#include <string>

#include "CabinAtmoConserveChecks.hh"
#include "CabinAtmoVolume.hh"

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Simple Cabin Atmosphere Model Configuration Data
///
/// @details  This holds the configuration and initial state data for the model, including the
///           volumes, valves, fan and hatch.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoConfigData
{
  public:
   double                    mCompoundCp[CabinAtmoMixture::NBULK];       /**< (J/mol/K)  cio(**) Specific heat constants for each compound in the air mixture. */
   double                    mCabinMixture[CabinAtmoMixture::NBULK];     /**< (1)        cio(**) Initial air mixture in the main cabin volume. */
   double                    mVestibuleMixture[CabinAtmoMixture::NBULK]; /**< (1)        cio(**) Initial air mixture in the vestibule volume. */
   double                    mImvDuctMixture[CabinAtmoMixture::NBULK];   /**< (1)        cio(**) Initial air mixture in the IMV duct volume. */
   CabinAtmoVolumeConfigData mCabin;                                     /**< (1)        cio(**) Configuration data for the main cabin volume. */
   CabinAtmoVolumeConfigData mVestibule;                                 /**< (1)        cio(**) Configuration data for the vestibule volume. */
   CabinAtmoVolumeConfigData mImvDuct;                                   /**< (1)        cio(**) Configuration data for the IMV duct volume. */
   double                    mImvFanMaxQ;                                /**< (mol/s)    cio(**) Maximum flow rate of the IMV fan. */
   double                    mImvFanMaxDp;                               /**< (Pa)       cio(**) Maximum delta-pressure of the IMV fan. */
   double                    mHatchG;                                    /**< (mol/s/Pa) cio(**) Flow conductance through the open hatch. */
   double                    mMpevG;                                     /**< (mol/s/Pa) cio(**) Flow conductance through the open MPEV. */
   double                    mGrillValveG;                               /**< (mol/s/Pa) cio(**) Flow conductance through the open grill valve. */
   double                    mImvValveG;                                 /**< (mol/s/Pa) cio(**) Flow conductance through the open IMV valve. */
   bool                      mHatchOpen;                                 /**< (1)        cio(**) Initial position of the hatch. */
   bool                      mMpevOpen;                                  /**< (1)        cio(**) Initial position of the MPEV. */
   bool                      mImvValveOpen;                              /**< (1)        cio(**) Initial position of the IMV valve. */
   bool                      mGrillValveOpen;                            /**< (1)        cio(**) Initial position of the grill valve. */
   bool                      mImvFanOn;                                  /**< (1)        cio(**) Initial running state of the IMV fan. */
   /// @brief  Default constructs this Simple Cabin Atmosphere Model Configuration Data.
   CabinAtmoConfigData();
   /// @brief  Default destructs this Simple Cabin Atmosphere Model Configuration Data.
   virtual ~CabinAtmoConfigData();

  private:
   /// @brief  Copy constructor unavailable since declared private and not implemented.
   CabinAtmoConfigData( CabinAtmoConfigData const & );
   /// @brief  Assignment operator unavailable since declared private and not implemented.
   CabinAtmoConfigData &operator=( CabinAtmoConfigData const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Simple Cabin Atmosphere Model
///
/// @details  This is a simple 'fan in a can' vehicle cabin model, consisting of a main cabin
///           volume, a vestibule volume separated by parallel hatch and equalization valves, and an
///           Inter-Module Ventilation (IMV) duct volume separated from the cabin and vestibule
///           by valves.  The IMV duct also has a fan in series with the IMV-cabin valve for making
///           circulation flow around the 3-volume loop.
///
///           The vestibule and IMV duct volumes each have a Fluid Distributed Interface for
///           combining with their counterparts in another model instance, for simulating two
///           vehicles docked together.  The IMV fans can create circulation around the total loop
///           of the combined vehicle's IMV ducts, cabins and hatches.
///
///           The volumes can be initialized with varying temperature, pressure, and air mixture,
///           for demonstrating pressure equalization, air fan circulation, and energy and mixture
///           mixing between connected volumes, with or without the distributed interfaces between
///           two combined models.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmo
{
  public:
   CabinAtmoConfigData const  *mConfig;            /**< (1) cio(**) The configuration data. */
   CabinAtmoVolume             mCabin;             /**< (1)         The main cabin volume model. */
   CabinAtmoVolume             mVestibule;         /**< (1)         The vestibule volume model. */
   CabinAtmoVolume             mImvDuct;           /**< (1)         The IMV duct volume model. */
   bool                        mHatchOpen;         /**< (1)         Position of the hatch. */
   bool                        mMpevOpen;          /**< (1)         Position of the MPEV. */
   bool                        mImvValveOpen;      /**< (1)         Position of the IMV valve. */
   bool                        mGrillValveOpen;    /**< (1)         Position of the grill valve. */
   bool                        mImvFanOn;          /**< (1)         Running state of the IMV fan. */
   double                      mTimestep;          /**< (s)         Integration time step of the current update. */
   double                      mImvFanSourceP;     /**< (Pa)        IMV fan source pressure rise. */
   double                      mImvFanDp;          /**< (Pa)        IMV fan pressure rise, or delta pressure (outlet - inlet). */
   CabinAtmoConserveParameters mConserveParams;    /**< (1)         Parameters for checking conservation of mass and energy. */
   double                      mHatchFlow;         /**< (mol/s)     Flow rate through the hatch, positive from cabin to vestibule. */
   double                      mMpevFlow;          /**< (mol/s)     Flow rate through the MPEV, positive from cabin to vestibule. */
   double                      mImvFlow;           /**< (mol/s)     Flow rate through the IMV valve & fan, positive from cabin to IMV duct. */
   double                      mGrillValveFlow;    /**< (mol/s)     Flow rate through the grill valve, positive from vestibule to IMV duct. */
   double                      mA[3][3];           /**< (mol/s/Pa)  Admittance matrix of the system of equations. */
   double                      mSourceVector[3];   /**< (mol/s)    Source vector of the system of equations. */
   double                      mSolutionVector[3]; /**< (Pa)       Solution vector of the system of equations. */
   double                      mAinv[3][3];        /**< (Pa*s/mol) Inverse of the admittance matrix. */
   /// @brief  Default constructs this Simple Cabin Atmosphere Model.
   CabinAtmo( std::string const &name, CabinAtmoConfigData const *config );
   /// @brief  Default destructs this Simple Cabin Atmosphere Model.
   virtual ~CabinAtmo();
   /// @brief  Initializes this Simple Cabin Atmosphere Model.
   void initialize();
   /// @brief  Propagates this Simple Cabin Atmosphere Model by one time step.
   void step( double const dt );

  protected:
   std::string mName;     /**< *o (1) cio(**) Sim-unique name of this object for user notifications. */
   bool        mInitFlag; /**< *o (1) cio(**) This object has been initialized. */
   /// @brief  Validates the configuration data.
   void validateConfig() const;
   /// @brief  Updates the main model by one time step.
   void updateModel();
   /// @brief  Builds the system of equations.
   void buildSOE();
   /// @brief  Computes the inverse of the admittance matrix.
   void invertMatrix();
   /// @brief  Computes the pressure solution for the given source vector.
   void solvePressures( double *x, double const *b );
   /// @brief  Computes internal flows resulting from the pressure solution.
   void computeFlows();
   /// @brief  Transports fluid flows between the volumes and interfaces.
   void transportFlows();
   /// @brief  Re-computes the volume pressures after fluid transport.
   void updatePressures();
   /// @brief  Computes the interface effective capacitances.
   void computeCapacitance();
   /// @brief  Updates the IMV fan model.
   void updateFan();
   /// @brief  Computes total conserved parameters.
   void updateConservation();
   /// @brief  Limits the given input to the given range.
   static double limitRange( double const min, double const input, double const max );

  private:
   /// @brief  Copy constructor unavailable since declared private and not implemented.
   CabinAtmo( CabinAtmo const & );
   /// @brief  Assignment operator unavailable since declared private and not implemented.
   CabinAtmo &operator=( CabinAtmo const & );
};

} // namespace DistIf

/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @param[in] min   (--) The minimum value of the range.
/// @param[in] input (--) The input value to apply the range limit to.
/// @param[in] max   (--) The maximum value of the range.
///
/// @returns  double (--) Input limited to the range.
///
/// @details  Returns the given input limited to the range [min, max].
////////////////////////////////////////////////////////////////////////////////////////////////////
inline double DistIf::CabinAtmo::limitRange( double const min, double const input, double const max )
{
   return std::max( std::min( input, max ), min );
}

#endif
