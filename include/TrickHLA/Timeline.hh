/*!
@file TrickHLA/Timeline.hh
@ingroup TrickHLA
@brief This class is the abstract base class for representing timelines.

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
@trick_link_dependency{../../source/TrickHLA/Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, April 2016, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_TIMELINE_HH
#define TRICKHLA_TIMELINE_HH

// System include files.
#include <cfloat>

namespace TrickHLA
{

class Timeline
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Timeline();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Initialization constructor for the TrickHLA Timeline class.
    *  @param t0 Epoch for the timeline. */
   explicit Timeline( double const t0 = 0.0 );

   /*! @brief Pure virtual destructor for the TrickHLA Timeline class. */
   virtual ~Timeline() = 0;

   //-----------------------------------------------------------------
   // This is a pure virtual function and must be defined by a full class.
   //-----------------------------------------------------------------
   /*! @brief Get the current time for this timeline in seconds.
    *  @return Returns the current timeline time in seconds. */
   virtual double const get_time() = 0;

   /*! @brief Get the minimum time resolution which is the smallest nonzero
    *  time for the given timeline.
    *  @return Returns the minimum time resolution in seconds. */
   virtual double const get_min_resolution() = 0;

   //-----------------------------------------------------------------
   // These are virtual functions for the class.
   //-----------------------------------------------------------------
   /*! @brief Get the elapsed time for this timeline in seconds from epoch.
    *  @return Returns the elapsed time from epoch in seconds. */
   virtual double const get_elapsed_time()
   {
      return ( get_time() - epoch );
   }

   /*! @brief Set the epoch for this timeline in seconds.
    *  @param time New time value for epoch in seconds. */
   virtual void set_epoch( double const time )
   {
      this->epoch = time;
   }

   /*! @brief Get the epoch for this timeline in seconds.
    *  @return Returns the epoch for this timeline in seconds. */
   virtual double const get_epoch()
   {
      return ( this->epoch );
   }

   /*! @brief Convert value to a time on the timeline with the minimum time resolution.
    *  @return Returns the time in seconds on the timeline with the minimum resolution.
    *  @param value The time value to convert. */
   virtual double const convert( double const value )
   {
      double const min_resolution = get_min_resolution();
      if ( min_resolution > DBL_MIN ) {
         // Compute the time in tics, which truncates to a fixed-point number.
         long long const time_tics = (long long)( value / min_resolution );

         // Convert to a time in seconds with the minimum time resolution.
         return (double)( time_tics * min_resolution );
      }
      return ( value );
   }

  protected:
   double epoch; /**<  @trick_units{s}
      Epoch for the simulation. This is the value of the timeline when the
      execution starts up. This value is often zero but is not required to
      be zero. */

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Timeline class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Timeline( Timeline const &rhs );

   /*! @brief Assignment operator for Timeline class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Timeline &operator=( Timeline const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_TIMELINE_HH: Do NOT put anything after this line!
