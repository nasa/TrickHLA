/*!
@ingroup encoding
@file models/encoding/include/Enum64Data.hh
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
@trick_link_dependency{encoding/src/Enum64Data.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, Dec 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_ENUM64_DATA_HH
#define TRICKHLA_MODEL_ENUM64_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

enum Int64Enum : long long { one64,
                             two64 };

class Enum64Data
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__Enum64Data();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel Enum64Data class. */
   Enum64Data();

   explicit Enum64Data( unsigned int const offset );

   /*! @brief Destructor for the TrickHLAModel Enum64Data class. */
   virtual ~Enum64Data();

   bool compare( Enum64Data const &data, std::string &explanation );

   std::string to_string();

  public:
   Int64Enum  enum64;
   Int64Enum  vec3_enum64[3];
   Int64Enum  m3x3_enum64[3][3];
   Int64Enum *ptr_enum64;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_ENUM64_DATA_HH: Do NOT put anything after this line!
