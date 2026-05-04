/*!
@file SpaceFOM/SpaceTimeCoordinateConfig.hh
@ingroup SpaceFOM
@brief The SpaceFOM Space Time Coordinate configuration helper class.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/RecordElement.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateConfig.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Initial implementation.}
@revs_end

*/

#ifndef SPACEFOM_SPACE_TIME_COORDINATE_CONFIG_HH
#define SPACEFOM_SPACE_TIME_COORDINATE_CONFIG_HH

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/RecordElement.hh"

namespace SpaceFOM
{

class SpaceTimeCoordinateConfig
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__SpaceTimeCoordinateConfig();

  public:
   /*! @brief Configure the TrickHLA Attribute and the fixed record elements for
    *  the Space Time Coordinate.
    *  @param attribute The Attributes instance to configure.
    *  @param fom_name The FOM name of the attribute.
    *  @param trick_root_name The Trick variable root name for this SpaceTimeCoordinateData instance.
    *  @param config The attribute config setting.
    *  @param publish True to publish the attribute.
    *  @param subscribe True to subscribe to the attribute.
    *  @param locally_owned True if the attribute is locally owned. */
   static void configure(
      TrickHLA::Attribute           *attribute,
      std::string const             &fom_name,
      std::string const             &trick_root_name,
      TrickHLA::DataUpdateEnum const config,
      bool const                     publish,
      bool const                     subscribe,
      bool const                     locally_owned );

   /*! @brief Configure the TrickHLA RecordElement and the fixed record elements
    *  for the Space Time Coordinate.
    *  @param rec_element The RecordElement instance to configure.
    *  @param trick_root_name The Trick variable root name for this SpaceTimeCoordinateData instance. */
   static void configure(
      TrickHLA::RecordElement *rec_element,
      std::string const       &trick_root_name );

  private:
   // This object is not copyable
   /*! @brief Copy constructor for SpaceTimeCoordinateConfig class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SpaceTimeCoordinateConfig( SpaceTimeCoordinateConfig const &rhs );
   /*! @brief Assignment operator for SpaceTimeCoordinateConfig class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SpaceTimeCoordinateConfig &operator=( SpaceTimeCoordinateConfig &rhs );
};

} // namespace SpaceFOM

#endif // End of SPACEFOM_SPACE_TIME_COORDINATE_CONFIG_HH: Do NOT put anything after this line!
