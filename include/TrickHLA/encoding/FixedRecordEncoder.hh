/*!
@file TrickHLA/encoding/FixedRecordEncoder.hh
@ingroup TrickHLA
@brief This class represents the fixed record encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/FixedRecordEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_FIXED_RECORD_ENCODER_HH
#define TRICKHLA_FIXED_RECORD_ENCODER_HH

// TrickHLA includes.
#include "../StandardsSupport.hh"
#include "EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAfixedRecord.h"
#pragma GCC diagnostic pop

namespace RTI1516_NAMESPACE
{
class HLAfixedRecord;
} // namespace RTI1516_NAMESPACE

namespace TrickHLA
{

class FixedRecordEncoder : public EncoderBase
{
   /* Let the Trick input processor access protected and private data. */
   /* InputProcessor is really just a marker class (does not really    */
   /* exists - at least yet). This friend statement just tells Trick   */
   /* to go ahead and process the protected and private data as well   */
   /* as the usual public data.                                        */
   friend class InputProcessor;
   /* IMPORTANT Note: you must have the following line too.            */
   /* Syntax: friend void init_attr<namespace>__<class name>();        */
   friend void init_attrTrickHLA__FixedRecordEncoder();

  public:
   /*! @brief Default constructor. */
   FixedRecordEncoder();

   /*! @brief Destructor for the TrickHLA FixedRecordEncoder class. */
   virtual ~FixedRecordEncoder();

   virtual void update_before_encode();

   virtual void update_after_decode();

   void update_before_encode( RTI1516_NAMESPACE::HLAfixedRecord *fixed_rec );

   void update_after_decode( RTI1516_NAMESPACE::HLAfixedRecord *fixed_rec );

  private:
   /* Do not allow the default, copy constructor or assignment operator. */
   /*! @brief Copy constructor for FixedRecordEncoder class.           */
   /*  @details This constructor is private to prevent inadvertent copies. */
   FixedRecordEncoder( FixedRecordEncoder const &rhs );
   /*! @brief Assignment operator for FixedRecordEncoder class.           */
   /*  @details Assignment operator is private to prevent inadvertent copies. */
   FixedRecordEncoder &operator=( FixedRecordEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_FIXED_RECORD_ENCODER_HH
