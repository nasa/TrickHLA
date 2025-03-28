/**
@file SpaceFOM/RelStateBase.hh
@ingroup SpaceFOM
@brief This is a base class for computing relative states with respect to
SISO Space Reference FOM Reference Frames.

@copyright Copyright 2024 United States Government as represented by the
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
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateData.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameTree.cpp}
@trick_link_dependency{../../source/SpaceFOM/RelStateBase.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

#ifndef SPACEFOM_REL_STATE_BASE_HH
#define SPACEFOM_REL_STATE_BASE_HH

// System include files.
#include <string>

// Trick include files.

// TrickHLA model include files.

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityData.hh"
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/RefFrameTree.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM
{

class RelStateBase : public SpaceFOM::PhysicalEntityData
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RelStateBase();

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  public:
   /*! @brief Default constructor for the SpaceFOM RelStateBase class.
    *  @param wrt_frame The frame in which to express the source state.
    *  @param tree  The reference frame tree containing all reference frames. */
   RelStateBase( RefFrameBase &wrt_frame, RefFrameTree &tree );
   /*! @brief Destructor for the SpaceFOM RelStateBase class. */
   virtual ~RelStateBase();

   /*! @brief Set the frame in which to express a source state.
    *  @return True if frame is set, false otherwise.
    *  @param wrt_frame Frame in which to express a source state. */
   bool set_frame( char const *wrt_frame );

   /*! @brief Set the frame in which to express a source state.
    *  @return True if frame is set, false otherwise.
    *  @param wrt_frame Frame in which to express a source state. */
   bool set_frame( std::string const &wrt_frame );

   /*! @brief Set the frame in which to express a source state.
    *  @return True if frame is set.  In this case always true.
    *  @param wrt_frame Frame in which to express a source state. */
   bool set_frame( RefFrameBase const &wrt_frame )
   {
      this->express_frame = &wrt_frame;
      return ( true );
   }

   /*! @brief Get the frame in which a source state will be expressed.
    *  @return Frame in which an entity state will be expressed. */
   RefFrameBase const *get_frame() { return ( express_frame ); }

   /*! @brief Get a copy of the current transformation path.
    *  @return Transformation data associated with current frame path. */
   RefFrameData get_transform() { return ( path_transform ); } // cppcheck-suppress [returnByReference]

   /*! @brief Compute the state of an entity with respect to a given frame.
    *  @return True if state successfully computed, false otherwise.
    *  @param entity The entity state to express in a different frame. */
   bool compute_state( PhysicalEntityData const *entity );

   /*! @brief Compute the state of an entity with respect to a given frame.
    *  @return True if state successfully computed, false otherwise.
    *  @param entity The entity state to express in a different frame.
    *  @param wrt_frame The frame in which to express the source state. */
   bool compute_state( PhysicalEntityData const *source, char const *wrt_frame );

   /*! @brief Compute the state of an entity with respect to a given frame.
    *  @return True if state successfully computed, false otherwise.
    *  @param entity The entity state to express in a different frame.
    *  @param wrt_frame The frame in which to express the source state. */
   bool compute_state( PhysicalEntityData const *source, std::string const &wrt_frame );

   /*! @brief Compute the state of an entity with respect to a given frame.
    *  @return True if state successfully computed, false otherwise.
    *  @param entity The entity state to express in a different frame.
    *  @param wrt_frame The frame in which to express the source state. */
   bool compute_state( PhysicalEntityData const *source, RefFrameBase const *wrt_frame );

   /*! @brief Print out the path transformation data.
    *  @param stream Output stream. */
   virtual void print_path_transform( std::ostream &stream = std::cout ) const;

  protected:
   RefFrameBase const *express_frame;  ///< @trick_units{--} Frame in which state is expressed.
   RefFrameTree       *frame_tree;     ///< @trick_units{--} Frame tree.
   RefFrameData        path_transform; /**< @trick_units{--} The reference frame transformation data
                                            needed to transform from an entity's
                                            parent frame into a desired express frame. */

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RelStateBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RelStateBase( RelStateBase const &rhs );
   /*! @brief Assignment operator for RelStateBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RelStateBase &operator=( RelStateBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REL_STATE_BASE_HH: Do NOT put anything after this line!
