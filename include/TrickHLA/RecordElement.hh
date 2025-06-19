/*!
@file TrickHLA/RecordElement.hh
@ingroup TrickHLA
@brief This class represents the record element implementation.

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
@trick_link_dependency{../../source/TrickHLA/RecordElement.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/encoding/EncoderBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_RECORD_ELEMENT_HH
#define TRICKHLA_RECORD_ELEMENT_HH

// System includes.
#include <string>

// Trick includes.
#include <trick/attributes.h>

// TrickHLA includes.
#include "StandardsSupport.hh"
#include "Types.hh"
#include "encoding/EncoderBase.hh"

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

class RecordElement
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__RecordElement();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA RecordElement class. */
   RecordElement();

   /*! @brief Destructor for the TrickHLA RecordElement class. */
   virtual ~RecordElement();

   void initialize_element_encoder();

   void initialize_element_encoder( void *address, ATTRIBUTES *attr );

   void update_before_encode();

   void update_after_decode();

   virtual std::string to_string();

  public:
   char *trick_name; ///< @trick_units{--} Trick variable name for the element.
   char *FOM_name;   ///< @trick_units{--} FOM name for the element.

   EncodingEnum rti_encoding; ///< @trick_units{--} RTI encoding of the data.

   int            element_count; ///< @trick_units{--} Number of fixed record elements.
   RecordElement *elements;      ///< @trick_units{--} Array of fixed record elements.

  protected:
   EncoderBase *encoder; ///< @trick_io{**} The HLA data encoder for this attribute data.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for RecordElement class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RecordElement( RecordElement const &rhs );
   /*! @brief Assignment operator for RecordElement class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RecordElement &operator=( RecordElement const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_RECORD_ELEMENT_HH
