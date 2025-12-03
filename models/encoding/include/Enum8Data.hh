/*!
@ingroup encoding
@file models/encoding/include/Enum8Data.hh
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
@trick_link_dependency{encoding/src/Enum8Data.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, Dec 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_ENUM8_DATA_HH
#define TRICKHLA_MODEL_ENUM8_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

enum Int8Enum : char { one8,
                       two8 };

class Enum8Data
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__Enum8Data();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel Enum8Data class. */
   Enum8Data();

   explicit Enum8Data( unsigned int const offset );

   /*! @brief Destructor for the TrickHLAModel Enum8Data class. */
   virtual ~Enum8Data();

   bool compare( Enum8Data const &data, std::string &explanation );

   std::string to_string();

  public:
   Int8Enum  enum8;
   Int8Enum  vec3_enum8[3];
   Int8Enum  m3x3_enum8[3][3];
   Int8Enum *ptr_enum8;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_ENUM8_DATA_HH: Do NOT put anything after this line!
