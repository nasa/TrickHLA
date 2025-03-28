/*!
@file IMSim/ExecutionConfiguration.hh
@ingroup IMSim
@brief Definition of the TrickHLA IMSim Execution Configuration Object (ExCO).

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

@python_module{IMSim}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/IMSim/ExecutionConfiguration.cpp}
@trick_link_dependency{../../source/IMSim/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, SISO IMSim Initialization.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, IMSim support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef IMSIM_EXECUTION_CONFIGURATION_HH
#define IMSIM_EXECUTION_CONFIGURATION_HH

// System include files.
#include <cstdint>

// TrickHLA include files.
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/Packing.hh"

// IMSim include files.
#include "IMSim/Types.hh"

namespace IMSim
{

class ExecutionControl;

class ExecutionConfiguration : public TrickHLA::ExecutionConfigurationBase
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrIMSim__ExecutionConfiguration();

   // Set up friend classes for controlled access.
   friend class IMSim::ExecutionControl;

  public:
   // The members below are part of the FOM data IMSim simulation configuration exchange.
   char *owner; /**< @trick_units{--}
      Specifies the name of the federate publishing the simulation
      configuration object. */

   char *scenario; /**< @trick_units{--}
      Specifies the identifying string associated with the scenario being
      executed. */

   char *mode; /**< @trick_units{--}
      Specifies the mode string describing the current federation execution
      run status. */

   int64_t run_duration; /**< @trick_units{us}
      Duration of the federation execution expressed in microseconds. */

   int32_t number_of_federates; /**< @trick_units{--}
      Number of the required federates for the federation execution. */

   char *required_federates; /**< @trick_units{--}
      Specifies the name of the required federates for the federation execution. */

   int32_t start_year; /**< @trick_units{yr}
      Starting year of the federation execution. */

   double start_seconds; /**< @trick_units{s}
      Starting time in seconds of year of the federation execution. */

   double DUT1; /**< @trick_units{s} A correction factor that approximates the
      difference between the UTC and UT1 time scales. UTC ~= UT1 + DUT1 */

   int32_t deltaAT; /**< @trick_units{s} Number of leap seconds that separate
      the UTC and TAI time scales.  TAI = UTC + deltaAT */

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the IMSim ExecutionConfiguration class. */
   ExecutionConfiguration();
   /*! @brief Initialization constructor for the TrickHLA IMSim ExecutionConfiguration class.
    *  @param s_define_name Full path name in the S_define for this ExecutionConfiguration instance. */
   explicit ExecutionConfiguration( char const *s_define_name );
   /*! @brief Pure virtual destructor for the IMSim ExecutionConfiguration class. */
   virtual ~ExecutionConfiguration();

   // Default data.
   /*! @brief Sets up the attributes for the simulation configuration using default values.
    *  These can be overridden in the input file. */
   virtual void configure_attributes();

   /*! @brief Sets up the attributes for the simulation configuration using
    *  default values.  These can be overridden in the input file.
    *  @param sim_config_name S_define level Trick name for simulation configuration object. */
   virtual void configure_attributes( char const *sim_config_name );

   /*! @brief Configure the execution configuration object. */
   virtual void configure();

   /*! @brief Get the reference to the associated IMSim::ExecutionControl object.
    *  @param exec_control Pointer to the associated IMSim::ExecutionControl object. */
   virtual void set_imsim_control( IMSim::ExecutionControl *exec_control );

   /*! @brief Get the reference to the associated IMSim::ExecutionControl object.
    *  @return Pointer to the associated IMSim::ExecutionControl object. */
   virtual IMSim::ExecutionControl *get_imsim_control();

   // From the TrickHLA::Packing class.
   virtual void pack();
   virtual void unpack();

   //
   // FOM data public accessor interface.
   //
   /*! @brief Set the name of the simulation configuration owner.
    *  @param owner_name Name of the simulation configuration owner. */
   virtual void set_owner( char const *owner_name );
   /*! @brief Get the name of the simulation configuration owner.
    *  @return Name of the simulation configuration owner as a constant string. */
   virtual char const *get_owner()
   {
      return owner;
   }

   /*! @brief Set the scenario execution description string.
    *  @param scenario_id String describing the federation execution scenario. */
   virtual void set_scenario( char const *scenario_id );
   /*! @brief Get the scenario execution description string.
    *  @return Scenario execution description as a constant string. */
   virtual char const *get_scenario()
   {
      return scenario;
   }

   /*! @brief Set the execution mode description string.
    *  @param mode_id String describing the federation execution mode. */
   virtual void set_mode( char const *mode_id );
   /*! @brief Get the execution mode description string.
    *  @return Execution mode description as a constant string. */
   virtual char const *get_mode()
   {
      return mode;
   }

   /*! @brief Set the planned federation execution run duration in microseconds.
    *  @param run_time Planned federation execution run duration in microseconds. */
   virtual void set_run_duration( int64_t run_time )
   {
      run_duration = run_time;
   }
   /*! @brief Get planned federation execution run duration in microseconds.
    *  @return The planned federation execution run duration in microseconds. */
   virtual int64_t get_run_duration()
   {
      return run_duration;
   }

   /*! @brief Set the number of required federates for this federation execution.
    *  @param num_federates Number of required federates for this federation execution. */
   virtual void set_number_of_federates( int32_t num_federates )
   {
      number_of_federates = num_federates;
   }
   /*! @brief Get the number of required federates for this federation execution.
    *  @return The number of required federates for this federation execution. */
   virtual int32_t get_number_of_federates()
   {
      return number_of_federates;
   }

   /*! @brief Set the list of required federates in a comma separated string.
    *  @param federates List of required federates in a comma separated string. */
   virtual void set_required_federates( char const *federates );
   /*! @brief Get the list of required federates.
    *  @return List of required federates as a comma separated constant string. */
   virtual char const *get_required_federates()
   {
      return required_federates;
   }

   /*! @brief Set the start year of the federation execution scenario time.
    *  @param year Start year of the federation execution scenario time. */
   virtual void set_start_year( int32_t year )
   {
      start_year = year;
   }
   /*! @brief Get the start year of the federation execution scenario time.
    *  @return The start year of the federation execution scenario time. */
   virtual int32_t get_start_year()
   {
      return start_year;
   }

   /*! @brief Set the start seconds of year for the federation execution scenario time.
    *  @param soy Start seconds of year for the federation execution scenario time. */
   virtual void set_start_seconds( double soy )
   {
      start_seconds = soy;
   }
   /*! @brief Get the start seconds of the federation execution scenario time..
    *  @return The start seconds of the federation execution scenario time. */
   virtual double get_start_seconds()
   {
      return start_seconds;
   }

   /*! @brief Set the offset between UT1 and UTC.
    *  @param offset The offset between UT1 and UTC. */
   virtual void set_DUT1( double offset )
   {
      DUT1 = offset;
   }
   /*! @brief Get the offset between UT1 and UTC.
    *  @return The offset between UT1 and UTC. */
   virtual double get_DUT1()
   {
      return DUT1;
   }

   /*! @brief Set the leap second time offset between TAI and UTC.
    *  @param leap_seconds The leap second time offset between TAI and UTC. */
   virtual void set_deltaAT( int32_t leap_seconds )
   {
      deltaAT = leap_seconds;
   }
   /*! @brief Get the leap second time offset between TAI and UTC.
    *  @return The leap second time offset between TAI and UTC. */
   virtual int32_t get_deltaAT()
   {
      return deltaAT;
   }

   // IMSim Simulation Configuration specific functions.
   /*! @brief Setup the Trick Ref Attributes for the ExecutionConfiguration object.
    *  @param packing_obj Associated packing object. */
   virtual void setup_ref_attributes( TrickHLA::Packing *packing_obj );

   /*! @brief Print the current Execution Configuration object to the console. */
   virtual void print_execution_configuration() const;

   /*! @brief Print the current simulation configuration state to the console. */
   virtual void print_simconfig( std::ostream &stream = std::cout ) const;

   /*! @brief Wait on an ExCO update.
    *  @return True for successful wait. */
   virtual bool wait_for_update();

  private:
   // Do not allow the copy constructor or assignment operator.
   ExecutionConfiguration( ExecutionConfiguration const &rhs );
   ExecutionConfiguration &operator=( ExecutionConfiguration const &rhs );
};

} // namespace IMSim

#endif // IMSIM_EXECUTION_CONFIGURATION_HH: Do NOT put anything after this line!
