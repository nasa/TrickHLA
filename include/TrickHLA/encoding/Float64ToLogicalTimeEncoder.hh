/*!
@file TrickHLA/encoding/Float64ToLogicalTimeEncoder.hh
@ingroup TrickHLA
@brief This class represents the char array ASCII string encoder implementation.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/Float64ToLogicalTimeEncoder.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/VariableArrayEncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_FLOAT64_TO_LOGICAL_TIME_ENCODER_HH
#define TRICKHLA_FLOAT64_TO_LOGICAL_TIME_ENCODER_HH

// System includes.
#include <cstddef>
#include <string>

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/BasicDataVariableArrayEncoders.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

namespace TrickHLA
{

class Float64ToLogicalTimeEncoder : public VariableArrayEncoderBase
{
   /* Let the Trick input processor access protected and private data. */
   /* InputProcessor is really just a marker class (does not really    */
   /* exists - at least yet). This friend statement just tells Trick   */
   /* to go ahead and process the protected and private data as well   */
   /* as the usual public data.                                        */
   friend class InputProcessor;
   /* IMPORTANT Note: you must have the following line too.            */
   /* Syntax: friend void init_attr<namespace>__<class name>();        */
   friend void init_attrTrickHLA__Float64ToLogicalTimeEncoder();

  public:
   /*! @brief Default constructor. */
   Float64ToLogicalTimeEncoder( void *addr, ATTRIBUTES *attr );

   /*! @brief Destructor for the TrickHLA Float64ToLogicalTimeEncoder class. */
   virtual ~Float64ToLogicalTimeEncoder();

   virtual void update_before_encode();

   virtual void update_after_decode();

  protected:
   long long time_data; ///< @trick_io{--} Time in base time units.

  private:
   /* Do not allow the default, copy constructor or assignment operator. */
   Float64ToLogicalTimeEncoder();
   /*! @brief Copy constructor for Float64ToLogicalTimeEncoder class.           */
   /*  @details This constructor is private to prevent inadvertent copies. */
   Float64ToLogicalTimeEncoder( Float64ToLogicalTimeEncoder const &rhs );
   /*! @brief Assignment operator for Float64ToLogicalTimeEncoder class.           */
   /*  @details Assignment operator is private to prevent inadvertent copies. */
   Float64ToLogicalTimeEncoder &operator=( Float64ToLogicalTimeEncoder const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_FLOAT64_TO_LOGICAL_TIME_ENCODER_HH
