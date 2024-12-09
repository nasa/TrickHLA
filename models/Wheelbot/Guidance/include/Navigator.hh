/********************************* TRICK HEADER *******************************
PURPOSE: (Navigator class)

LIBRARY DEPENDENCY:
    ((Guidance/src/Navigator.cpp))

PROGRAMMERS:
    (((John M. Penn) (L3 Communications) (March 2015) (Trick Refresher Project)))
*******************************************************************************/

#ifndef NAVIGATOR_HH
#define NAVIGATOR_HH

#include "Point.hh"

namespace TrickHLAModel
{

class Navigator
{
  public:
   Navigator( double initial_heading, Point initial_location )
      : heading( initial_heading ), location( initial_location )
   {
      return;
   }

   void set_heading( double heading );
   void set_location( double north, double west );

   double distance_to( Point const &map_point );
   double bearing_to( Point const &map_point );

   Point convert_map_to_platform( Point const &map_point );
   Point convert_platform_to_map( Point const &platform_point );
   Point convert_platform_to_body( Point const &platform_point );
   Point convert_body_to_platform( Point const &body_point );

  private:
   double heading;
   Point  location;
};

} // namespace TrickHLAModel
#endif
