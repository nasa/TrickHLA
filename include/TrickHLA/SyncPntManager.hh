/*!
@file TrickHLA/SyncPntManager.hh
@ingroup TrickHLA
@brief This class will manage different lists of HLA synchronization points. It
       is intended for this class to be extended by an Execution Control class.

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

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/SyncPntManager.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPnt.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2024, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_PNT_MANAGER_HH
#define TRICKHLA_SYNC_PNT_MANAGER_HH

// System includes.
#include <map>
#include <string>
#include <vector>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/SyncPntLoggable.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace TrickHLA
{

typedef std::vector< SyncPnt * >                SyncPntListVec;
typedef std::map< std::string, SyncPntListVec > SyncPntListMap;

static std::string const UNKNOWN_SYNC_PNT_LIST = "UNKNOWN_SYNC_PNT_LIST";

class SyncPntManager
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPntManager();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SyncPntManager class. */
   SyncPntManager( Federate *federate );

   /*! @brief Pure virtual destructor for the TrickHLA SyncPntManager class. */
   virtual ~SyncPntManager() = 0;

  protected:
   /*! @brief Add the given synchronization point label to the named list.
    *  @param label Synchronization point label.
    *  @param list_name The name of the list of sync-points to add to.
    *  @return True if the synchronization point is added, false otherwise. */
   bool add_sync_point_list( std::string const &list_name );

   SyncPnt *get_sync_point( std::wstring const &label ); // Search all lists for the unique sync-point label.

   SyncPntListVec get_sync_point_list( std::string const &list_name );

   bool remove_sync_point_list( std::string const &list_name );

   bool add_sync_point( std::wstring const &label, std::string const &list_name );

   bool add_sync_point( std::wstring const &label, std::string const &list_name, Int64Time time );

   bool contains_sync_point( std::wstring const &label );

   bool contains_sync_point_list_name( std::string const &list_name );

   bool is_sync_point_registered( std::wstring const &label );

   bool mark_sync_point_registered( std::wstring const &label );

   bool register_sync_point( std::wstring const &label );

   bool register_sync_point( std::wstring const &label, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool register_all_sync_points( std::string const &list_name );

   bool register_all_sync_points( std::string const &list_name, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool register_sync_point( SyncPnt *sp );

   bool register_sync_point( SyncPnt *sp, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool is_sync_point_announced( std::wstring const &label );

   bool mark_sync_point_announced( std::wstring const &label );

   bool wait_for_sync_point_announced( std::wstring const &label );

   bool wait_for_all_sync_points_announced( std::string const &list_name );

   bool wait_for_sync_point_announced( SyncPnt *sp );

   bool is_sync_point_achieved( std::wstring const &label );

   bool achieve_sync_point( std::wstring const &label );

   bool achieve_all_sync_points( std::string const &list_name );

   bool achieve_sync_point( SyncPnt *sp );

   bool is_sync_point_synchronized( std::wstring const &label );

   bool mark_sync_point_synchronized( std::wstring const &label );

   bool wait_for_sync_point_synchronized( std::wstring const &label );

   bool wait_for_all_sync_points_synchronized( std::string const &list_name );

   bool wait_for_sync_point_synchronized( SyncPnt *sp );

   // Callbacks from FedAmb.
   virtual void sync_point_registration_succeeded( std::wstring const &label );

   virtual void sync_point_registration_failed( std::wstring const &label, RTI1516_NAMESPACE::SynchronizationPointFailureReason reason );

   virtual void sync_point_announced( std::wstring const &label, RTI1516_NAMESPACE::VariableLengthData const &user_supplied_tag );

   virtual void sync_point_federation_synchronized( std::wstring const &label );

  protected:
   MutexLock mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

   SyncPntListMap sync_pnt_lists; ///< @trick_io{**} Map of named sync-point lists.

   Federate *federate; ///< @trick_units{--} Associated TrickHLA Federate.

  private:
   /*! @brief Default constructor for the TrickHLA SyncPntManager class. */
   SyncPntManager();

   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPntManager class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPntManager( SyncPntManager const &rhs );
   /*! @brief Assignment operator for SyncPntManager class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPntManager &operator=( SyncPntManager const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_PNT_MANAGER_HH */
