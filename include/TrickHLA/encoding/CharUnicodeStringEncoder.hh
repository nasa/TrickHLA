/*!
@file TrickHLA/encoding/CharUnicodeStringEncoder.hh
@ingroup TrickHLA
@brief This class represents the char array Unicode string encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/CharUnicodeStringEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_CHAR_UNICODE_STRING_ENCODER_HH
#define TRICKHLA_CHAR_UNICODE_STRING_ENCODER_HH

// System includes.
#include <cstddef>
#include <string>

// Trick include files.
#include "trick/reference.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

namespace TrickHLA
{

class CharUnicodeStringEncoder : public EncoderBase
{
   /* Let the Trick input processor access protected and private data. */
   /* InputProcessor is really just a marker class (does not really    */
   /* exists - at least yet). This friend statement just tells Trick   */
   /* to go ahead and process the protected and private data as well   */
   /* as the usual public data.                                        */
   friend class InputProcessor;
   /* IMPORTANT Note: you must have the following line too.            */
   /* Syntax: friend void init_attr<namespace>__<class name>();        */
   friend void init_attrTrickHLA__CharUnicodeStringVariableArrayEncoder();

  public:
   /*! @brief Default constructor. */
   CharUnicodeStringEncoder( std::string const &trick_variable_name,
                             REF2              *r2 );

   /*! @brief Destructor for the TrickHLA CharUnicodeStringEncoder class. */
   virtual ~CharUnicodeStringEncoder();

   virtual RTI1516_NAMESPACE::VariableLengthData &encode();

   virtual bool const decode( RTI1516_NAMESPACE::VariableLengthData const &encoded_data );

   virtual std::string to_string();

  private:
   /* Do not allow the default, copy constructor or assignment operator. */
   CharUnicodeStringEncoder();
   /*! @brief Copy constructor for CharUnicodeStringEncoder class.         */
   /*  @details This constructor is private to prevent inadvertent copies. */
   CharUnicodeStringEncoder( CharUnicodeStringEncoder const &rhs );
   /*! @brief Assignment operator for CharUnicodeStringEncoder class.         */
   /*  @details Assignment operator is private to prevent inadvertent copies. */
   CharUnicodeStringEncoder &operator=( CharUnicodeStringEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_CHAR_UNICODE_STRING_ENCODER_HH
