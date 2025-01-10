/*!
@file TrickHLA/ScenarioTimeline.hh
@ingroup TrickHLA
@brief This class represents the scenario timeline.

\par<b>Assumptions and Limitations:</b>
- Instances of this class represent the timeline for the scenario associated
with the problem.
- The time scale for this timeline is always Terrestrial Time (TT) which
complies with the Space Reference FOM standard.
- Note that the epoch value for this CTE timeline represents the epoch or
starting point of the CTE timeline. This will correspond to the starting
time in the TT time standard represented in Truncated Julian Date format
(TJD) expressed in seconds.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/ScenarioTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/SimTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_SCENARIO_TIMELINE_HH
#define TRICKHLA_SCENARIO_TIMELINE_HH

// System include files.
#include <cstdint>

// TrickHLA include files.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/SimTimeline.hh"
#include "TrickHLA/Timeline.hh"

namespace TrickHLA
{

class ScenarioTimeline : public Timeline
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ScenarioTimeline();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Initialization constructor for the TrickHLA ScenarioTimeline class.
    *  @param sim_timeline Simulation time line.
    *  @param tt0 Scenario time epoch in TT seconds TJD format.
    *  @param st0 Simulation starting time offset from epoch. */
   explicit ScenarioTimeline( SimTimeline &sim_timeline,
                              double const tt0 = 0.0,
                              double const st0 = 0.0 );

   /*! @brief Destructor for the TrickHLA ScenarioTimeline class. */
   virtual ~ScenarioTimeline();

   //-----------------------------------------------------------------
   // This is virtual function must be defined by a full class.
   //-----------------------------------------------------------------
   /*! @brief Get the current scenario time.
    *  @return Current scenario time in seconds. */
   virtual double const get_time();

   /*! Get the minimum time resolution which is the smallest nonzero
    *  time for the given timeline.
    *  @return Returns the minimum time resolution in seconds. */
   virtual double const get_min_resolution();

   //
   // Additional function specific to this implementation.
   //
   /*! @brief Compute a simulation time from a given scenario time.
    *  @return Simulation time in seconds.
    *  @param scenario_time Desired scenario time in seconds. */
   virtual double const compute_simulation_time( double const scenario_time );

   /*! @brief Compute a scenario time from a given simulation time.
    *  @return Scenario time in seconds.
    *  @param sim_time Desired simulation time. */
   virtual double const time_from_simulation_time( double const sim_time );

   /*! @brief Compute a HLA Logical Time (HLT) from a given scenario time.
    *  @return HLT in the base HLA Logical Time representation.
    *  @param scenario_time Desired scenario time. */
   virtual Int64Time const compute_HLT( double const scenario_time );

   /*! @brief Compute a scenario time from and given HLA Logical Time (HLT).
    *  @return Scenario time in seconds.
    *  @param hlt Desired HLT in the base HLA Logical Time representation. */
   virtual double const time_from_HLT( Int64Time const &hlt );

   // Accessor functions.
   /*! @brief Get the offset of the simulation time line from the scenario
    * timeline epoch.
    *  @return Offset time in seconds. */
   virtual double const get_sim_offset()
   {
      return this->sim_offset;
   }

   /*! @brief Set the offset of the simulation time line from the scenario
    * timeline epoch.
    *  @param st0 The offset time in seconds. */
   virtual void set_sim_offset( double const st0 )
   {
      this->sim_offset = st0;
   }

   /*! @brief Get the offset of the HLA Logical Time (HLT) timeline from the
    * scenario timeline.
    *  @return Offset in the base HLA Logical Time representation. */
   virtual Int64Time const get_HLT_offset()
   {
      return this->hlt_offset;
   }

   /*! @brief Set the offset of the HLA Logical Time (HLT) timeline from the
    * scenario timeline.
    *  @param hlt0 Desired offset in the base HLA Logical Time representation. */
   virtual void set_HTL_offset( Int64Time const &hlt0 )
   {
      this->hlt_offset = hlt0;
   }

  protected:
   SimTimeline &sim_timeline; ///< @trick_io{**} Reference to simulation timeline.

   double sim_offset; /*!< @trick_units{s} The offset of the simulation timeline
                           from the scenario timeline epoch. For early joiners,
                           this number will usually be 0.0. However, for
                           late joiners, this will provide the starting
                           offset of the simulation with respect to the
                           original start of the federation execution. */

   Int64Time hlt_offset; /*!< @trick_units{us} The offset of the HLA Logical
                              Time (HLT) timeline from the scenario timeline
                              epoch. */

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ScenarioTimeline class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ScenarioTimeline( ScenarioTimeline const &rhs );

   /*! @brief Assignment operator for ScenarioTimeline class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ScenarioTimeline &operator=( ScenarioTimeline const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_SCENARIO_TIMELINE_HH: Do NOT put anything after this line!
