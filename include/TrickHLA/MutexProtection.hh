/*!
@file TrickHLA/MutexProtection.hh
@ingroup TrickHLA
@brief Mutex protection, automatically unlocks mutex when this object goes out of scope.

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
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end
*/

#ifndef TRICKHLA_MUTEX_PROTECTION_HH
#define TRICKHLA_MUTEX_PROTECTION_HH

namespace TrickHLA
{
class MutexLock;
} /* namespace TrickHLA */

namespace TrickHLA
{

class MutexProtection
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__MutexProtection();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA MutexProtection class. */
   explicit MutexProtection( TrickHLA::MutexLock *mutex_lock );
   /*! @brief Destructor for the TrickHLA MutexProtection class. */
   virtual ~MutexProtection();

   MutexLock *mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for MutexProtection class.
    *  @details This constructor is private to prevent inadvertent copies. */
   MutexProtection( MutexProtection const &rhs );
   /*! @brief Assignment operator for MutexProtection class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   MutexProtection &operator=( MutexProtection const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_MUTEX_PROTECTION_HH: Do NOT put anything after this line!
