/********************************* TRICK HEADER *******************************
PURPOSE:
    (Integrator default initialization data.)
REFERENCE:
    (((Bailey, R.W, and Paddock, E.J.)
      (Trick Simulation Environment) (NASA:JSC #37943)
      (JSC/Engineering Directorate/Software, Robotics and Simulation Division)
      (March 1997)))
ASSUMPTIONS AND LIMITATIONS:
    ((See Trick documentation.))
PROGRAMMERS:
    (((Edwin Z. Crues) (Titan-AEU) (Jan 2003) (HLA Test Integration Data File)))
*******************************************************************************/

/*
 * $Log: integrator.d,v $
 * Revision 8.1  2009-08-20 14:18:45-05  dmidura
 * Bump version number for new group TrickHLA_2.2
 *
 * Revision 7.1  2009-08-05 10:31:17-05  dmidura
 * Bump version number for new group TrickHLA_2.1
 *
 * Revision 6.1  2009-06-29 13:27:51-05  dmidura
 * Bump version number for new group TrickHLA_2.0
 *
 * Revision 5.2  2009-06-25 17:23:26-05  ddexter
 * Updated division name.
 *
 * Revision 3.1  2008-08-25 12:11:25-05  razor
 * Bump version number for new group TrickHLA_1.4
 *
 * Revision 2.1  2008-08-14 11:20:30-05  dmidura
 * Bump version number for new group TrickHLA_1.3
 *
 * Revision 1.6  2008-04-04 19:04:03-05  ddexter
 * Files from v1.1
 *
 * Revision 1.5  2008-04-04 18:43:29-05  ddexter
 * bumping the version
 *
 * Revision 1.4  2006-10-27 13:23:17-05  ddexter
 * Get code ready for release.
 *
 * Revision 1.3  2006-09-06 16:27:23-05  ddexter
 * Add Trick Headers and remove dead code.
 *
 * Revision 1.2  2006-09-01 15:13:17-05  ddexter
 * Adding HLA Interactions
 *
 * Revision 1.1  2006-08-23 15:05:27-05  ddexter
 * Initial revision
 *
 * Revision 1.1  2004-05-17 15:07:35-05  ltran
 * Initial revision
 *
 * Revision 3.2  2002/10/07 15:15:58  lin
 * Add rcs version info to all trick_models files
 *
 */

#define NUM_STEP        12  /* use up to 12 intermediate steps:
                               8th order RK Fehlberg */
#define NUM_VARIABLES   4   /* x,y position state and x,y velocity state */

/* Unconstrained arrays must be sized */
INTEGRATOR.state    [NUM_VARIABLES] ;
INTEGRATOR.deriv    [NUM_STEP][NUM_VARIABLES] ;
INTEGRATOR.state_ws [NUM_STEP][NUM_VARIABLES] ;

INTEGRATOR.num_state    = NUM_VARIABLES ;
INTEGRATOR.option       = Runge_Kutta_2 ;    /* 2nd order Runge Kutta */
INTEGRATOR.init         = True ;
INTEGRATOR.first_step_deriv = Yes ;

#undef NUM_STEP
#undef NUM_VARIABLES

