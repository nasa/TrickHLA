/*!
@file SpaceFOM/LRTreeBase.hh
@ingroup SpaceFOM
@brief This class provides an reference implementation for a specialized
form of a Directed Acyclic Graph (DAG) called a Labeled Rooted Tree.

This is the base implementation for specialized Directed Acyclic Graph (DAG)
referred to as a Labeled Rooted Tree (LRTree).  In addition to the acyclic
characteristic of a DAG, the LRTree has a single defined root/top/bottom node
that defines the 'start' of the tree.  The root node will not have a parent
node; all other nodes will reference a single parent node that must also be in
the LRTree.  The resulting hierarchical tree will insure that one and only
one path exists through the LRTree connecting any two nodes.

This construct will be use in the SpaceFOM as the basis for the Reference
Frame Tree.  The paths between nodes will provide the information necessary
to compute transformations between nodes.

Note: this implementation does NOT manage node memory allocation.

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
@trick_link_dependency{../../source/SpaceFOM/LRTreeNodeBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/LRTreeBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, February 2024, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_L_R_TREE_BASE_HH
#define SPACEFOM_L_R_TREE_BASE_HH

// System include files.
#include <iostream>
#include <map>
#include <string>
#include <vector>

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/LRTreeNodeBase.hh"

namespace SpaceFOM
{

typedef std::vector< LRTreeNodeBase * >           LRTreeNodeVector;
typedef std::set< const LRTreeNodeBase * >        LRTreeNodeSet;
typedef std::map< std::string, LRTreeNodeBase * > LRTreeNodeMap;
class LRTreeBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__LRTreeBase();

   // Mark the LRTreeNodeBase class as a friend class.
   friend class LRTeeNodeBase;

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor. */
   LRTreeBase();
   /*! @brief Destructor. */
   virtual ~LRTreeBase();

   /*! @brief Add a reference frame to the tree.
    *  @details Add a node to the tree.
    *  @param node_ptr Pointer to the node to add.
    *  @return Success or failure of the add. */
   virtual bool add_node( LRTreeNodeBase *node_ptr );

   virtual bool build_tree();
   virtual bool check_tree();

   virtual bool has_node( unsigned int const node_id );
   virtual bool has_node( char const *name );
   virtual bool has_node( std::string const &name );
   virtual bool has_node( LRTreeNodeBase const *node );

   virtual unsigned int    get_root_node_id() { return ( root_node_id ); }
   virtual LRTreeNodeBase *get_root_node() { return ( root_node_ptr ); }

   LRTreeNodeBase *find_node( unsigned int const node_id );
   LRTreeNodeBase *find_node( char const *name );
   LRTreeNodeBase *find_node( std::string const &name );

   /*! @brief Print out the LRTree nodes.
    *  @param stream Output stream. */
   virtual void print_nodes( std::ostream &stream = std::cout );

  protected:
   LRTreeNodeVector nodes;    ///< @trick_io{**} Node vector used to build and manage the tree.
   LRTreeNodeMap    node_map; ///< @trick_io{**} Map used to build and manage the tree.

   unsigned int       root_node_id;  ///< @trick_units{--} Tree root node ID.
   LRTreeNodeBase    *root_node_ptr; ///< @trick_io{**} Reference to the tree's root node.
   LRTreeNodeVector **paths;         ///< @trick_io{**} Path matrix used to store and retrieve node-to-node paths.

   virtual bool allocate_paths();
   virtual void free_paths();
   virtual void print_paths( std::ostream &stream = std::cout );

   virtual LRTreeNodeBase *find_root();
   virtual bool            is_cyclic( LRTreeNodeBase const *node );

   virtual LRTreeNodeVector *get_path_to_root( unsigned int const node_id );
   virtual LRTreeNodeVector *get_path_to_root( LRTreeNodeBase const *node );

   virtual LRTreeNodeVector *find_path( unsigned int const local,
                                        unsigned int const wrt );

   virtual LRTreeNodeVector *find_path( LRTreeNodeBase const *local,
                                        LRTreeNodeBase const *wrt );

   virtual LRTreeNodeBase *find_common_node( unsigned int const local,
                                             unsigned int const wrt );

   virtual LRTreeNodeBase *find_common_node( LRTreeNodeVector *up_path,
                                             LRTreeNodeVector *down_path );

  private:
   // This object is not copyable
   /*! @brief Copy constructor for LRTreeBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   LRTreeBase( LRTreeBase const &rhs );
   /*! @brief Assignment operator for LRTreeBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   LRTreeBase &operator=( LRTreeBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_L_R_TREE_BASE_HH: Do NOT put anything after this line!
