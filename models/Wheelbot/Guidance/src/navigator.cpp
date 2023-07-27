#include <math.h> // for: sqrt(), atan2(), cos(), and sin()
#include "navigator.hh"
#include <iostream>

void Navigator::setHeading(double h) {
    heading = h;
}

void Navigator::setLocation(double x, double y) {
    location.setX(x);
    location.setY(y);
}

double Navigator::distanceTo(Point& mapPoint ) {

    double deltaX = location.getX() - mapPoint.getX();
    double deltaY = location.getY() - mapPoint.getY();
    double distance = sqrt( deltaX * deltaX + deltaY * deltaY );
    return (distance) ;
}

double Navigator::bearingTo(Point& mapPoint ) {

    Point platformPoint = convertMapToPlatform(mapPoint);
    Point bodyPoint = convertPlatformToBody(platformPoint);
    double head2 = asin(bodyPoint.getY()/distanceTo(mapPoint) );
    return head2;
}

Point Navigator::convertMapToPlatform(Point& mapPoint ) {

    Point platformPoint;
    platformPoint.setX(mapPoint.getX() - location.getX());
    platformPoint.setY(mapPoint.getY() - location.getY());
    return (platformPoint);
}

Point Navigator::convertPlatformToMap( Point& platformPoint ) {

    Point mapPoint;
    mapPoint.setX(platformPoint.getX() + location.getX());
    mapPoint.setY(platformPoint.getY() + location.getY());
    return (mapPoint);
}

Point Navigator::convertPlatformToBody( Point& platformPoint ) {

    Point bodyPoint;
    bodyPoint.setX(cos(heading) * platformPoint.getX() + sin(heading) * platformPoint.getY());
    bodyPoint.setY(-sin(heading) * platformPoint.getX() + cos(heading) * platformPoint.getY());
    return (bodyPoint);
}

Point Navigator::convertBodyToPlatform( Point& bodyPoint ) {

    Point platformPoint;
    platformPoint.setX(cos(heading) * bodyPoint.getX() - sin(heading) * bodyPoint.getY());
    platformPoint.setY(sin(heading) * bodyPoint.getX() + cos(heading) * bodyPoint.getY());
    return (platformPoint);
}