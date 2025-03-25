/*!
@file TrickHLA/TSyncCTETimeline.hh
@ingroup TrickHLA
@brief The implementation of a TSync CTE timeline.

This is a baseline implementation based off of the TSync hardware clock.
It is intended to provide the definition of the CTE Timeline interface.

TSync Driver:
https://safran-navigation-timing.com/portal/public-downloads/latest-tsyncpcie-update-files/

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
@trick_link_dependency{../../source/TrickHLA/TSyncCTETimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_TSYNC_CTE_TIMELINE_HH
#define TRICKHLA_TSYNC_CTE_TIMELINE_HH

// System include files.
#include <time.h>

// Trick include files.
#include "trick/Clock.hh"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/Timeline.hh"

#if !defined( SWIG )
#   if !defined( __linux__ )
#      error "The TSync Central Timing Equipment (CTE) card is only supported on Linux."
#   endif

extern "C" {
#   include "tsync.h" // cppcheck-suppress [missingInclude]
}
#endif

namespace TrickHLA
{

class TSyncCTETimeline : public CTETimelineBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TSyncCTETimeline();

  public:
   std::string full_device_name; ///< @trick_units{--} TSync board device name.

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA TSyncCTETimeline class. */
   TSyncCTETimeline();
   /*! @brief Destructor for the TrickHLA TSyncCTETimeline class. */
   virtual ~TSyncCTETimeline();

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

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for TSyncCTETimeline class.
    *  @details This constructor is private to prevent inadvertent copies. */
   TSyncCTETimeline( TSyncCTETimeline const &rhs );
   /*! @brief Assignment operator for TSyncCTETimeline class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   TSyncCTETimeline &operator=( TSyncCTETimeline const &rhs );

   TSYNC_BoardHandle board_handle; ///< @trick_units{--} TSync board handle.
};

} // namespace TrickHLA

#endif // TRICKHLA_TSYNC_CTE_TIMELINE_HH: Do NOT put anything after this line!
