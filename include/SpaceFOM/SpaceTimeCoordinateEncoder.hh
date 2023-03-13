/*!
@file SpaceFOM/SpaceTimeCoordinateEncoder.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM Space/Time coordinate encoder.

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
v@trick_link_dependency{../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_SPACE_TIME_COORDINATE_ENCODER_HH
#define SPACEFOM_SPACE_TIME_COORDINATE_ENCODER_HH

// TrickHLA include files.
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/StandardsSupport.hh"

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

// SpaceFOM include files.
#include "SpaceFOM/SpaceTimeCoordinateData.h"

namespace SpaceFOM
{

class SpaceTimeCoordinateEncoder : public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__SpaceTimeCoordinateEncoder();

  public:
   /*! @brief Default constructor for the SpaceFOM SpaceTimeCoordinateEncoder class. */
   SpaceTimeCoordinateEncoder();

   /*! @brief Encode the spacetime coordinate data for sending out. */
   void encode();
   /*! @brief Decode the incoming spacetime coordinate data. */
   void decode();

   /*! @brief Get the spacetime coordinate data.
    *  @return A reference to the SpaceTimeCoordinateData. */
   SpaceTimeCoordinateData &get_data()
   {
      return data;
   }

  protected:
   SpaceTimeCoordinateData data; ///< trick_units{--} Reference frame transmission data.

   // Data encoding helpers.
   rti1516e::HLAfloat64LE   position[3];         ///< @trick_io{**} HLAfloat64LE position array
   rti1516e::HLAfixedArray  position_encoder;    ///< @trick_io{**} Position encoder
   rti1516e::HLAfloat64LE   velocity[3];         ///< @trick_io{**} HLAfloat64LE velocity array
   rti1516e::HLAfixedArray  velocity_encoder;    ///< @trick_io{**} Velocity encoder
   rti1516e::HLAfixedRecord trans_state_encoder; ///< @trick_io{**} Translational state encoder

   rti1516e::HLAfloat64LE   quat_scalar_encoder; ///< @trick_io{**} Quaternion scalar encoder
   rti1516e::HLAfloat64LE   quat_vector[3];      ///< @trick_io{**} HLAfloat64LE quaternion vector
   rti1516e::HLAfixedArray  quat_vector_encoder; ///< @trick_io{**} Quaternion vector encoder
   rti1516e::HLAfixedRecord quat_encoder;        ///< @trick_io{**} Attitude quaternion encoder
   rti1516e::HLAfloat64LE   angular_velocity[3]; ///< @trick_io{**} HLAfloat64LE angular velocity array.
   rti1516e::HLAfixedArray  ang_vel_encoder;     ///< @trick_io{**} Angular velocity encoder
   rti1516e::HLAfixedRecord rot_state_encoder;   ///< @trick_io{**} Rotational state encoder

   rti1516e::HLAfloat64LE time_encoder; ///< @trick_io{**} Time encoder

   rti1516e::HLAfixedRecord encoder; ///< @trick_io{**} Space/Time coordinate encoder

  private:
   // This object is not copyable
   /*! @brief Copy constructor for SpaceTimeCoordinateEncoder class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SpaceTimeCoordinateEncoder( SpaceTimeCoordinateEncoder const &rhs );
   /*! @brief Assignment operator for SpaceTimeCoordinateEncoder class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SpaceTimeCoordinateEncoder &operator=( SpaceTimeCoordinateEncoder &rhs );
};

} // namespace SpaceFOM

#endif // End of SPACEFOM_SPACE_TIME_COORDINATE_ENCODER_HH: Do NOT put anything after this line!
