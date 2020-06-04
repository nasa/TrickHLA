/*!
@file models/sine/include/SineInteractionHandler.hh
@ingroup TrickHLAModel
@brief This class handles the HLA interactions for the sine wave simulation.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{sine/src/SineInteractionHandler.o}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, Aug 2006, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_MODEL_SINE_INTERACTION_HANDLER_HH_
#define _TRICKHLA_MODEL_SINE_INTERACTION_HANDLER_HH_

// We need the forward declaration and the include so that the Trick IO source
// code will compile if we flesh out the class only in the header file.
#include "TrickHLA/InteractionHandler.hh"
#include "TrickHLA/StandardsSupport.hh"

#define SINE_MSG_SIZE 2

namespace TrickHLAModel
{

class SineInteractionHandler : public TrickHLA::InteractionHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineInteractionHandler();

 public:
   char *name;    ///< @trick_units{--} Example of a unique name to identify the interaction handler.
   char *message; ///< @trick_units{--} Example of a static array of strings.

 public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineInteractionHandler class. */
   SineInteractionHandler();
   /*! @brief Destructor for the TrickHLAModel SineInteractionHandler class. */
   virtual ~SineInteractionHandler();

 public:
   /*! @brief Send the HLA interaction using either Timestamp Order or Receive
    * Order which is determined at compile time.
    *  @param send_time HLA time to send the interaction {s}. */
   void send_sine_interaction(double send_time);

   /*! @brief Receive the HLA interaction.
    *  @param the_user_supplied_tag User tag. */
   virtual void receive_interaction(RTI1516_USERDATA const &the_user_supplied_tag);

 protected:
   double time; ///< @trick_units{s}  Example of floating-point data.
   int year;    ///< @trick_units{--} Example of integer data.

   int send_cnt;    ///< @trick_units{--} The number of times an interaction is sent.
   int receive_cnt; ///< @trick_units{--} The number of times an interaction was received.

 private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SineInteractionHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SineInteractionHandler(const SineInteractionHandler &rhs);
   /*! @brief Assignment operator for SineInteractionHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SineInteractionHandler &operator=(const SineInteractionHandler &rhs);

};

} // namespace TrickHLAModel

#endif // _TRICKHLA_MODEL_SINE_INTERACTION_HANDLER_HH_: Do NOT put anything after this line!
