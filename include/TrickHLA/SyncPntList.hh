/*!
@file TrickHLA/SyncPntList.hh
@ingroup TrickHLA
@brief This class extends the TrickHLA::SyncPntListBase class and provides an
instantiable implementation for storing and managing HLA synchronization points
for TrickHLA.

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
@trick_link_dependency{../source/TrickHLA/SyncPntListBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_SYNC_PNT_LIST_HH
#define TRICKHLA_SYNC_PNT_LIST_HH

// System includes.
#include <string>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPntListBase.hh"

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

class SyncPntList : public TrickHLA::SyncPntListBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SyncPntList();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SyncPntList class. */
   SyncPntList()
   {
      return;
   }

   /*! @brief Pure virtual destructor for the TrickHLA SyncPntList class. */
   virtual ~SyncPntList()
   {
      return;
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SyncPntList class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SyncPntList( SyncPntList const &rhs );
   /*! @brief Assignment operator for SyncPntList class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SyncPntList &operator=( SyncPntList const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_SYNC_PNT_LIST_HH */
