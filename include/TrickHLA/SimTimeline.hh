/*!
@file TrickHLA/Attribute.hh
@ingroup TrickHLA
@brief This class represents the simulation timeline.

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
@trick_link_dependency{../../source/TrickHLA/SimTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, NExSyS, April 2016, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_SIM_TIMELINE_HH
#define TRICKHLA_SIM_TIMELINE_HH

// TrickHLA includes.
#include "Timeline.hh"

namespace TrickHLA
{

class SimTimeline : public Timeline
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SimTimeline();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SimTimeline class. */
   SimTimeline();

   /*! @brief Destructor for the TrickHLA SimTimeline class. */
   virtual ~SimTimeline();

   //-----------------------------------------------------------------
   // This is a virtual function and must be defined by a full class.
   //-----------------------------------------------------------------
   /*! @brief Get the current time for this timeline in seconds.
    *  @return Returns the current timeline time in seconds. */
   virtual double const get_time() const;

   /*! Get the minimum time resolution which is the smallest nonzero
    *  time for the given timeline.
    *  @return Returns the minimum time resolution in seconds. */
   virtual double const get_min_resolution() const;

   //
   // Additional function specific to this implementation.
   //

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SimTimeline class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SimTimeline( SimTimeline const &rhs );

   /*! @brief Assignment operator for SimTimeline class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SimTimeline &operator=( SimTimeline const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_SIM_TIMELINE_HH: Do NOT put anything after this line!
