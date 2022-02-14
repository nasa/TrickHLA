/*!
@file SpaceFOM/RefFrameBase.hh
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM Reference Frames.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to the Reference Frame object. This needs to be available
to the SpaceFOM initialization process for the root reference frame
discovery step in the initialization process.

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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _SPACEFOM_REF_FRAME_BASE_HH_
#define _SPACEFOM_REF_FRAME_BASE_HH_

// System include files.

// TrickHLA include files.
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameData.h"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
namespace TrickHLA
{
class Attribute;
class Object;
class Manager;
} // namespace TrickHLA

namespace SpaceFOM
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class ExecutionControl;

class RefFrameBase : public TrickHLA::Packing
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameBase();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM RefFrameBase class. */
   RefFrameBase();
   /*! @brief Destructor for the SpaceFOM RefFrameBase class. */
   virtual ~RefFrameBase();

   // Default data.
   /*! @brief Sets up the attributes for a reference frame using default values.
    *  @param execution_control SpaceFOM::ExecutionControl associated with this federate.
    *  @param object TrickHLA::Object associated with this reference frame.
    *  @param sim_obj_name Name of SimObject containing this reference frame.
    *  @param ref_frame_obj_name Name of the ReferenceFrame object in the SimObject.
    *  @param ref_frame_name Name of the ReferenceFrame instance.
    *  @param publishes Does this federate publish this reference frame.
    *  */
   virtual void default_data( SpaceFOM::ExecutionControl *execution_control,
                              TrickHLA::Object *          object,
                              const char *                sim_obj_name,
                              const char *                ref_frame_obj_name,
                              const char *                ref_frame_name,
                              bool                        publishes );

   // Initialize the packing object.
   /*! @brief Set the reference to the reference frame data.
    *  @param ref_frame_data_ptr Pointer to the RefFrameData instance. */
   void initialize( RefFrameData *ref_frame_data_ptr );

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // Access functions.
   /*! @brief Access function to set the HLA federation instance name for the reference frame.
    *  @param new_name Object instance name for this reference frame. */
   virtual void set_name( const char *new_name );

   /*! @brief Access function to get the HLA federation instance name for the reference frame.
    *  @return Object instance name for this reference frame. */
   virtual const char *get_name()
   {
      return name;
   }

   /*! @brief Access function to set the HLA federation instance name for the parent reference frame.
    *  @param name Object instance name for the parent reference frame. */
   virtual void set_parent_name( const char *name );

   /*! @brief Access function to get the HLA federation instance name for the parent reference frame.
    *  @return Object instance name for the parent reference frame. */
   virtual const char *get_parent_name()
   {
      return parent_name;
   }

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   // Access to protected data.
   virtual TrickHLA::Object *get_object()
   {
      return object;
   }

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   bool                 initialized;    ///< @trick_units{--} Initialization indication flag.
   RefFrameData *       ref_frame_data; ///< @trick_units{--} Reference frame data.
   TrickHLA::Attribute *ref_frame_attr; ///< @trick_io{**} Reference Frame Attribute.

   char *name;        ///< @trick_units{--} Name of the reference frame.
   char *parent_name; ///< @trick_units{--} Name of this frames parent frame.

   // Instantiate the Space/Time Coordinate encoder
   SpaceTimeCoordinateEncoder stc_encoder; ///< @trick_units{--} Encoder.
   SpaceTimeCoordinateData &  stc_data;    ///< @trick_units{--} Encoder data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RefFrameBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameBase( const RefFrameBase & );
   /*! @brief Assignment operator for RefFrameBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameBase &operator=( const RefFrameBase & );

   /*! @brief Uses Trick memory allocation routines to allocate a new string
    *  that is input file compliant. */
   char *allocate_input_string( const char *c_string );
   /*! @brief Uses Trick memory allocation routines to allocate a new string
    *  that is input file compliant. */
   char *allocate_input_string( std::string cpp_string );
};

} // namespace SpaceFOM

#endif // _SPACEFOM_REF_FRAME_BASE_HH_: Do NOT put anything after this line!
