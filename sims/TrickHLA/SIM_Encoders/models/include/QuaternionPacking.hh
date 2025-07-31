/*!
@file QuaternionPacking.hh
@ingroup SpaceFOM
@brief This class provides an example class for testing the encoding and
decoding of the SpaceFOM Quaternion data type.

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

@python_module{ArtemisFOM}

@tldh
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../source/SpaceFOM/QuaternionEncoder.cpp}
@trick_link_dependency{models/src/QuaternionPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, April 2025, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_QUAT_PACKING_HH
#define SPACEFOM_QUAT_PACKING_HH

// System include files.
#include <iostream>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/QuaternionEncoder.hh"

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
namespace TrickHLA
{
class Attribute;
class Object;
} // namespace TrickHLA

namespace SpaceFOM
{

class QuaternionPacking : public TrickHLA::Packing
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__QuaternionPacking();

  public:
   SpaceFOM::QuaternionData test_data; ///< @trick_units{--} Test quaternion.

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM QuaternionPacking class. */
   QuaternionPacking();
   /*! @brief Destructor for the SpaceFOM QuaternionPacking class. */
   virtual ~QuaternionPacking();

   // Default data.
   /*! @brief Sets up the attributes for using default values.
    *  @param publishes     Does this federate publish the object.
    *  @param sim_obj_name  Name of SimObject containing this object.
    *  @param packing_name  Name of the data packing object in the SimObject.
    *  @param working_data  Reference to the working data associated with this STC object.
    *  @param mngr_object   TrickHLA::Object associated with this STC object.
    *  */
   virtual void base_config( bool                      publishes,
                             char const               *sim_obj_name,
                             char const               *packing_name,
                             SpaceFOM::QuaternionData *working_data = NULL,
                             TrickHLA::Object         *mngr_object = NULL );

   // Pre-initialize the packing object.
   /*! @brief Function to begin the configuration/initialization of the RefFrame.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the RefFrame object is not being configured with an initialization
    *  constructor. */
   void configure();

   // Initialize the packing object.
   /*! @brief Finish the initialization of the RefFrame. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Access function to set the appropriate publish flags. */
   virtual void publish();

   /*! @brief Access function to set the appropriate subscribe flags. */
   virtual void subscribe();

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief A routine to test the unpacked values against the test values. */
   virtual void unpack_test();

   /*! @brief Print out the data values.
    *  @param stream Output stream. */
   virtual void print_data( std::ostream &stream = std::cout ) const;

  public:
   bool debug; ///< @trick_units{--} Debug output flag.
   bool test;  ///< @trick_units{--} Flag to enable testing.

  protected:
   SpaceFOM::QuaternionData * working_data; ///< @trick_units{--} Working data.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute * quat_attr; ///< @trick_io{**} Quaternion Attribute.

   // Assign to these parameters when setting up the data associations for the
   // SpaceFOM TrickHLAObject data for the Quaternion.
   SpaceFOM::QuaternionData packing_data; ///< @trick_units{--} STC packing data.

   // Instantiate the Space/Time Coordinate encoder
   SpaceFOM::QuaternionEncoder quat_encoder; ///< @trick_units{--} Encoder.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for QuaternionPacking class.
    *  @details This constructor is private to prevent inadvertent copies. */
   QuaternionPacking( QuaternionPacking const &rhs );
   /*! @brief Assignment operator for QuaternionPacking class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   QuaternionPacking &operator=( QuaternionPacking const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_QUAT_PACKING_HH: Do NOT put anything after this line!
