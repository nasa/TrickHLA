/*!
@file Ball/WallsPacking.hh
@ingroup Ball
@brief Definition of the TrickHLA Walls packing class.

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

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../src/WallsPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, April 2025, --, Initial version.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_WALLS_PACKING_H
#define TRICKHLA_MODEL_WALLS_PACKING_H

// System include files.
#include <iostream>

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// Walls model include files.
#include "Ball/include/ball_walls.h"

namespace TrickHLAModel
{

class WallsPacking : public TrickHLA::Packing, public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__WallsPacking();

   // Make the Conditional class a friend.
   friend class WallsConditionalBase;

  public:
   // Public constructors and destructors.
   WallsPacking();                                     // Default constructor.
   explicit WallsPacking( BallWalls &ball_walls_ref ); // Initialization constructor.
   virtual ~WallsPacking();                            // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a Walls using default values.
    *  @param sim_obj_name Name of SimObject containing this Walls.
    *  @param entity_obj_name Name of the Walls object in the SimObject.
    *  @param entity_name Name of the Walls instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this Walls instance.
    *  @param publishes Does this federate publish this Walls.
    *  @param mngr_object TrickHLA::Object associated with this Walls.
    *  */
   virtual void base_config( char const       *sim_obj_name,
                             char const       *walls_obj_name,
                             char const       *walls_name,
                             bool              publishes,
                             TrickHLA::Object *mngr_object = NULL );

   /*! @brief Function to begin the configuration/initialization of the
    *  Walls.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the Walls object is not being configured with an
    *  initialization constructor. */
   void configure(); // cppcheck-suppress [duplInheritedMember]

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the reference to the physical entity data.
    *  @param physical_data_ptr Pointer to the PhysicalEntity data instance. */
   virtual void set_data( BallWalls * walls_data_ptr )
   {
      walls = walls_data_ptr;
      return;
   }

   /*! @brief Get the reference to the physical entity data.
    *  @return Pointer to the PhysicalEntity data. */
   virtual BallWalls *get_data()
   {
      return ( walls );
   }

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute * floor_y_pos_attr;      ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute * right_wall_x_pos_attr; ///< @trick_io{**} Time Attribute.
   TrickHLA::Attribute * ceiling_y_pos_attr;    ///< @trick_io{**} Position Attribute.
   TrickHLA::Attribute * left_wall_x_pos_attr;  ///< @trick_io{**} Velocity Attribute.

   // Assign to these parameters when setting up the data associations for the
   // TrickHLAObject data for the Walls.
   double floor_y_pos;      /**< (m)  Horizontal floor location on Y axis. */
   double right_wall_x_pos; /**< (m)  Vertical right wall location on X axis. */
   double ceiling_y_pos;    /**< (m)  Horizontal ceiling location on Y axis. */
   double left_wall_x_pos;  /**< (m)  Vertical left wall location on X axis. */

   /*! @brief Print out the packing data debug information.
    *  @param stream Output stream. */
   virtual void debug_print( std::ostream &stream = std::cout ) const;

   // Reference to simulation Walls position data.
   BallWalls * walls; ///< @trick_units{--} Walls position data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for WallsPacking class.
    *  @details This constructor is private to prevent inadvertent copies. */
   WallsPacking( WallsPacking const &rhs );
   /*! @brief Assignment operator for WallsPacking class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   WallsPacking &operator=( WallsPacking const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_WALLS_PACKING_H: Do NOT put anything after this line!
