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

#ifndef IMSIM_FREEZE_INTERACTION_HANDLER_HH
#define IMSIM_FREEZE_INTERACTION_HANDLER_HH

// TrickHLA include files.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/InteractionHandler.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace IMSim
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class ExecutionControl;

class FreezeInteractionHandler : public TrickHLA::InteractionHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrIMSim__FreezeInteractionHandler();

   // Mark the IMSim ExecutionControl class as a friend.
   friend class ExecutionControl;

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
   void receive_interaction( RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag );

   // FIXME: This is not a good thing to do. Why have the data private?
   /*! @brief Get the address of the interaction time.
    *  @return Pointer to the interaction time. */
   double *get_address_of_interaction_time()
   {
      return ( &time );
   }

   /*! @brief Set the associated IMSim::ExecutionControl.
    *  @param exec_cntrl_ptr Pointer to the associated IMSim::ExecutionControl. */
   void set_exection_control( IMSim::ExecutionControl *exec_cntrl_ptr )
   {
      this->execution_control = exec_cntrl_ptr;
   }

   /*! @brief Set the associated IMSim::ExecutionControl.
    *  @return exec_cntrl_ptr Pointer to the associated IMSim::ExecutionControl. */
   IMSim::ExecutionControl *get_exection_control()
   {
      return ( this->execution_control );
   }

  protected:
   IMSim::ExecutionControl *execution_control; /**<< @trick_io{**} Pointer
     to IMSim execution control instance. */

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
   static bool check_values( double const &v1, double const &v2 )
   {
      return ( ( v1 == v2 ) || ( ( v2 > v1 ) ? ( ( v2 - v1 ) < 0.000001 ) : ( ( v1 - v2 ) < 0.000001 ) ) );
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   FreezeInteractionHandler( FreezeInteractionHandler const &rhs );
   FreezeInteractionHandler &operator=( FreezeInteractionHandler const &rhs );
};

} // namespace IMSim

#endif // IMSIM_FREEZE_INTERACTION_HANDLER_HH: Do NOT put anything after this line!
