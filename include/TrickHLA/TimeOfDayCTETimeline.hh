/*!
@file TrickHLA/TimeOfDayCTETimeline.hh
@ingroup TrickHLA
@brief This class represents the CTE timeline.

\par<b>Assumptions and Limitations:</b>
- Instances of this class represent the timeline for the CTE associated with
the problem.
- The time scale for this timeline is always Terrestrial Time (TT) which
complies with the Space Reference FOM standard.
- Note that the epoch value for this CTE timeline represents the epoch or
starting point of the CTE timeline. This will correspond to the starting
time in the TT time standard represented in Truncated Julian Date format
(TJD) expressed in seconds.

@copyright Copyright 2025 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/CTETimelineBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Timeline.cpp}
@trick_link_dependency{../../source/TrickHLA/TimeOfDayCTETimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2025, --, Rewrite of CTE Timeline into this class.}
@revs_end

*/

#ifndef TRICKHLA_TIMEOFDAY_CTE_TIMELINE_HH
#define TRICKHLA_TIMEOFDAY_CTE_TIMELINE_HH

// System include files.
#include <time.h>

// Trick include files.
#include "trick/Clock.hh"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/Timeline.hh"

namespace TrickHLA
{

class TimeOfDayCTETimeline : public CTETimelineBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TimeOfDayCTETimeline();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA TimeOfDayCTETimeline class. */
   TimeOfDayCTETimeline();
   /*! @brief Destructor for the TrickHLA TimeOfDayCTETimeline class. */
   virtual ~TimeOfDayCTETimeline();

   //-----------------------------------------------------------------
   // These virtual function must be defined by a full class.
   //-----------------------------------------------------------------
   // Virtual TrickHLATimeline functions.

   /*! Get the minimum time resolution which is the smallest nonzero
    *  time for the given timeline.
    *  @return Returns the minimum time resolution in seconds. */
   virtual double const get_min_resolution();

   /*! @brief Update the clock tics per second resolution of this clock
    *  to match the Trick executive resolution. */
   virtual void set_clock_tics_per_sec( int const tics_per_sec );

   /*! @brief Get the current CTE time.
    *  @return Current time of day in seconds. */
   virtual double const get_time();

   /*! @brief Initialize the Trick::Clock functions. */
   virtual int clock_init();

   /*! @brief Get the wall clock time.
    *  @return The current real time as a count of microseconds. */
   virtual long long wall_clock_time();

   /*! @brief Stop the CTE clock.
    *  @return Default implementation always returns 0. */
   virtual int clock_stop();

   /*! @brief Sets the clock ID (system clock type). */
   virtual void set_clock_ID( clockid_t const id );

   /*! @brief Gets the current clock ID (system clock type).
    *  @return The system clock type in use. */
   virtual clockid_t const get_clock_ID();

  protected:
   clockid_t clk_id; /**< @trick_io{**} System clock type used. The default clock ID is <i>CLOCK_MONOTONIC</i>. */

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for TimeOfDayCTETimeline class.
    *  @details This constructor is private to prevent inadvertent copies. */
   TimeOfDayCTETimeline( TimeOfDayCTETimeline const &rhs );
   /*! @brief Assignment operator for TimeOfDayCTETimeline class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   TimeOfDayCTETimeline &operator=( TimeOfDayCTETimeline const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_TIMEOFDAY_CTE_TIMELINE_HH: Do NOT put anything after this line!
