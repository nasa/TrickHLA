/*!
@file SpaceFOM/LRTreeNodeBase.hh
@ingroup SpaceFOM
@brief This class provides an reference implementation for a node of a
specialized form of a Directed Acyclic Graph (DAG) called a Labeled Rooted Tree.

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

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, February 2024, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_L_R_TREE_NODE_BASE_HH
#define SPACEFOM_L_R_TREE_NODE_BASE_HH

// System include files.
#include <iostream>
#include <string>

// TrickHLA include files.

// SpaceFOM include files.

namespace SpaceFOM
{

class LRTreeNodeBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__LRTreeNodeBase();

   // Mark the LRTreeBase class as a friend class.
   friend class LRTreeBase;

  public:
   char           *name;   ///< @trick_units{--} Node name.
   LRTreeNodeBase *parent; ///< @trick_units{--} Pointer to parent node.

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor. */
   LRTreeNodeBase();

   /*! @brief Initialization constructor.
    *  @param node_name The name of the node to be constructed.
    *  @param node_parent The parent node in the tree for the constructed node. */
   LRTreeNodeBase( char const     *node_name,
                   LRTreeNodeBase *node_parent );

   /*! @brief Destructor. */
   virtual ~LRTreeNodeBase();

   /*! @brief Get the node's path index/ID.
    *  @return LRTree node's path index/ID. */
   inline virtual unsigned int get_id() const
   {
      return ( node_id );
   }

   /*! @brief Set the name of the node.
    *  @param node_name Name of the node. */
   virtual void set_name( char const *node_name );

   /*! @brief Check to see if this is a root node for the tree.
    *  @return True if this is the root node, false otherwise. */
   virtual bool is_root() { return ( is_root_node ); }

   /*! @brief Set this reference frame as the root reference frame.
    *  @return True if set succeeded, false otherwise. */
   virtual bool set_root( bool root_state );

   /*! @brief Print out the LRTree node.
    *  @param stream Output stream. */
   virtual void print_node( std::ostream &stream = std::cout ) const;

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   bool is_root_node; ///< @trick_units{--} Indicator that this is the root node of the tree.

   unsigned int node_id; /**< @trick_units{--} Index into the path matrix that is
      assigned by the LRTreeBase class when a node is added. */

  private:
   // This object is not copyable
   /*! @brief Copy constructor for LRTreeNodeBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   LRTreeNodeBase( LRTreeNodeBase const &rhs );
   /*! @brief Assignment operator for LRTreeNodeBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   LRTreeNodeBase &operator=( LRTreeNodeBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_L_R_TREE_NODE_BASE_HH: Do NOT put anything after this line!
