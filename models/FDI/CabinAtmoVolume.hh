#ifndef CabinAtmoVolume_EXISTS
#define CabinAtmoVolume_EXISTS

/**
@file     CabinAtmoVolume.hh
@brief    Simple Cabin Atmosphere Volume Model declarations

@copyright Copyright 2024 United States Government as represented by the Administrator of the
           National Aeronautics and Space Administration.  All Rights Reserved.

@details
PURPOSE:
- (This models an air volume within the Simple Cabin Atmosphere model.  This includes the
   state of the air, and a Fluid Distributed Interface for exchanging air between this
   volume's counterpart in another distributed model.)

REFERENCE:
- (TBD)

ASSUMPTIONS AND LIMITATIONS:
- ((The air state is assumed to be homogeneous throughout the volume.)
   (Ideal Gas Law is assumed.))

LIBRARY DEPENDENCY:
- (CabinAtmoVolume.o)

PROGRAMMERS:
- ((Jason Harvey) (CACI) (Initial) (2024-10))

@{
*/

#include "CabinAtmoMixture.hh"
#include <string>
#include "Distributed2WayBusFluid.hh"

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Simple Cabin Atmosphere Volume Model Configuration Data.
///
/// @details  This holds the configuration data for an air volume model, including the initial air
///           state, and whether this is the master side of the Fluid Distributed Interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoVolumeConfigData
{
    public:
        double        mVolume;        /**< (m3)      cio(**) Air volume. */
        double        mTemperature;   /**< (K)       cio(**) Air temperature. */
        double        mPressure;      /**< (Pa)      cio(**) Air pressure. */
        const double* mMoleFractions; /**< (1)       cio(**) Compound mole fractions of the air mixture. */
        const double* mCompoundCp;    /**< (J/mol/K) cio(**) Specific heats of the chemical compounds in the air mixture.  */
        bool          mIsIfMaster;    /**< (1)       cio(**) This is the master side of the Fluid Distributed Interface pairing. */
        bool          mIsIfEnthalpy;  /**< (1)       cio(**) The Fluid Distributed Interface transports energy as specific enthalpy instead of temperature. */
        /// @brief  Default constructs this Simple Cabin Atmosphere Volume Model Configuration Data.
        CabinAtmoVolumeConfigData(const double  volume        = 0.0,
                                  const double  temperature   = 0.0,
                                  const double  pressure      = 0.0,
                                  const double* moleFractions = 0,
                                  const double* compoundCp    = 0,
                                  const bool    isIfMaster    = false,
                                  const bool    isIfEnthalpy  = false);
        /// @brief  Default destructs this Simple Cabin Atmosphere Volume Model Configuration Data.
        virtual ~CabinAtmoVolumeConfigData();

    private:
        /// @brief  Copy constructor unavailable since declared private and not implemented.
        CabinAtmoVolumeConfigData(const CabinAtmoVolumeConfigData&);
        /// @brief  Assignment operator unavailable since declared private and not implemented.
        CabinAtmoVolumeConfigData& operator =(const CabinAtmoVolumeConfigData&);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Simple Cabin Atmosphere Volume Model.
///
/// @details  This models an air volume within the Simple Cabin Atmosphere model.  This includes the
///           state of the air, and a Fluid Distributed Interface for exchanging air between this
///           volume's counterpart in another distributed model.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CabinAtmoVolume
{
    public:
        const CabinAtmoVolumeConfigData*  mConfig;       /**<    (1) cio(**) The configuration data. */
        double                            mTemperature;  /**<    (K)         Air temperature. */
        double                            mPressure;     /**<    (Pa)        Air pressure. */
        double                            mMoles;        /**<    (mol)       Air total moles. */
        CabinAtmoMixture                  mMixture;      /**<    (1)         Air mixture data. */
        double                            mEnthalpy;     /**<    (J/mol)     Air molar specific enthalpy. */
        double                            mCapacitance;  /**<    (mol/Pa)    Capacitance of the internal model at this volume location. */
        Distributed2WayBusFluid           mIf;           /**<    (1)         The Distributed Fluid Interface at this volume. */
        Distributed2WayBusFluidFluidState mIfFluid;      /**<    (1)         Working fluid state of the Distributed Fluid Interface. */
        Distributed2WayBusFluidFlowState  mIfFlow;       /**<    (1)         Working flow state of the Distributed Fluid Interface. */
        bool                              mIfDataValid;  /**<    (1)         Received data in the interface is valid. */
        double                            mIfDemandLim;  /**<    (mol/s)     Demand limit of the Distributed Interface. */
        double                            mInflowRate;   /**<    (mol/s)     Net flow rate into the volume. */
        double                            mIfInflowN;    /**<    (mol)       Moles of internal flows to the Distributed Interface Demand role. */
        double                            mIfInflowH;    /**<    (J/mol)     Enthalpy of internal flows to the Distributed Interface Demand role. */
        CabinAtmoMixture                  mIfMixIn;      /**<    (1)         Working mixture object for flows from the Distributed Interface into this model. */
        CabinAtmoMixture                  mIfMixOut;     /**<    (1)         Working mixture object for flows from this model out to the Distributed Interface. */
        static const double               R_UNIV;        /**< ** (J/mol/K)   Universal gas constant. */
        /// @brief  Default constructs this Simple Cabin Atmosphere Volume Model.
        CabinAtmoVolume(const std::string& name = "");
        /// @brief  Default destructs this Simple Cabin Atmosphere Volume Model.
        virtual ~CabinAtmoVolume();
        /// @brief  Initializes this Simple Cabin Atmosphere Volume Model with its configuration data.
        void initialize(const CabinAtmoVolumeConfigData& config);
        /// @brief  Updates the total moles of air in this volume.
        void updateMoles();
        /// @brief  Updates the air pressure in this volume.
        void updatePressure();
        /// @brief  Computes the air pressure in this volume for a given moles.
        double computePressure(const double moles) const;
        /// @brief  Updates the air temperature in this volume.
        void updateTemperature();
        /// @brief  Computes the temperature in this volume for a given specific enthalpy.
        double computeTemperature(const double enthalpy) const;
        /// @brief  Update the air specific enthalpy in this volume.
        void updateEnthalpy();
        /// @brief  Compute the specific enthalpy in this volume for a given temperature.
        double computeEnthalpy(const double temperature) const;
        /// @brief  Compute the local capacitance of this volume.
        double computeCapacitance() const;
        /// @brief  Updates the Fluid Distributed Interface prior to the main model update.
        void updateIfPre(const double dt, const double demandSideP);
        /// @brief  Updates the Fluid Distributed Interface following the main model update.
        void updateIfPost();
        /// @brief  Adds the given amount of a fluid mixture to the contents in this volume.
        void addMixture(const double moles, const double enthalpy, const double* moleFractions);
        /// @brief  Adds the given amount of a fluid mixture to the contents in this volume.
        void addMixture(const double moles, const double enthalpy, const CabinAtmoMixture& mixture);
        /// @brief  Removes the given amount of moles from this volume.
        void removeMoles(const double moles);
        /// @brief  Returns the specific enthalpy represented in the Fluid Distributed Interface energy field.
        double computeIfEnthalpy(const double energy, const double specificHeat) const;
        /// @brief  Returns the energy represented in the Fluid Distributed Interface energy field.
        double computeIfEnergy(const double temperature, const double specificHeat) const;

    protected:
        std::string mName; /**< (1) cio(**) Name of this volume object for messages. */
        /// @brief  Validates the configuration data.
        void validateConfig() const;
        /// @brief  Converts the given variable name string for giving to the Trick Memory Manager.
        void convertNameForTmm(std::string& name) const;
        /// @brief  Applies lower bound limit on air moles.
        void limitMoles();

    private:
        /// @brief  Copy constructor unavailable since declared private and not implemented.
        CabinAtmoVolume(const CabinAtmoVolume&);
        /// @brief  Assignment operator unavailable since declared private and not implemented.
        CabinAtmoVolume& operator =(const CabinAtmoVolume&);
};

#endif
