/*!
@file TrickHLA/SyncPointManagerBase.hh
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
@trick_link_dependency{../../source/TrickHLA/SyncPointManagerBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPointList.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2024, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_POINT_MANAGER_BASE_HH
#define TRICKHLA_SYNC_POINT_MANAGER_BASE_HH

// System includes.
#include <map>
#include <string>
#include <vector>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPointList.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

#define SYNC_POINT_LIST_TMM_ARRAY 1

namespace TrickHLA
{

#if SYNC_POINT_LIST_TMM_ARRAY
// typedef SyncPointList **SyncPointList2DArray;
#else
typedef std::vector< SyncPointList * > SyncPointListVector;
#endif

static std::string const UNKNOWN_SYNC_POINT_LIST = "Unknown";

class SyncPointManagerBase : public TrickHLA::CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPointManagerBase();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SyncPointManagerBase class. */
   SyncPointManagerBase();
   /*! @brief Constructor for the TrickHLA SyncPointManagerBase class. */
   explicit SyncPointManagerBase( Federate *fed );

   /*! @brief Pure virtual destructor for the TrickHLA SyncPointManagerBase class. */
   virtual ~SyncPointManagerBase() = 0;

  public:
   void setup( Federate *fed );

   void clear();

   int const get_list_index_for_sync_point( std::wstring const &label ); // Search all lists for the unique sync-point label.

   int const get_list_index_for_list_name( std::string const &list_name );

   SyncPtStateEnum const get_sync_point_state( std::wstring const &label );

   /*! @brief Add the given synchronization point label to the named list.
    *  @param label Synchronization point label.
    *  @param list_name The name of the list of sync-points to add to.
    *  @return True if the synchronization point is added, false otherwise. */
   bool const add_sync_point_list( std::string const &list_name );

   bool const add_sync_point( std::wstring const &label, std::string const &list_name );

   bool const add_sync_point( std::wstring const &label, std::string const &list_name, Int64Time time );

   bool const contains_sync_point( std::wstring const &label );

   bool const contains_sync_point_list_name( std::string const &list_name );

   bool const is_sync_point_registered( std::wstring const &label );

   bool const mark_sync_point_registered( std::wstring const &label );

   bool const register_sync_point( std::wstring const &label );

   bool const register_sync_point( std::wstring const &label, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool const register_all_sync_points( std::string const &list_name );

   bool const register_all_sync_points( std::string const &list_name, RTI1516_NAMESPACE::FederateHandleSet const &handle_set );

   bool const is_sync_point_announced( std::wstring const &label );

   bool const mark_sync_point_announced( std::wstring const &label, RTI1516_USERDATA const &user_supplied_tag );

   bool const wait_for_sync_point_announced( std::wstring const &label );

   bool const wait_for_all_sync_points_announced( std::string const &list_name );

   bool const is_sync_point_achieved( std::wstring const &label );

   bool const achieve_sync_point( std::wstring const &label );

   bool const achieve_all_sync_points( std::string const &list_name );

   bool const is_sync_point_synchronized( std::wstring const &label );

   bool const mark_sync_point_synchronized( std::wstring const &label );

   bool const wait_for_sync_point_synchronized( std::wstring const &label );

   bool const wait_for_all_sync_points_synchronized( std::string const &list_name );

   bool const achieve_sync_point_and_wait_for_synchronization( std::wstring const &label );

   std::string to_string();

   std::string to_string( std::wstring const &label );

   void print_sync_points();

   // Callbacks from FedAmb.
   virtual void sync_point_registration_succeeded( std::wstring const &label );

   virtual void sync_point_registration_failed( std::wstring const &label, RTI1516_NAMESPACE::SynchronizationPointFailureReason reason );

   virtual void sync_point_announced( std::wstring const &label, RTI1516_USERDATA const &user_supplied_tag );

   virtual void sync_point_federation_synchronized( std::wstring const &label );

   /*! @brief Encode the variables to a form Trick can checkpoint. */
   virtual void encode_checkpoint();

   /*! @brief Decode the state of this class from the Trick checkpoint. */
   virtual void decode_checkpoint();

   /*! @brief Free/release the memory used for the checkpoint data structures. */
   virtual void free_checkpoint();

  protected:
   MutexLock mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

#if SYNC_POINT_LIST_TMM_ARRAY
   SyncPointList **sync_pnt_lists;       ///< @trick_units{--} Array of named sync-point lists.
   int             sync_pnt_lists_count; ///< @trick_units{--} Size of the sync-point lists.
#else
   SyncPointListVector sync_pnt_lists; ///< @trick_units{--} Array of named sync-point lists.
#endif

   Federate *federate; ///< @trick_units{--} Associated TrickHLA Federate.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPointManagerBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPointManagerBase( SyncPointManagerBase const &rhs );
   /*! @brief Assignment operator for SyncPointManagerBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPointManagerBase &operator=( SyncPointManagerBase const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_POINT_MANAGER_BASE_HH */
