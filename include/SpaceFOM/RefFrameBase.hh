/*!
@file SpaceFOM/RefFrameBase.hh
@ingroup SpaceFOM
@brief This class provides an extendable base class for SpaceFOM Reference Frames packing.

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
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/SpaceFOM/LRTreeNodeBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/ExecutionControl.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_BASE_HH
#define SPACEFOM_REF_FRAME_BASE_HH

// System include files.
#include <iostream>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/LRTreeNodeBase.hh"
#include "SpaceFOM/RefFrameData.hh"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

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

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class ExecutionControl;

class RefFrameBase : public TrickHLA::Packing, public SpaceFOM::LRTreeNodeBase
{

   friend class RefFrameTree;

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameBase();

   // Make the Conditional class a friend.
   friend class RefFrameConditionalBase;

   // Make the Lag Compensation class a friend.
   friend class RefFrameLagCompBase;

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM RefFrameBase class. */
   RefFrameBase();
   /*! @brief Destructor for the SpaceFOM RefFrameBase class. */
   virtual ~RefFrameBase();

   // Default data.
   /*! @brief Sets up the attributes for a reference frame using default values.
    *  @param publishes             Does this federate publish this reference frame.
    *  @param sim_obj_name          Name of SimObject containing this reference frame.
    *  @param ref_frame_obj_name    Name of the ReferenceFrame object in the SimObject.
    *  @param ref_frame_name        Name of the ReferenceFrame instance.
    *  @param ref_frame_parent_name Name of the parent frame for this ReferenceFrame instance.
    *  @param ref_frame_parent      Reference to parent frame for this ReferenceFrame instance.
    *  @param mngr_object           TrickHLA::Object associated with this reference frame.
    *  */
   virtual void base_config( bool              publishes,
                             char const       *sim_obj_name,
                             char const       *ref_frame_obj_name,
                             char const       *ref_frame_name,
                             char const       *ref_frame_parent_name = NULL,
                             RefFrameBase     *ref_frame_parent      = NULL,
                             TrickHLA::Object *mngr_object           = NULL );

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

   // Access functions.
   /*! @brief Access function to set the HLA federation instance name for the reference frame.
    *  @param new_name Object instance name for this reference frame. */
   virtual void set_name( char const *new_name );

   /*! @brief Access function to get the HLA federation instance name for the reference frame.
    *  @return Object instance name for this reference frame. */
   virtual char const *get_name() const
   {
      return packing_data.name;
   }

   /*! @brief Access function to set the HLA federation instance name for the parent reference frame.
    *  @param name Object instance name for the parent reference frame. */
   virtual void set_parent_name( char const *name );

   /*! @brief Access function to get the HLA federation instance name for the parent reference frame.
    *  @return Object instance name for the parent reference frame. */
   virtual char const *get_parent_name() const
   {
      return packing_data.parent_name;
   }

   /*! @brief Access function to set the pointer to the parent reference frame.
    *  @param pframe_ptr Pointer to the parent reference frame. */
   virtual void set_parent_frame( RefFrameBase *pframe_ptr );

   /*! @brief Access function to get the pointer to the parent reference frame.
    *  @return Pointer to the parent reference frame. */
   virtual RefFrameBase const *get_parent_frame() const
   {
      return parent_frame;
   }

   /*! @brief Set this reference frame as the root reference frame.
    *  @return True if set succeeded, false otherwise. */
   virtual bool set_root( bool root_state );

   /*! @brief Get the current scenario time associated with the PhysicalEntity.
    *  @return Current time associated with the PhysicalEntity. */
   double get_time() const
   {
      return packing_data.state.time;
   }

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
   virtual void pack_from_working_data() = 0;

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data() = 0;

   /*! @brief Print out the reference frame data values.
    *  @param stream Output stream. */
   virtual void print_data( std::ostream &stream = std::cout ) const;

  protected:
   RefFrameBase *parent_frame; ///< @trick_units{--} Pointer to this frame's parent frame.

   TrickHLA::Attribute *name_attr;        ///< @trick_io{**} Reference frame name Attribute.
   TrickHLA::Attribute *parent_name_attr; ///< @trick_io{**} Parent reference frame name Attribute.
   TrickHLA::Attribute *state_attr;       ///< @trick_io{**} Reference frame state Attribute.

   // Assign to these parameters when setting up the data associations for the
   // SpaceFOM TrickHLAObject data for the Reference Frame.
   RefFrameData packing_data; ///< @trick_units{--} Reference frame packing data.

   // Instantiate the Space/Time Coordinate encoder
   SpaceTimeCoordinateEncoder stc_encoder; ///< @trick_units{--} Encoder.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RefFrameBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameBase( RefFrameBase const &rhs );
   /*! @brief Assignment operator for RefFrameBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameBase &operator=( RefFrameBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_BASE_HH: Do NOT put anything after this line!
