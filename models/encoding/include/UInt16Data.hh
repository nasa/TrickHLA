/*!
@ingroup encoding
@file models/encoding/include/UInt16Data.hh
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
@trick_link_dependency{encoding/src/UInt16Data.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, August 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_UINT16_DATA_HH
#define TRICKHLA_MODEL_UINT16_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

class UInt16Data
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__UInt16Data();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel UInt16Data class. */
   UInt16Data();

   explicit UInt16Data( unsigned short const offset );

   /*! @brief Destructor for the TrickHLAModel UInt16Data class. */
   virtual ~UInt16Data();

   bool compare( UInt16Data const &data, std::string &explanation );

   std::string to_string();

  public:
   unsigned short  ui16;
   unsigned short  vec3_ui16[3];
   unsigned short  m3x3_ui16[3][3];
   unsigned short *ptr_ui16;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_UINT16_DATA_HH: Do NOT put anything after this line!
