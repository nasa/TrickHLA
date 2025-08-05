/*!
@file TrickHLA/encoding/CharOpaqueDataEncoder.hh
@ingroup TrickHLA
@brief This class represents the char array Opaque data encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/CharOpaqueDataEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/VariableArrayEncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_CHAR_OPAQUE_DATA_ENCODER_HH
#define TRICKHLA_CHAR_OPAQUE_DATA_ENCODER_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/attributes.h"

// TrickHLA includes.
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAopaqueData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class CharOpaqueDataEncoder : public VariableArrayEncoderBase
{
   /* Let the Trick input processor access protected and private data. */
   /* InputProcessor is really just a marker class (does not really    */
   /* exists - at least yet). This friend statement just tells Trick   */
   /* to go ahead and process the protected and private data as well   */
   /* as the usual public data.                                        */
   friend class InputProcessor;
   /* IMPORTANT Note: you must have the following line too.            */
   /* Syntax: friend void init_attr<namespace>__<class name>();        */
   friend void init_attrTrickHLA__CharOpaqueDataEncoder();

  public:
   /*! @brief Default constructor. */
   CharOpaqueDataEncoder( void              *addr,
                          ATTRIBUTES        *attr,
                          std::string const &name );

   /*! @brief Destructor for the TrickHLA CharOpaqueDataEncoder class. */
   virtual ~CharOpaqueDataEncoder();

   virtual void update_before_encode();

   virtual void update_after_decode();

   virtual int const get_data_size()
   {
      return ( ( data_encoder != NULL )
                  ? dynamic_cast< RTI1516_NAMESPACE::HLAopaqueData * >( data_encoder )->dataLength()
                  : 0 );
   }

   virtual std::string to_string()
   {
      return "CharOpaqueDataEncoder[" + data_name + "]";
   }

  private:
   /* Do not allow the default, copy constructor or assignment operator. */
   CharOpaqueDataEncoder();
   /*! @brief Copy constructor for CharOpaqueDataEncoder class.            */
   /*  @details This constructor is private to prevent inadvertent copies. */
   CharOpaqueDataEncoder( CharOpaqueDataEncoder const &rhs );
   /*! @brief Assignment operator for CharOpaqueDataEncoder class.            */
   /*  @details Assignment operator is private to prevent inadvertent copies. */
   CharOpaqueDataEncoder &operator=( CharOpaqueDataEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_CHAR_OPAQUE_DATA_ENCODER_HH
