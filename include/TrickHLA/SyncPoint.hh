/*!
@file TrickHLA/SyncPoint.hh
@ingroup TrickHLA
@brief This class provides a sync-point implementation for storing and
managing TrickHLA synchronization points.

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
@trick_link_dependency{../../source/TrickHLA/SyncPoint.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Adding checkpoint support.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_POINT_HH
#define TRICKHLA_SYNC_POINT_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "CheckpointConversionBase.hh"
#include "StandardsSupport.hh"
#include "Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class SyncPoint : public TrickHLA::CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // is_known - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPoint();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor. */
   SyncPoint();

   /*! @brief Initialization constructor.
    *  @param lbl Synchronization point label. */
   explicit SyncPoint( std::wstring const &lbl );

   /*! @brief Destructor for the TrickHLA SyncPoint class. */
   virtual ~SyncPoint();

   // Functions to check synchronization point state.
   /*! @brief Check if the synchronization point has been created and is_known
    *  in at least on valid state.
    *  @return True if the synchronization point has a valid state. */
   virtual bool const is_valid() const;

   // Functions to check synchronization point state.
   /*! @brief Check if the synchronization point is known.
    *  @return True if the synchronization point is known. */
   virtual bool const is_known() const;

   /*! @brief Check if the synchronization point is registered.
    *  @return True if the synchronization point is registered. */
   virtual bool const is_registered() const;

   /*! @brief Check if the synchronization point is announced.
    *  @return True if the synchronization point is announced. */
   virtual bool const is_announced() const;

   /*! @brief Check if the synchronization point is achieved.
    *  @return True if the synchronization point is achieved. */
   virtual bool const is_achieved() const;

   /*! @brief Check if the synchronization point is synchronized.
    *  @return True if the synchronization point is synchronized. */
   virtual bool const is_synchronized() const;

   /*! @brief Check if the synchronization point has a bad state.
    *  @return True if the synchronization point has a bad state. */
   virtual bool const is_error() const;

   // Accessor functions.
   /*! @brief Get the synchronization point label.
    *  @return The synchronization point label. */
   virtual std::wstring const &get_label() const
   {
      return this->label;
   }

   /*! @brief Get the synchronization point label.
    *  @param lbl The synchronization point label. */
   virtual void set_label( std::wstring const &lbl )
   {
      this->label = lbl;
   }

   /*! @brief Get the synchronization point state.
    *  @return The current state for this synchronization point. */
   virtual SyncPtStateEnum const get_state() const
   {
      return this->state;
   }

   /*! @brief Set the current state of the synchronization point.
    *  @param s Current synchronization point state. */
   virtual void set_state( SyncPtStateEnum const s )
   {
      this->state = s;
   }

   /*! @brief Encode the user supplied tag data.
    *  @return The encoded user supplied tag. */
   virtual RTI1516_NAMESPACE::VariableLengthData const encode_user_supplied_tag();

   /*! @brief Decode the user supplied data.
    *  @param supplied_tag supplied_tag The supplied tag to decode as the user supplied tag. */
   virtual void decode_user_supplied_tag( RTI1516_NAMESPACE::VariableLengthData const &supplied_tag );

   // Utility functions.
   /*! @brief Create a C++ string with the synchronization point label and
    *  current state.
    *  @return A string with the synchronization point label and current state. */
   virtual std::string to_string();

   /*! @brief Encode the variables to a form Trick can checkpoint. */
   virtual void encode_checkpoint();

   /*! @brief Decode the state of this class from the Trick checkpoint. */
   virtual void decode_checkpoint();

   /*! @brief Free/release the memory used for the checkpoint data structures. */
   virtual void free_checkpoint();

  protected:
   std::wstring    label; ///< @trick_io{**} Sync-point name.
   SyncPtStateEnum state; ///< @trick_units{--} Sync-point state.

   RTI1516_NAMESPACE::VariableLengthData user_supplied_tag; ///< @trick_io{**} Sync-point user supplied data.

   char *label_chkpt; ///< @trick_units{--} Trick memory allocated label that is checkpointable.
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_POINT_HH */
