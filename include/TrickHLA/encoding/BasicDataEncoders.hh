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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_BASIC_DATA_ENCODERS_HH
#define TRICKHLA_BASIC_DATA_ENCODERS_HH

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

namespace TrickHLA
{

#define DEFINE_BASIC_ENCODER_CLASS( EncoderClassName, EncodableDataType )           \
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
      explicit EncoderClassName( void *addr );                                      \
                                                                                    \
      virtual ~EncoderClassName();                                                  \
                                                                                    \
      virtual void update_before_encode()                                           \
      {                                                                             \
         return;                                                                    \
      }                                                                             \
                                                                                    \
      virtual void update_after_decode()                                            \
      {                                                                             \
         return;                                                                    \
      }                                                                             \
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

DEFINE_BASIC_ENCODER_CLASS( ASCIICharEncoder, HLAASCIIchar )
DEFINE_BASIC_ENCODER_CLASS( ASCIIStringEncoder, HLAASCIIstring )
DEFINE_BASIC_ENCODER_CLASS( BoolEncoder, HLAboolean )
DEFINE_BASIC_ENCODER_CLASS( ByteEncoder, HLAbyte )
DEFINE_BASIC_ENCODER_CLASS( Float32BEEncoder, HLAfloat32BE )
DEFINE_BASIC_ENCODER_CLASS( Float32LEEncoder, HLAfloat32LE )
DEFINE_BASIC_ENCODER_CLASS( Float64BEEncoder, HLAfloat64BE )
DEFINE_BASIC_ENCODER_CLASS( Float64LEEncoder, HLAfloat64LE )
DEFINE_BASIC_ENCODER_CLASS( Int16BEEncoder, HLAinteger16BE )
DEFINE_BASIC_ENCODER_CLASS( Int16LEEncoder, HLAinteger16LE )
DEFINE_BASIC_ENCODER_CLASS( Int32BEEncoder, HLAinteger32BE )
DEFINE_BASIC_ENCODER_CLASS( Int32LEEncoder, HLAinteger32LE )
DEFINE_BASIC_ENCODER_CLASS( Int64BEEncoder, HLAinteger64BE )
DEFINE_BASIC_ENCODER_CLASS( Int64LEEncoder, HLAinteger64LE )

#if defined( IEEE_1516_2025 )
DEFINE_BASIC_ENCODER_CLASS( UInt16BEEncoder, HLAunsignedInteger16BE )
DEFINE_BASIC_ENCODER_CLASS( UInt16LEEncoder, HLAunsignedInteger16LE )
DEFINE_BASIC_ENCODER_CLASS( UInt32BEEncoder, HLAunsignedInteger32BE )
DEFINE_BASIC_ENCODER_CLASS( UInt32LEEncoder, HLAunsignedInteger32LE )
DEFINE_BASIC_ENCODER_CLASS( UInt64BEEncoder, HLAunsignedInteger64BE )
DEFINE_BASIC_ENCODER_CLASS( UInt64LEEncoder, HLAunsignedInteger64LE )
#endif // IEEE_1516_2025

DEFINE_BASIC_ENCODER_CLASS( UnicodeCharEncoder, HLAunicodeChar )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DEFINE_BASIC_ENCODER_CLASS( UnicodeStringEncoder, HLAunicodeString )
#endif // TRICK_WSTRING_MM_SUPPORT

} // namespace TrickHLA

#endif // TRICKHLA_BASIC_DATA_ENCODERS_HH
