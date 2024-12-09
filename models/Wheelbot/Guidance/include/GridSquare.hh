/********************************* TRICK HEADER *******************************
PURPOSE: (GridSquare class)
*******************************************************************************/
#ifndef GRIDSQUARE_HH
#define GRIDSQUARE_HH

namespace TrickHLAModel
{

class GridSquare
{
  public:
   bool        is_blocked;
   char        mark;
   GridSquare *parent;
   int         g_score;
   int         f_score;
};

} // namespace TrickHLAModel
#endif
