#ifndef GRIDSQUARE_HH
#define GRIDSQUARE_HH

class GridSquare
{
  public:
   bool        is_blocked;
   char        mark;
   GridSquare *parent;
   int         g_score;
   int         f_score;
};
#endif
