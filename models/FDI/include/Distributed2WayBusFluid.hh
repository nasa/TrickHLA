#ifndef Distributed2WayBusFluid_EXISTS
#define Distributed2WayBusFluid_EXISTS

/**
@ingroup  DistIf
@file     Distributed2WayBusFluid.hh
@brief    Fluid Distributed 2-Way Bus Interface declarations

@details Classes for the Fluid Distributed 2-Way Bus Interface.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

\par<b>References</b>
- https://github.com/nasa/gunns/wiki/Distributed-Bi-Directional-Flow-Fluid-Interface

@trick_parse{everything}

@python_module{DistIf}

@tldh
@trick_link_dependency{../src/Distributed2WayBusFluid.cpp}

@revs_title
@revs_begin
@rev_entry{Jason Harvey, CACI, TrickHLA, November 2024, --, Initial version.}
@revs_end

@{
*/

#include "Distributed2WayBusBase.hh"

namespace DistIf
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed Fluid Mixture Data
///
/// @details  This describes the intrinsic properties (energy & mixture) of a fluid for
///           communication across a distributed modeling interface.
///
/// @note     This must remain a base class, since it it used in multiple inheritance below, to
///           avoid the diamond inheritance problem.
////////////////////////////////////////////////////////////////////////////////////////////////////
class FluidDistributedMixtureData // !!! NOTE this must remain a base class, see above !!!
{
  public:
   double  mEnergy;          /**< (1)         Fluid temperature (K) or specific enthalpy (J/kg). */
   double *mMoleFractions;   /**< (1) cio(**) Fluid mole fractions. */
   double *mTcMoleFractions; /**< (1) cio(**) Trace compounds mole fractions. */
   /// @brief  Default constructs this Fluid Distributed mixture data.
   FluidDistributedMixtureData();
   /// @brief  Default destructs this Fluid Distributed mixture data.
   virtual ~FluidDistributedMixtureData();
   /// @brief  Assignment operator for this Fluid Distributed mixture data.
   FluidDistributedMixtureData &operator=( FluidDistributedMixtureData const &that );
   /// @brief  Allocates dynamic arrays for bulk fluid and trace compounds and mole fractions.
   virtual void initialize( unsigned int const nBulk,
                            unsigned int const nTc,
                            std::string const &name = "" );
   /// @brief  Sets the bulk fluid mole fractions to the given values.
   void setMoleFractions( double const *fractions, unsigned int const size );
   /// @brief  Sets the trace compound mole fractions to the given values.
   void setTcMoleFractions( double const *fractions, unsigned int const size );
   /// @brief  Sets the given array to the bulk fluid mole fraction values.
   void getMoleFractions( double *fractions, unsigned int const size ) const;
   /// @brief  Sets the given array to the trace compound mole fraction values.
   void getTcMoleFractions( double *fractions, unsigned int const size ) const;
   /// @brief  Returns the size of this mixture data bulk fluid mole fractions array.
   unsigned int getNumFluid() const;
   /// @brief  Returns the size of this mixture data trace compounds mole fractions array.
   unsigned int getNumTc() const;

  protected:
   unsigned int mNumFluid; /**< *o (1) cio(**) Number of primary fluid compounds in this mixture. */
   unsigned int mNumTc;    /**< *o (1) cio(**) Number of trace compounds in this mixture. */

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   FluidDistributedMixtureData( FluidDistributedMixtureData const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed Fluid 2-Way Bus Interface Data
///
/// @details  This holds the data that is transferred between instances of
///           Distributed2WayBusFluid across the sim-sim interface (HLA, etc.)  The class
///           variables, including the base class variables, map to the HLA FOM.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusFluidInterfaceData : public FluidDistributedMixtureData, public Distributed2WayBusBaseInterfaceData
{
  public:
   double mCapacitance; /**< (mol/Pa) Model capacitance. */
   double mSource;      /**< (1)      Fluid pressure (Pa) or molar flow (mol/s). */
   /// @brief  Default constructs this Fluid Distributed 2-Way Bus interface data.
   Distributed2WayBusFluidInterfaceData();
   /// @brief  Default destructs this Fluid Distributed 2-Way Bus interface data.
   virtual ~Distributed2WayBusFluidInterfaceData();
   /// @brief  Returns whether this object has received valid data.
   virtual bool hasValidData() const;
   /// @brief Assignment operator for this Fluid Distributed 2-Way Bus interface data.
   Distributed2WayBusFluidInterfaceData &operator=( Distributed2WayBusFluidInterfaceData const &that );

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusFluidInterfaceData( Distributed2WayBusFluidInterfaceData const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed Fluid State Data
///
/// @details  This describes the properties of a fluid state (pressure, energy & mixture) for
///           communication across a distributed modeling interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusFluidFluidState : public FluidDistributedMixtureData
{
  public:
   double mPressure; /**< (Pa) Fluid pressure. */
   /// @brief Default constructor.
   Distributed2WayBusFluidFluidState();
   /// @brief Default destructor.
   virtual ~Distributed2WayBusFluidFluidState();
   /// @brief Assignment operator.
   Distributed2WayBusFluidFluidState &operator=( Distributed2WayBusFluidFluidState const &that );

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusFluidFluidState( Distributed2WayBusFluidFluidState const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Distributed Fluid Flow State Data
///
/// @details  This describes the properties of a fluid flow (flow rate, energy & mixture) for
///           communication across a distributed modeling interface.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusFluidFlowState : public FluidDistributedMixtureData
{
  public:
   double mFlowRate; /**< (mol/s) Fluid molar flow rate. */
   /// @brief Default constructor.
   Distributed2WayBusFluidFlowState();
   /// @brief Default destructor.
   virtual ~Distributed2WayBusFluidFlowState();
   /// @brief Assignment operator.
   Distributed2WayBusFluidFlowState &operator=( Distributed2WayBusFluidFlowState const &that );

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusFluidFlowState( Distributed2WayBusFluidFlowState const & );
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief    Fluid Distributed 2-Way Bus Interface.
///
/// @details  See the link in REFERENCE above.  Main features:
///           - Designed to be generic and reusable.
///           - No dependencies on any 3rd party code.
///           - Implements handshaking and coordination of bi-directional fluid flow between
///             distributed models.
///           - Interfaces with another instance of itself over the data interface (HLA, etc.)
///             - The side of the interface having the larger fluid capacitance is usually the
///               Supply role, supplies the Fluid State (pressure, energy, mixture) in the
///               interface volume to the other side, and receives the other side's demanded Flow
///               State (flow rate, energy, mixture) to be placed as a flow source on this side.
///             - The side of the interface having the smaller fluid capacitance is usually the
///               Demand role, receives the supplied Fluid State from the other side to be placed as
///               a boundary condition on this side, and returns the resulting flow rate between the
///               interface location and the local model as a Flow State demanded to the other side.
///             - The roles flip when the Supply side's capacitance drops to a certain threshold
///               below the Demand side's capacitance.  This helps ensure stability.
///             - The Demand role, which computes the Flow State across the interface, applies
///               limits on the flow rate to ensure stability of the interface.
///           - Depending on the interface Supply or Demand role, the local model receives either a
///             Fluid State or Flow State boundary condition to apply at the interface location.
///           - The local model continuously computes fluid capacitance (the ratio of delta-moles
///             over delta-pressure) and gives this to this interface.
///           - Depending on the interface Supply or Demand role, the local model either supplies
///             the local Fluid State or the interface volume or the Flow State of flows to/from the
///             volume.
///
///           The order of operations between the local model and this object during each model step
///           should be:
///           1.  this->mInData updated with incoming data from the interface data network.
///           2.  Model calls this->processInputs()
///           3.  Model calls this->isInDemandRole(), responds to role swap as needed.
///           4.  Model calls this->getFluidState() or getFlowState() based on role.
///           5.  Model applies the returned fluid or flow state boundary conditions to its interface
///               volume.
///           6.  If in Demand role, model calls this->computeDemandLimit()
///           7.  Model does its main update.  If in Demand role, applies the demand flow rate limit.
///           8.  Model calculates its local capacitance at the interface.
///           9.  Model calls this->setFluidState() or setFlowState() based on role.
///           10. Model calls this->processOutputs()
///           11. Model calls this->isInDemandRole(), responds to role swap as needed.
///           12. Model calls this->popNotification() recursively until no notifications left,
///               transfers notification to the sim's messaging system as desired.
///           13. this->mOutData transmitted across the interface data network to the other side.
////////////////////////////////////////////////////////////////////////////////////////////////////
class Distributed2WayBusFluid : public Distributed2WayBusBase
{
  public:
   Distributed2WayBusFluidInterfaceData mInData;  /**< (1) Input data received from the remote side. */
   Distributed2WayBusFluidInterfaceData mOutData; /**< (1) Output data to transmit to the remote side. */
   /// @brief Default Constructor.
   Distributed2WayBusFluid();
   /// @brief Default Destructor.
   virtual ~Distributed2WayBusFluid();
   /// @brief Initializes this Distributed 2-Way Bus Interface.
   void initialize( bool const         isPairMaster,
                    unsigned int const nIfBulk,
                    unsigned int const nIfTc );
   /// @brief Process received data prior to the model update.
   void processInputs();
   /// @brief Returns the limit on max flow between the Demand-side model and the interface volume.
   double computeDemandLimit( double const timestep, double const demandSidePressure );
   /// @brief Returns the stability filter gain used in the Demand-side flow rate limit.
   double getDemandLimitGain() const;
   /// @brief Special processing of data outputs from the model after the network update.
   void processOutputs( double const capacitance );
   /// @brief Sets the outgoing fluid state of the interface volume when in the Supply role.
   void setFluidState( Distributed2WayBusFluidFluidState const &fluid );
   /// @brief Gets the incoming fluid state of the interface volume when in the Demand role.
   bool getFluidState( Distributed2WayBusFluidFluidState &fluid ) const;
   /// @brief Sets the outgoing state of flows to/from the interface volume when in the Demand role.
   void setFlowState( Distributed2WayBusFluidFlowState const &flow );
   /// @brief Gets the incoming state of flows to/from the interface volume when in the Supply role.
   bool getFlowState( Distributed2WayBusFluidFlowState &flow ) const;

  protected:
   double              mDemandLimitGain;        /**<    (1)             The current Demand-side flow rate limit filter gain. */
   double              mDemandLimitFlowRate;    /**<    (mol/s)         The current Demand-side flow rate limit. */
   static double const mModingCapacitanceRatio; /**<    (1)     cio(**) Supply over Demand capacitance ratio for triggering mode flip. */
   static double const mDemandFilterConstA;     /**< ** (1)     cio(**) Demand filter gain constant A. */
   static double const mDemandFilterConstB;     /**< ** (1)     cio(**) Demand filter gain constant B. */
   /// @brief Handles several mode flip cases based on input data.
   void flipModesOnInput();
   /// @brief Flips to the Demand mode.
   void flipToDemandMode();
   /// @brief Flips to the Supply mode.
   void flipToSupplyMode();
   /// @brief Flips to the Demand mode based on capacitances.
   void flipModesOnCapacitance();

  private:
   /// @brief Copy constructor unavailable since declared private and not implemented.
   Distributed2WayBusFluid( Distributed2WayBusFluid const &that );
   /// @brief Assignment operator unavailable since declared private and not implemented.
   Distributed2WayBusFluid &operator=( Distributed2WayBusFluid const &that );
};

} // namespace DistIf

/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Returns the value of mNumFluid.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline unsigned int DistIf::FluidDistributedMixtureData::getNumFluid() const
{
   return mNumFluid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Returns the value of mNumTc.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline unsigned int DistIf::FluidDistributedMixtureData::getNumTc() const
{
   return mNumTc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @details  Returns the value of mDemandLimitGain.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline double DistIf::Distributed2WayBusFluid::getDemandLimitGain() const
{
   return mDemandLimitGain;
}

#endif
