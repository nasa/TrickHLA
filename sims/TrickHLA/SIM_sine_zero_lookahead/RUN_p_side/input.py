
#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe(True)
#trick.checkpoint_pre_init(1)
trick.checkpoint_post_init(1)
#trick.add_read(0.0 , '''trick.checkpoint('checkpoint')''')

# NOTE: You must set this to be the same as the master federate's frame for IMSim freezing
trick.exec_set_software_frame(0.25)
trick.exec_set_stack_trace(False)

# Trick config
trick.exec_set_enable_freeze(False)
trick.exec_set_freeze_command(False)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(False)

run_duration = 15.0


#---------------------------------------------
# Set up data to record.
#---------------------------------------------
exec(open( "Log_data/log_sine_states.py" ).read())
log_sine_states( 'A', 0.250 )
log_sine_states( 'P', 0.250 )
log_sine_states( 'AC', 0.250 )
log_sine_states( 'PC', 0.250 )

#---------------------------------------------
# Set up the initial Sine states
#---------------------------------------------
exec(open( "Modified_data/sine_init.py" ).read())


#TODO: setup integration
#P.integ.option = Runge_Kutta_Fehlberg_45
#P.integ.first_step_deriv = Yes


# Example of a 1-dimensional dynamic array.
A.packing.buff_size = 10
A.packing.buff = trick.sim_services.alloc_type( A.packing.buff_size, 'unsigned char' )
P.packing.buff_size = 10
P.packing.buff = trick.sim_services.alloc_type( P.packing.buff_size, 'unsigned char' )
AC.packing.buff_size = 10
AC.packing.buff = trick.sim_services.alloc_type( AC.packing.buff_size, 'unsigned char' )
PC.packing.buff_size = 10
PC.packing.buff = trick.sim_services.alloc_type( PC.packing.buff_size, 'unsigned char' )


# We are taking advantage of the input file to specify a unique name for the
# sim-data name field for the P-side federate.
A.sim_data.name  = 'A.name.P-side'
P.sim_data.name  = 'P.name.P-side'
AC.sim_data.name = 'AC.name.P-side.cyclic'
PC.sim_data.name = 'PC.name.P-side.cyclic'


# =========================================================================
# Set up HLA interoperability.
# =========================================================================
# Show or hide the TrickHLA debug messages.
# Use Level-3 to show the ownership transfer debug messages.
THLA.federate.debug_level = trick.DEBUG_LEVEL_4_TRACE

# Specify the HLA base time units (default: trick.HLA_BASE_TIME_MICROSECONDS).
THLA.federate.set_HLA_base_time_units( trick.HLA_BASE_TIME_MICROSECONDS )

# Scale the Trick Time Tic value based on the HLA base time units.
THLA.federate.scale_trick_tics_to_base_time_units()

# Configure the CRC.
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
# Mak specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'
THLA.federate.lookahead_time = 0.0

# Configure the federate.
THLA.federate.name             = 'P-side-Federate'
THLA.federate.FOM_modules      = 'FOMs/S_FOMfile.xml,FOMs/TrickHLAFreezeInteraction.xml'
THLA.federate.federation_name  = 'SineWaveSim'
THLA.federate.time_regulating  = True
THLA.federate.time_constrained = True

# Configure ExecutionControl.
# Set the multiphase initialization synchronization points.
THLA.execution_control.multiphase_init_sync_points = 'Phase1, Phase2'
# Set the simulation timeline to be used for time computations.
THLA.execution_control.sim_timeline = THLA_INIT.sim_timeline
# Set the scenario timeline to be used for configuring federation freeze times.
THLA.execution_control.scenario_timeline = THLA_INIT.scenario_timeline


# The list of Federates known to be in our Federation. The simulation will
# wait for all Federates marked as required to join the Federation before
# continuing on.
THLA.federate.enable_known_feds      = True
THLA.federate.known_feds_count       = 2
THLA.federate.known_feds             = trick.sim_services.alloc_type( THLA.federate.known_feds_count, 'TrickHLA::KnownFederate' )
THLA.federate.known_feds[0].name     = 'A-side-Federate'
THLA.federate.known_feds[0].required = True
THLA.federate.known_feds[1].name     = 'P-side-Federate'
THLA.federate.known_feds[1].required = True


#---------------------------------------------
# Set up for simulation configuration.
#---------------------------------------------
THLA.simple_sim_config.owner        = 'P-side-Federate'
THLA.simple_sim_config.run_duration = run_duration


# The Federate has two objects, it publishes one and subscribes to another.
THLA.manager.obj_count = 4
THLA.manager.objects   = trick.sim_services.alloc_type( THLA.manager.obj_count, 'TrickHLA::Object' )


# Configure the object this federate subscribes to but will not create an
# HLA instance for.
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine'
THLA.manager.objects[0].create_HLA_instance = False
THLA.manager.objects[0].packing             = A.packing
THLA.manager.objects[0].attr_count          = 8
THLA.manager.objects[0].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[0].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[0].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[0].attributes[0].trick_name      = 'A.packing.time'
THLA.manager.objects[0].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[0].publish         = True
THLA.manager.objects[0].attributes[0].subscribe       = True
THLA.manager.objects[0].attributes[0].locally_owned   = False
THLA.manager.objects[0].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[0].attributes[1].trick_name      = 'A.packing.value'
THLA.manager.objects[0].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[1].publish         = True
THLA.manager.objects[0].attributes[1].subscribe       = True
THLA.manager.objects[0].attributes[1].locally_owned   = False
THLA.manager.objects[0].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[0].attributes[2].trick_name      = 'A.packing.dvdt'
THLA.manager.objects[0].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[2].publish         = True
THLA.manager.objects[0].attributes[2].subscribe       = True
THLA.manager.objects[0].attributes[2].locally_owned   = False
THLA.manager.objects[0].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[0].attributes[3].trick_name      = 'A.packing.phase_deg' # using packed data instead of 'A.sim_data.phase'
THLA.manager.objects[0].attributes[3].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[3].publish         = True
THLA.manager.objects[0].attributes[3].subscribe       = True
THLA.manager.objects[0].attributes[3].locally_owned   = False
THLA.manager.objects[0].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[0].attributes[4].trick_name      = 'A.packing.freq'
THLA.manager.objects[0].attributes[4].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[4].publish         = True
THLA.manager.objects[0].attributes[4].subscribe       = True
THLA.manager.objects[0].attributes[4].locally_owned   = False
THLA.manager.objects[0].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[0].attributes[5].trick_name      = 'A.packing.amp'
THLA.manager.objects[0].attributes[5].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[5].publish         = True
THLA.manager.objects[0].attributes[5].subscribe       = True
THLA.manager.objects[0].attributes[5].locally_owned   = False
THLA.manager.objects[0].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[0].attributes[6].trick_name      = 'A.packing.tol'
THLA.manager.objects[0].attributes[6].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[6].publish         = True
THLA.manager.objects[0].attributes[6].subscribe       = True
THLA.manager.objects[0].attributes[6].locally_owned   = False
THLA.manager.objects[0].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[0].attributes[7].trick_name      = 'A.packing.name'
THLA.manager.objects[0].attributes[7].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[0].attributes[7].publish         = True
THLA.manager.objects[0].attributes[7].subscribe       = True
THLA.manager.objects[0].attributes[7].locally_owned   = False
THLA.manager.objects[0].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING


# Configure the object this federate will create an HLA instance and
# publish data for.
THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine'
THLA.manager.objects[1].create_HLA_instance = True
THLA.manager.objects[1].packing             = P.packing
THLA.manager.objects[1].attr_count          = 8
THLA.manager.objects[1].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[1].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[1].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[1].attributes[0].trick_name      = 'P.packing.time'
THLA.manager.objects[1].attributes[0].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[0].publish         = True
THLA.manager.objects[1].attributes[0].locally_owned   = True
THLA.manager.objects[1].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[1].attributes[1].trick_name      = 'P.packing.value'
THLA.manager.objects[1].attributes[1].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[1].publish         = True
THLA.manager.objects[1].attributes[1].subscribe       = True
THLA.manager.objects[1].attributes[1].locally_owned   = True
THLA.manager.objects[1].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[1].attributes[2].trick_name      = 'P.packing.dvdt'
THLA.manager.objects[1].attributes[2].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[2].publish         = True
THLA.manager.objects[1].attributes[2].locally_owned   = True
THLA.manager.objects[1].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[1].attributes[3].trick_name      = 'P.packing.phase_deg' # using packed data instead of 'P.sim_data.phase'
THLA.manager.objects[1].attributes[3].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[3].publish         = True
THLA.manager.objects[1].attributes[3].locally_owned   = True
THLA.manager.objects[1].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[1].attributes[4].trick_name      = 'P.packing.freq'
THLA.manager.objects[1].attributes[4].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[4].publish         = True
THLA.manager.objects[1].attributes[4].locally_owned   = True
THLA.manager.objects[1].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[1].attributes[5].trick_name      = 'P.packing.amp'
THLA.manager.objects[1].attributes[5].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[5].publish         = True
THLA.manager.objects[1].attributes[5].locally_owned   = True
THLA.manager.objects[1].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[1].attributes[6].trick_name      = 'P.packing.tol'
THLA.manager.objects[1].attributes[6].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[6].publish         = True
THLA.manager.objects[1].attributes[6].locally_owned   = True
THLA.manager.objects[1].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[1].attributes[7].trick_name      = 'P.packing.name'
THLA.manager.objects[1].attributes[7].config          = trick.CONFIG_ZERO_LOOKAHEAD
THLA.manager.objects[1].attributes[7].publish         = True
THLA.manager.objects[1].attributes[7].locally_owned   = True
THLA.manager.objects[1].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING

# Cyclic analytic data.
THLA.manager.objects[2].FOM_name            = 'Test'
THLA.manager.objects[2].name                = 'A-side-Federate.Sine.cyclic'
THLA.manager.objects[2].create_HLA_instance = False
THLA.manager.objects[2].packing             = AC.packing
THLA.manager.objects[2].attr_count          = 8
THLA.manager.objects[2].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[2].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[2].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[2].attributes[0].trick_name      = 'AC.packing.time'
THLA.manager.objects[2].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[0].publish         = True
THLA.manager.objects[2].attributes[0].subscribe       = True
THLA.manager.objects[2].attributes[0].locally_owned   = False
THLA.manager.objects[2].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[2].attributes[1].trick_name      = 'AC.packing.value'
THLA.manager.objects[2].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[1].publish         = True
THLA.manager.objects[2].attributes[1].subscribe       = True
THLA.manager.objects[2].attributes[1].locally_owned   = False
THLA.manager.objects[2].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[2].attributes[2].trick_name      = 'AC.packing.dvdt'
THLA.manager.objects[2].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[2].publish         = True
THLA.manager.objects[2].attributes[2].subscribe       = True
THLA.manager.objects[2].attributes[2].locally_owned   = False
THLA.manager.objects[2].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[2].attributes[3].trick_name      = 'AC.packing.phase_deg' # using packed data instead of 'A.sim_data.phase'
THLA.manager.objects[2].attributes[3].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[3].publish         = True
THLA.manager.objects[2].attributes[3].subscribe       = True
THLA.manager.objects[2].attributes[3].locally_owned   = False
THLA.manager.objects[2].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[2].attributes[4].trick_name      = 'AC.packing.freq'
THLA.manager.objects[2].attributes[4].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[4].publish         = True
THLA.manager.objects[2].attributes[4].subscribe       = True
THLA.manager.objects[2].attributes[4].locally_owned   = False
THLA.manager.objects[2].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[2].attributes[5].trick_name      = 'AC.packing.amp'
THLA.manager.objects[2].attributes[5].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[5].publish         = True
THLA.manager.objects[2].attributes[5].subscribe       = True
THLA.manager.objects[2].attributes[5].locally_owned   = False
THLA.manager.objects[2].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[2].attributes[6].trick_name      = 'AC.packing.tol'
THLA.manager.objects[2].attributes[6].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[6].publish         = True
THLA.manager.objects[2].attributes[6].subscribe       = True
THLA.manager.objects[2].attributes[6].locally_owned   = False
THLA.manager.objects[2].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[2].attributes[7].trick_name      = 'AC.packing.name'
THLA.manager.objects[2].attributes[7].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[7].publish         = True
THLA.manager.objects[2].attributes[7].subscribe       = True
THLA.manager.objects[2].attributes[7].locally_owned   = False
THLA.manager.objects[2].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING

# Cyclic propagated data.
THLA.manager.objects[3].FOM_name            = 'Test'
THLA.manager.objects[3].name                = 'P-side-Federate.Sine.cyclic'
THLA.manager.objects[3].create_HLA_instance = True
THLA.manager.objects[3].packing             = PC.packing
THLA.manager.objects[3].attr_count          = 8
THLA.manager.objects[3].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[3].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[3].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[3].attributes[0].trick_name      = 'PC.packing.time'
THLA.manager.objects[3].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[0].publish         = True
THLA.manager.objects[3].attributes[0].locally_owned   = True
THLA.manager.objects[3].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[3].attributes[1].trick_name      = 'PC.packing.value'
THLA.manager.objects[3].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[1].publish         = True
THLA.manager.objects[3].attributes[1].subscribe       = True
THLA.manager.objects[3].attributes[1].locally_owned   = True
THLA.manager.objects[3].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[3].attributes[2].trick_name      = 'PC.packing.dvdt'
THLA.manager.objects[3].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[2].publish         = True
THLA.manager.objects[3].attributes[2].locally_owned   = True
THLA.manager.objects[3].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[3].attributes[3].trick_name      = 'PC.packing.phase_deg' # using packed data instead of 'P.sim_data.phase'
THLA.manager.objects[3].attributes[3].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[3].publish         = True
THLA.manager.objects[3].attributes[3].locally_owned   = True
THLA.manager.objects[3].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[3].attributes[4].trick_name      = 'PC.packing.freq'
THLA.manager.objects[3].attributes[4].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[4].publish         = True
THLA.manager.objects[3].attributes[4].locally_owned   = True
THLA.manager.objects[3].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[3].attributes[5].trick_name      = 'PC.packing.amp'
THLA.manager.objects[3].attributes[5].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[5].publish         = True
THLA.manager.objects[3].attributes[5].locally_owned   = True
THLA.manager.objects[3].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[3].attributes[6].trick_name      = 'PC.packing.tol'
THLA.manager.objects[3].attributes[6].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[6].publish         = True
THLA.manager.objects[3].attributes[6].locally_owned   = True
THLA.manager.objects[3].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[3].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[3].attributes[7].trick_name      = 'PC.packing.name'
THLA.manager.objects[3].attributes[7].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[3].attributes[7].publish         = True
THLA.manager.objects[3].attributes[7].locally_owned   = True
THLA.manager.objects[3].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING

#---------------------------------------------
# Set up simulation termination time.
#---------------------------------------------
trick.sim_services.exec_set_terminate_time( run_duration )
