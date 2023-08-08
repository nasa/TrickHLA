#include "navigator.hh"
#include <iostream>
#include <math.h> // for: sqrt(), atan2(), cos(), and sin()

void Navigator::set_heading( double h )
{
   heading = h;
}

void Navigator::set_location( double x, double y )
{
   location.setX( x );
   location.setY( y );
}

double Navigator::distance_to( Point &map_point )
{

   double deltaX   = location.getX() - map_point.getX();
   double deltaY   = location.getY() - map_point.getY();
   double distance = sqrt( deltaX * deltaX + deltaY * deltaY );
   return ( distance );
}

double Navigator::bearing_to( Point &map_point )
{

   Point  platform_point = convert_map_to_platform( map_point );
   Point  body_point     = convert_platform_to_body( platform_point );
   double head2          = asin( body_point.getY() / distance_to( map_point ) );
   return head2;
}

Point Navigator::convert_map_to_platform( Point &map_point )
{

   Point platform_point;
   platform_point.setX( map_point.getX() - location.getX() );
   platform_point.setY( map_point.getY() - location.getY() );
   return ( platform_point );
}

Point Navigator::convert_platform_to_map( Point &platform_point )
{

   Point map_point;
   map_point.setX( platform_point.getX() + location.getX() );
   map_point.setY( platform_point.getY() + location.getY() );
   return ( map_point );
}

Point Navigator::convert_platform_to_body( Point &platform_point )
{

   Point body_point;
   body_point.setX( cos( heading ) * platform_point.getX() + sin( heading ) * platform_point.getY() );
   body_point.setY( -sin( heading ) * platform_point.getX() + cos( heading ) * platform_point.getY() );
   return ( body_point );
}

Point Navigator::convert_body_to_platform( Point &body_point )
{

   Point platform_point;
   platform_point.setX( cos( heading ) * body_point.getX() - sin( heading ) * body_point.getY() );
   platform_point.setY( sin( heading ) * body_point.getX() + cos( heading ) * body_point.getY() );
   return ( platform_point );
}
