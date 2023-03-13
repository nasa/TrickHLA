/*!
@file TrickHLA/OwnershipHandler.hh
@ingroup TrickHLA
@brief This class represents ownership transfer of HLA attributes for a
specific object.

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

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, December 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_OWNERSHIP_HANDLER_HH
#define TRICKHLA_OWNERSHIP_HANDLER_HH

// System include files.
#include <string>

// TrickHLA include files.
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Object;
class Attribute;
class OwnershipItem;

// FIXME: We need to rename this. Unfortunately, AttributeMap is already
// being used by Attribute.hh.

typedef std::map< std::string, Attribute * > THLAAttributeMap; // ** Map of TrickHLA-Attributes.

// The Key is the time for the requested ownership transfer.
typedef std::map< double, THLAAttributeMap *, std::less< double > > AttributeOwnershipMap; // ** Map of attribute-maps.

class OwnershipHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__OwnershipHandler();

   // Let the Object class have full access to the pull and push requests.
   friend class Object;

  public:
   //-----------------------------------------------------------------
   // Constructors / destructor
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA Attribute class. */
   OwnershipHandler();
   /*! @brief Destructor for the TrickHLA Attribute class. */
   virtual ~OwnershipHandler();

  public:
   // The following routines decode / encode the push & pull
   // AttributeOwnershipMaps into / from linear arrays.
   /*! @brief Encodes the push and pull attribute ownership maps into
    * checkpoint-able queues.*/
   void setup_checkpoint_requests();

   /*! @brief Clears out the push / pull checkpoint-able queues. */
   void clear_checkpoint();

   /*! @brief Decodes the push / pull checkpoint-able queues back into
    * attribute ownership maps. */
   void restore_requests();

   /*! @brief Initializes the callback to the interaction.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( Object *obj );

   /*! @brief Returns the name of the object.
    *  @return Name of the object. */
   std::string get_object_name() const;

   /*! @brief Returns the FOM name of the object.
    *  @return FOM name of the object. */
   std::string get_object_FOM_name() const;

   /*! @brief Returns the number of attributes the object contains.
    *  @return Number of attributes. */
   int get_attribute_count() const;

   /*! @brief Returns the attribute FOM names of the object.
    *  @return Vector of attribut FOM name strings. */
   VectorOfStrings get_attribute_FOM_names() const;

   /*! @brief Query if the attribute is locally owned.
    *  @return True if attribute is locally owned; False otherwise.
    *  @param attribute_FOM_name Attribute FOM name. */
   bool is_locally_owned( char const *attribute_FOM_name );

   /*! @brief Query if the attribute is remotely owned.
    *  @return True if attribute is remotely owned; False otherwise.
    *  @param attribute_FOM_name Attribute FOM name. */
   bool is_remotely_owned( char const *attribute_FOM_name );

   /*! @brief Query if the attribute is published.
    *  @return True if attribute is published; False otherwise.
    *  @param attribute_FOM_name Attribute FOM name. */
   bool is_published( char const *attribute_FOM_name );

   /*! @brief Query if the attribute is subscribed.
    *  @return True if attribute is subscribed; False otherwise.
    *  @param attribute_FOM_name Attribute FOM name. */
   bool is_subscribed( char const *attribute_FOM_name );

   /*! @brief Pull ownership of all object attributes as soon as possible. */
   void pull_ownership();

   /*! @brief Pull ownership of all object attributes at the specified time.
    *  @param time Requested time to pull ownership. */
   void pull_ownership( double time );

   /*! @brief Pull ownership of the specified attribute as soon as possible.
    *  @param attribute_FOM_name Attribute FOM name. */
   void pull_ownership( char const *attribute_FOM_name );

   /*! @brief Pull ownership of the specified attribute at the given time.
    *  @param attribute_FOM_name Attribute FOM name.
    *  @param time               Requested time to pull ownership. */
   void pull_ownership( char const *attribute_FOM_name, double time );

   /*! @brief Push ownership of all the object attributes as soon as possible. */
   void push_ownership();

   /*! @brief Push ownership of all the object attributes at the specified time.
    *  @param time Requested time to push ownership. */
   void push_ownership( double time );

   /*! @brief Push ownership of the specified attribute as soon as possible.
    *  @param attribute_FOM_name Attribute FOM name. */
   void push_ownership( char const *attribute_FOM_name );

   /*! @brief Push ownership of the specified attribute at the given time.
    *  @param attribute_FOM_name Attribute FOM name.
    *  @param time Requested time to push ownership. */
   void push_ownership( char const *attribute_FOM_name, double time );

   /*! @brief Return a copy of the object's lookahead time.
    *  @return A copy of the fedetate's lookahead time */
   Int64Interval get_lookahead() const;

   /*! @brief Return a copy of the granted HLA logical time.
    *  @return A copy of the federation granted time. */
   Int64Time get_granted_time() const;

   /*! @brief Get the current scenario time.
    *  @return Returns the current scenario time in seconds. */
   double get_scenario_time();

   /*! @brief Get the current Central Timing Equipment (CTE) time.
    *  @return Returns the current CTE time. */
   double get_cte_time();

  protected:
   /*! @brief Returns the attribute for the given attribute FOM name or NULL
    * if an attribute corresponding to the FOM name is not found.
    * @return Attribute of the object.
    * @param attribute_FOM_name Attribute FOM name.*/
   Attribute *get_attribute( char const *attribute_FOM_name );

   Object *object; ///< @trick_io{**} Reference to the TrickHLA Object.

   AttributeOwnershipMap pull_requests; ///< @trick_io{**} Map of pull ownership user requests.
   AttributeOwnershipMap push_requests; ///< @trick_io{**} Map of push ownership user requests.

   size_t         pull_items_cnt; ///< @trick_units{count} Number of pull items
   OwnershipItem *pull_items;     ///< @trick_units{--}    Array of pulled attributes
   size_t         push_items_cnt; ///< @trick_units{count} Number of push items
   OwnershipItem *push_items;     ///< @trick_units{--}    Array of pushed attributes

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for OwnershipHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   OwnershipHandler( OwnershipHandler const &rhs );
   /*! @brief Assignment operator for OwnershipHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   OwnershipHandler &operator=( OwnershipHandler const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_OWNERSHIP_HANDLER_HH: Do NOT put anything after this line!
