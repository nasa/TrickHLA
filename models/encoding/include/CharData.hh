/*!
@ingroup encoding
@file models/encoding/include/CharData.hh
@brief This is a container class for general encoder test data.

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

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{encoding/src/CharData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_CHAR_DATA_HH
#define TRICKHLA_MODEL_CHAR_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

class CharData
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__CharData();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel CharData class. */
   CharData();

   explicit CharData( int const offset );

   /*! @brief Destructor for the TrickHLAModel CharData class. */
   virtual ~CharData();

   bool compare( CharData &data );

   std::string to_string();

  public:
   char  _char;
   char  vec3_char[3];
   char  m3x3_char[3][3];
   char *ptr_char;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_CHAR_DATA_HH: Do NOT put anything after this line!
