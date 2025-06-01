/*!
@file TrickHLA/encoding/BasicDataEncoders.hh
@ingroup TrickHLA
@brief This class represents the basic data encoder implementation.

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

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/BasicDataEncoders.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_DATA_ENCODERS_HH
#define TRICKHLA_BASIC_DATA_ENCODERS_HH

// System includes.
#include <cstddef>
#include <string>

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

namespace TrickHLA
{

#define DEFINE_BASIC_ENCODER_CLASS( EncoderClassName )                              \
                                                                                    \
   class EncoderClassName : public EncoderBase                                      \
   {                                                                                \
      /* Let the Trick input processor access protected and private data.  */       \
      /* InputProcessor is really just a marker class (does not really     */       \
      /* exists - at least yet). This friend statement just tells Trick    */       \
      /* to go ahead and process the protected and private data as well    */       \
      /* as the usual public data.                                         */       \
      friend class InputProcessor;                                                  \
      /* IMPORTANT Note: you must have the following line too.             */       \
      /* Syntax: friend void init_attr<namespace>__<class name>();         */       \
      friend void init_attrTrickHLA__EncoderClassName();                            \
                                                                                    \
     public:                                                                        \
      EncoderClassName( void       *var_address,                                    \
                        ATTRIBUTES *var_attr );                                     \
                                                                                    \
      virtual ~EncoderClassName();                                                  \
                                                                                    \
      virtual std::string to_string();                                              \
                                                                                    \
     private:                                                                       \
      /* Do not allow the default, copy constructor or assignment operator. */      \
      EncoderClassName();                                                           \
      /*! @brief Copy constructor for EncoderClassName class.                 */    \
      /*  @details This constructor is private to prevent inadvertent copies. */    \
      EncoderClassName( EncoderClassName const &rhs );                              \
      /*! @brief Assignment operator for EncoderClassName class.                 */ \
      /*  @details Assignment operator is private to prevent inadvertent copies. */ \
      EncoderClassName &operator=( EncoderClassName const &rhs );                   \
   };

DEFINE_BASIC_ENCODER_CLASS( ASCIICharEncoder )
DEFINE_BASIC_ENCODER_CLASS( ASCIIStringEncoder )
DEFINE_BASIC_ENCODER_CLASS( BoolEncoder )
DEFINE_BASIC_ENCODER_CLASS( ByteEncoder )
DEFINE_BASIC_ENCODER_CLASS( Float32BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Float32LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Float64BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Float64LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int16BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int16LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int32BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int32LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int64BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( Int64LEEncoder )

#if defined( IEEE_1516_2025 )
DEFINE_BASIC_ENCODER_CLASS( UInt16BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( UInt16LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( UInt32BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( UInt32LEEncoder )
DEFINE_BASIC_ENCODER_CLASS( UInt64BEEncoder )
DEFINE_BASIC_ENCODER_CLASS( UInt64LEEncoder )
#endif // IEEE_1516_2025

DEFINE_BASIC_ENCODER_CLASS( UnicodeCharEncoder )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DEFINE_BASIC_ENCODER_CLASS( UnicodeStringEncoder )
#endif // TRICK_WSTRING_MM_SUPPORT

} // namespace TrickHLA

#endif // TRICKHLA_BASIC_DATA_ENCODERS_HH
