/*
 * PURPOSE: (Class to represent a point)
 */
#ifndef POINT_HH
#define POINT_HH
class Point
{
  private:
   double coordinates[2]; /* *io (m) Array to store x and y coordinates*/

  public:
   Point()
   {
      coordinates[0] = 0.0; // Initialize x coordinate to 0.0
      coordinates[1] = 0.0; // Initialize y coordinate to 0.0
   }

   Point( double X, double Y )
   {
      coordinates[0] = X; // Set x coordinate
      coordinates[1] = Y; // Set y coordinate
   }

   double getX() const
   {
      return coordinates[0]; // Return x coordinate
   }

   void setX( double X )
   {
      coordinates[0] = X; // Set x coordinate
   }

   double getY() const
   {
      return coordinates[1]; // Return y coordinate
   }

   void setY( double Y )
   {
      coordinates[1] = Y; // Set y coordinate
   }
};
#endif
