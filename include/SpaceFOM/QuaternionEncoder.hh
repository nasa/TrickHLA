/*!
@file SpaceFOM/QuaternionEncoder.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM quaternion encoding utility.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionData.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_QUATERNION_ENCODER_HH
#define SPACEFOM_QUATERNION_ENCODER_HH

// TrickHLA include files.
#include "TrickHLA/OpaqueBuffer.hh"

// SpaceFOM include files.
#include "SpaceFOM/QuaternionData.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/HLAfixedArray.h"
#include "RTI/encoding/HLAfixedRecord.h"
#pragma GCC diagnostic pop

// Put this class in the SpaceFOM namespace.
namespace SpaceFOM
{

class QuaternionEncoder : public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__QuaternionEncoder();

  public:
   /*! @brief Initialization constructor for the SpaceFOM QuaternionEncoder class.
    *  @param quat_data Reference to quaternion data instance. */
   explicit QuaternionEncoder( QuaternionData &quat_data );

   /*! @brief Encode the quaternion data for sending out. */
   void encode();
   /*! @brief Decode the quaternion space/time coordinate data. */
   void decode();

   /*! @brief Get the quaternion data.
    *  @return A reference to the QuaternionData. */
   QuaternionData &get_data()
   {
      return data;
   }

  protected:
   QuaternionData &data; ///< @trick_units{--} Quaternion transmission data.

   // Data encoding helpers.
   rti1516e::HLAfloat64LE   scalar_encoder; ///< @trick_io{**} Quaternion scalar encoder
   rti1516e::HLAfloat64LE   vector[3];      ///< @trick_io{**} HLAfloat64LE quaternion vector
   rti1516e::HLAfixedArray  vector_encoder; ///< @trick_io{**} Quaternion vector encoder
   rti1516e::HLAfixedRecord encoder;        ///< @trick_io{**} Attitude quaternion encoder

   rti1516e::VariableLengthData encoded_data; ///< @trick_io{**} Variable length encode/decode data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for QuaternionEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   QuaternionEncoder( QuaternionEncoder const &rhs );
   /*! @brief Assignment operator for QuaternionEncoder class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   QuaternionEncoder &operator=( QuaternionEncoder &rhs );
};

} // namespace SpaceFOM

#endif /* SPACEFOM_QUATERNION_ENCODER_HH */
