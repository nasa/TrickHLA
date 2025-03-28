/*!
@file SpaceFOM/RefFrameTree.hh
@ingroup SpaceFOM
@brief This class provides an reference implementation for a SpaceFOM
reference frame tree.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to a Reference Frame tree object. This needs to be available
to the SpaceFOM initialization process for the root reference frame
discovery step in the initialization process.

The reference frame tree takes the form of a Directed Acyclic Graph (DAG).
A node in the tree is a SpaceFOM::RefFrameBase object.

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
@trick_link_dependency{../../source/SpaceFOM/LRTreeNodeBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/LRTreeBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameTree.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_TREE_HH
#define SPACEFOM_REF_FRAME_TREE_HH

// System include files.
#include <map>
#include <string>

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/LRTreeBase.hh"
#include "SpaceFOM/RefFrameBase.hh"

namespace SpaceFOM
{

class RefFrameTree : public SpaceFOM::LRTreeBase
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameTree();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM RefFrameBase class. */
   RefFrameTree();
   /*! @brief Destructor for the SpaceFOM RefFrameBase class. */
   virtual ~RefFrameTree();

   /*! @brief Add a reference frame to the tree.
    *  @details Add a SpaceFOM reference frame into the reference frame tree.
    *  @param frame_ptr Pointer to the reference frame to add.
    *  @return Success or failure of the add. */
   virtual bool add_frame( RefFrameBase *frame_ptr );

   /*! @brief Iterate through the frames and build a consistent tree.
    *  @return True on success, false otherwise. */
   virtual bool build_tree(); // cppcheck-suppress [uselessOverride]

   /*! @brief Check for consistency of the tree.
    *  @return True on success, false otherwise. */
   virtual bool check_tree(); // cppcheck-suppress [uselessOverride]

   /*! @brief Check if a frame is in the tree.
    *  @return True if in the tree, false otherwise.
    *  @param name Name of the frame to check for. */
   virtual bool has_frame( char const *name );

   /*! @brief Check if a frame is in the tree.
    *  @return True if in the tree, false otherwise.
    *  @param name Name of the frame to check for. */
   virtual bool has_frame( std::string const &name );

   /*! @brief Check if a frame is in the tree.
    *  @return True if in the tree, false otherwise.
    *  @param name Reference of the frame to check for. */
   virtual bool has_frame( RefFrameBase const *frame );

   /*! @brief Find a frame in the tree.
    *  @return Tree frame instance if found, NULL otherwise.
    *  @param name Name of the frame to check for. */
   virtual RefFrameBase *find_frame( char const *name );

   /*! @brief Find a frame in the tree.
    *  @return Tree frame instance if found, NULL otherwise.
    *  @param name Name of the frame to check for. */
   virtual RefFrameBase *find_frame( std::string const &name );

   /*! @brief Build a composite transform from source to express frame.
    *  @detail The frame is constructed by using the pre-computed path from
    *  source to the express frame.
    *  @return True if successfully built transformation, false otherwise.
    *  @param source_frame Starting frame in the ReferenceFrameTree.
    *  @param express_frame Desired express frame in the ReferenceFrameTree.
    *  @param transform_data Transformation from the source into the express frame. */
   virtual bool build_transform( RefFrameBase const *source_frame,
                                 RefFrameBase const *express_frame,
                                 RefFrameData       *transform_data );

   /*! @brief Print out the Reference Frame Tree nodes.
    *  @param stream Output stream. */
   virtual void print_tree( std::ostream &stream = std::cout ) const;

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RefFrameTree class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameTree( RefFrameTree const &rhs );
   /*! @brief Assignment operator for RefFrameTree class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameTree &operator=( RefFrameTree const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_TREE_HH: Do NOT put anything after this line!
