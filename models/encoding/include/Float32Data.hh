/*!
@ingroup encoding
@file models/encoding/include/Float32Data.hh
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
@trick_link_dependency{encoding/src/Float32Data.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_FLOAT32_DATA_HH
#define TRICKHLA_MODEL_FLOAT32_DATA_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

namespace TrickHLAModel
{

class Float32Data
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__Float32Data();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel Float32Data class. */
   Float32Data();

   explicit Float32Data( int const offset );

   /*! @brief Destructor for the TrickHLAModel Float32Data class. */
   virtual ~Float32Data();

   bool compare( Float32Data &data );

   std::string to_string();

  public:
   float  f32;
   float  vec3_f32[3];
   float  m3x3_f32[3][3];
   float *ptr_f32;
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_FLOAT32_DATA_HH: Do NOT put anything after this line!
