/*!
@file IMSim/FreezeInteractionHandler.hh
@ingroup IMSim
@brief This class is a specialized class for handling HLA Freeze interactions.

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

@python_module{IMSim}

@tldh
@trick_link_dependency{../../source/TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../../source/IMSim/FreezeInteractionHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, July 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _IMSIM_FREEZE_INTERACTION_HANDLER_HH_
#define _IMSIM_FREEZE_INTERACTION_HANDLER_HH_

// System include files.

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/InteractionHandler.hh"

// HLA include files.
#include RTI1516_HEADER

namespace IMSim
{

class FreezeInteractionHandler : public TrickHLA::InteractionHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__FreezeInteractionHandler();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA FreezeInteractionHandler class. */
   FreezeInteractionHandler();
   /*! @brief Destructor for the TrickHLA FreezeInteractionHandler class. */
   virtual ~FreezeInteractionHandler();

  public:
   /*! @brief Send the freeze interaction scenario time immediately using Timestamp Order.
    *  @param freeze_time Reference to the freeze scenario time, typically Terrestrial Time (TT).
    *  @param late_joining_federate True if late joining federate; False otherwise. */
   void send_scenario_freeze_interaction( double &freeze_time, bool late_joining_federate = false );

   /*! @brief Called when the interaction is received from the RTI.
    *  @param theUserSuppliedTag User tag. */
   void receive_interaction( RTI1516_USERDATA const &theUserSuppliedTag );

   // FIXME: This is not a good thing to do.  Why have the data private?
   /*! @brief Get the address of the interaction time.
    *  @return Pointer to the interaction time. */
   double *get_address_of_interaction_time()
   {
      return ( &time );
   }

  private:
   //
   // Private data
   //
   double time; /**< @trick_units{s} Scenario Time on which to freeze
                     simulation execution on a major frame boundary. */

   //
   // Private methods
   //
   /*! @brief Value comparison check.
    *  @return True is values match to within a tolerance; False otherwise.
    *  @param v1 First value in comparison.
    *  @param v2 Second value in comparison. */
   static bool check_values( const double &v1, const double &v2 )
   {
      return ( ( v1 == v2 ) || ( ( v2 > v1 ) ? ( ( v2 - v1 ) < 0.000001 ) : ( ( v1 - v2 ) < 0.000001 ) ) );
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   FreezeInteractionHandler( const FreezeInteractionHandler &rhs );
   FreezeInteractionHandler &operator=( const FreezeInteractionHandler &rhs );
};

} // namespace IMSim

#endif // _IMSIM_FREEZE_INTERACTION_HANDLER_HH_: Do NOT put anything after this line!
