/*!
@file TrickHLA/Int64FixedArrayEncoder.hh
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/Int64FixedArrayEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_INT64_FIXED_ARRAY_ENCODER_HH
#define TRICKHLA_INT64_FIXED_ARRAY_ENCODER_HH

// System includes.
#include <cstdint>
#include <map>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/reference.h"

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

class Int64FixedArrayEncoder : public EncoderBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Int64FixedArrayEncoder();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Int64FixedArrayEncoder class. */
   Int64FixedArrayEncoder( std::string const &trick_variable_name,
                           std::string const &fom_variable_name,
                           EncodingEnum       hla_encoding,
                           REF2              *r2 );

   /*! @brief Destructor for the TrickHLA Int64FixedArrayEncoder class. */
   virtual ~Int64FixedArrayEncoder();

  protected:
   /*! @brief Initializes the TrickHLA Int64FixedArrayEncoder. */
   virtual void initialize();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Int64FixedArrayEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Int64FixedArrayEncoder( Int64FixedArrayEncoder const &rhs );
   /*! @brief Assignment operator for Int64FixedArrayEncoder class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Int64FixedArrayEncoder &operator=( Int64FixedArrayEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_INT64_FIXED_ARRAY_ENCODER_HH
