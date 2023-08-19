/*!
@file SpaceFOM/RefFrameTree.hh
@ingroup SpaceFOM
@brief This class provides an reference implementation for a SpaceFOM
reference frame tree.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to a  Reference Frame object. This needs to be available
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
#include "SpaceFOM/RefFrameBase.hh"

namespace SpaceFOM
{

class RefFrameTree
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
    *  @details This function is used to add a SpaceFOM reference frame into
    *  the reference frame tree.
    *  @param frame_ptr Pointer to the reference frame to add.
    *  @return Success or failure of the add. */
   bool add_frame( RefFrameBase *frame_ptr );

   bool build_tree();
   bool check_tree();

   bool has_frame( char const *name );
   bool has_frame( std::string const name );
   bool has_frame( RefFrameBase *frame );

   RefFrameBase *find_frame( char const *name );
   RefFrameBase *find_frame( std::string const name );

   RefFrameBase *find_common_base( char const *child_1,
                                   char const *child_2 );
   RefFrameBase *find_common_base( std::string const child_1,
                                   std::string const child_2 );
   RefFrameBase *find_common_base( RefFrameBase *child_1,
                                   RefFrameBase *child_2 );

  protected:
   // Map used to build and manage the reference frame tree.
   std::map< std::string, RefFrameBase * > ref_frame_map;

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
