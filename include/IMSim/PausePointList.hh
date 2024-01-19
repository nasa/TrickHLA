/*!
@file IMSim/PausePointList.hh
@ingroup IMSim
@brief This class provides a mechanism for storing and managing HLA
synchronization points for Trick.

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

@python_module{IMSim}

@tldh
@trick_link_dependency{../../source/TrickHLA/TimedSyncPntList.cpp}
@trick_link_dependency{../../source/IMSim/PausePointList.cpp}
@trick_link_dependency{../../source/IMSim/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Systems Corp., DIS, Oct 2004, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef IMSIM_PAUSE_POINT_LIST_HH
#define IMSIM_PAUSE_POINT_LIST_HH

// System includes.// System include files.
#include <string>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/TimedSyncPntList.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// IMSim include files.
#include "IMSim/Types.hh"

namespace IMSim
{

class PausePointList : public TrickHLA::TimedSyncPntList
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrIMSim__PausePointList();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA PausePointList class. */
   PausePointList();

   /*! @brief Destructor for the TrickHLA PausePointList class. */
   virtual ~PausePointList()
   {
      return;
   }

  public:
   /*! @brief Clear the given synchronization point label.
    *  @return True if synchronization point is cleared.
    *  @param label The synchronization point label. */
   virtual bool clear_sync_point( std::wstring const &label );

   /*! @brief Check the state of the sync-points to determine if the state
    * needs to go to Freeze or Run. */
   void check_state();

   PausePointStateEnum get_state() const
   {
      return ( this->state );
   }

   void clear_state()
   {
      this->state = IMSim::PAUSE_POINT_STATE_UNKNOWN;
   }

   bool should_run() const
   {
      return ( this->state == PAUSE_POINT_STATE_RUN );
   }

   bool should_freeze() const
   {
      return ( this->state == PAUSE_POINT_STATE_FREEZE );
   }

   bool should_exit() const
   {
      return ( this->state == PAUSE_POINT_STATE_EXIT );
   }

   bool should_restart() const
   {
      return ( this->state == PAUSE_POINT_STATE_RESTART );
   }

   bool should_reconfig() const
   {
      return ( this->state == PAUSE_POINT_STATE_RECONFIG );
   }

   /*! @brief Returns a wide string representing the state of the
    *  synchronization points.
    *  @return String summary of synchronization points. */
   std::wstring to_wstring();

   /*! @brief Dumps synchronization point information to the screen. */
   virtual void print_sync_points();

  private:
   PausePointStateEnum state; ///< @trick_units{--} State of the pause points.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for PausePointList class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PausePointList( PausePointList const &rhs );
   /*! @brief Assignment operator for PausePointList class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PausePointList &operator=( PausePointList const &rhs );
};

} // namespace IMSim

#endif /* IMSIM_PAUSE_POINT_LIST_HH */
