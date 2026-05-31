/*!
@file TrickHLA/KnownFederate.hh
@ingroup TrickHLA
@brief A class representing an HLA federate known to the Federation.

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
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, April 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_KNOWN_FEDERATE_HH
#define TRICKHLA_KNOWN_FEDERATE_HH

// System includes.
#include <cstdint>
#include <string>

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/SaveRestoreServices.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include <RTI/Typedefs.h>

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

/*
 * Enumerated type used to step through the Save process.
 */
enum class THLAFederateUpdateProcessEnum : uint8_t {
   FEDERATE_UPDATE_FIRST       = 0,
   FEDERATE_UPDATE_NONE        = 0,
   FEDERATE_UPDATE_ACTIVATE    = 1,
   FEDERATE_UPDATE_INITIATED   = 2,
   FEDERATE_UPDATE_RECEIVED    = 3,
   FEDERATE_UPDATE_IN_PROGRESS = 4,
   FEDERATE_UPDATE_COMPLETE    = 5,
   FEDERATE_UPDATE_FAILED      = 6,
   FEDERATE_UPDATE_LAST        = 6
};

/*! @brief Convert a THLAFederateUpdateProcessEnum value into a string
 *  representation.
 *  @param update_state Value to convert to a string. */
std::string to_string( THLAFederateUpdateProcessEnum update_state );

class KnownFederate
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__KnownFederate();

   // Allow the TrickHLA core classes to have direct access to protected
   // and private data.
   friend class Federate;
   friend class SaveRestoreServices;

  public:
   //----------------------------- USER VARIABLES -----------------------------
   // Variables below this point are for either use within a users simulation
   // or must be configured by the user.

   std::wstring name;     ///< @trick_units{--} Name of a Federate in the Federation.
   std::wstring type;     ///< @trick_units{--} Type of a Federate in the Federation.
   bool         required; ///< @trick_units{--} True requires federate to be in federation before continuing.

  public:
   /*! @brief Default constructor for the TrickHLA KnownFederate class. */
   KnownFederate()
      : name(),
        type(),
        required( false ),
        object_instance_handle(),
        MOM_instance_name()
   {
      return;
   };
   /*! @brief Destructor for the TrickHLA KnownFederate class. */
   virtual ~KnownFederate()
   {
      return;
   };

   /*! @brief Check if the known federate MOM data is completely resolved.
    *  @return True if all items have been resolved; otherwise, false. */
   bool is_complete() const
   {
      if ( name.empty()
           || type.empty()
           || MOM_instance_name.empty()
           || !federate_handle.isValid()
           || !object_instance_handle.isValid() ) {
         return false;
      }

      return ( true );
   }

   /*! @brief Get a copy of the federate MOM instance name.
    *  @return Wide string with the federate MOM instance name. */
   std::wstring const &get_MOM_instance_name()
   {
      return ( MOM_instance_name );
   }

   /*! @brief Get a copy of the federate MOM instance handle.
    *  @return Copy of the federate MOM instance handle. */
   RTI1516_NAMESPACE::ObjectInstanceHandle get_object_instance_handle()
   {
      return ( object_instance_handle );
   }

  protected:
   RTI1516_NAMESPACE::FederateHandle       federate_handle;        ///< @trick_io{**}    HLA Federate handle.
   RTI1516_NAMESPACE::ObjectInstanceHandle object_instance_handle; ///< @trick_io{**}    HLA Federate object instance handle.
   std::wstring                            MOM_instance_name;      ///< @trick_units{--} MOM instance name for the federate object.
};

typedef std::map< RTI1516_NAMESPACE::ObjectInstanceHandle, KnownFederate > KnownFederateMap;
typedef std::set< RTI1516_NAMESPACE::ObjectInstanceHandle >                FederateObjectInstanceSet;
typedef std::vector< KnownFederate >                                       KnownFederateVector;

} // namespace TrickHLA

#ifdef SWIG
   %template(KnownFederateVector) std::vector< TrickHLA::KnownFederate >;
#endif

#endif // TRICKHLA_KNOWN_FEDERATE_HH
