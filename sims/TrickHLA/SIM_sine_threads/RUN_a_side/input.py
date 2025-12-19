#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe(True)
#trick.checkpoint_pre_init(1)
trick.checkpoint_post_init(1)
#trick.add_read(0.0 , '''trick.checkpoint('checkpoint')''')

# Realtime setup
exec(open( "Modified_data/trick/realtime.py" ).read())

# Trick config
trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)
trick.sim_control_panel_set_enabled(True)
trick.exec_set_stack_trace(False)

trick.exec_set_thread_process_type( 1, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 1, 0.250 )

trick.exec_set_thread_process_type( 2, trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 2, 0.250 )


run_duration = 15.0

#---------------------------------------------
# Set up data to record.
#---------------------------------------------
exec(open( "Log_data/log_sine_states.py" ).read())
log_sine_states( 'A', 0.250 )
log_sine_states( 'P', 0.250 )


#---------------------------------------------
# Set up the initial Sine states
#---------------------------------------------
exec(open( "Modified_data/sine_init.py" ).read())


# Example of a 1-dimensional dynamic array.
A.packing.buff_size = 10
A.packing.buff = trick.sim_services.alloc_type( A.packing.buff_size, 'unsigned char' )
P.packing.buff_size = 10
P.packing.buff = trick.sim_services.alloc_type( P.packing.buff_size, 'unsigned char' )

# We are taking advantage of the input file to specify a unique name for the
# sim-data name field for the A-side federate.
A.sim_data.name = 'A.sim_data.name.A-side'
P.sim_data.name = 'P.sim_data.name.A-side'

# We are taking advantage of the input file to specify a unique name and
# message for the A-side federate interaction handler.
A.interaction_handler.name = 'A-side: A.interaction_handler.name'
P.interaction_handler.name = 'A-side: P.interaction_handler.name'

A.interaction_handler.message = 'A-side: A.interaction_handler.message'
P.interaction_handler.message = 'A-side: P.interaction_handler.message'


# Send an interaction.
trick.add_read(12.0 , '''A.interaction_handler.send_sine_interaction( THLA_INIT.scenario_timeline.get_time() )''')


# =========================================================================
# Set up HLA interoperability.
# =========================================================================
# Show or hide the TrickHLA debug messages.
# Use Level-3 to show the ownership transfer debug messages.
THLA.federate.debug_level = trick.DEBUG_LEVEL_5_TRACE


# Configure the CRC.
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
# MAK specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'

THLA.federate.lookahead_time = 0.250

# Specify the HLA base time unit (default: trick.HLA_BASE_TIME_MICROSECONDS)
# and scale the Trick time tics value.
THLA.federate.set_HLA_base_time_unit_and_scale_trick_tics( trick.HLA_BASE_TIME_MICROSECONDS )

# Disable Trick child thread IDs associated to TrickHLA in the S_define file
# as a comma separated list.
#THLA.federate.disable_trick_child_thread_associations( '1, 2' )


# Configure the federate.
THLA.federate.name             = 'A-side-Federate'
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
THLA.simple_sim_config.owner        = 'A-side-Federate'
THLA.simple_sim_config.run_duration = run_duration


# TrickHLA Interactions and Parameters.
THLA.manager.inter_count  = 1
THLA.manager.interactions = trick.sim_services.alloc_type( THLA.manager.inter_count, 'TrickHLA::Interaction' )

THLA.manager.interactions[0].FOM_name    = 'Communication'
THLA.manager.interactions[0].publish     = True
THLA.manager.interactions[0].subscribe   = False
THLA.manager.interactions[0].handler     = A.interaction_handler
THLA.manager.interactions[0].param_count = 3
THLA.manager.interactions[0].parameters  = trick.sim_services.alloc_type( THLA.manager.interactions[0].param_count, 'TrickHLA::Parameter' )

THLA.manager.interactions[0].parameters[0].FOM_name     = 'Message'
THLA.manager.interactions[0].parameters[0].trick_name   = 'A.interaction_handler.message'
THLA.manager.interactions[0].parameters[0].rti_encoding = trick.ENCODING_UNICODE_STRING

THLA.manager.interactions[0].parameters[1].FOM_name     = 'time'
THLA.manager.interactions[0].parameters[1].trick_name   = 'A.interaction_handler.time'
THLA.manager.interactions[0].parameters[1].rti_encoding = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.interactions[0].parameters[2].FOM_name     = 'year'
THLA.manager.interactions[0].parameters[2].trick_name   = 'A.interaction_handler.year'
THLA.manager.interactions[0].parameters[2].rti_encoding = trick.ENCODING_LITTLE_ENDIAN


# The Federate has two objects, it publishes one and subscribes to another.
THLA.manager.obj_count = 2
THLA.manager.objects   = trick.sim_services.alloc_type( THLA.manager.obj_count, 'TrickHLA::Object' )

# Configure the object this federate will create an HLA instance and
# publish data for.
THLA.manager.objects[0].FOM_name            = 'Test'
THLA.manager.objects[0].name                = 'A-side-Federate.Sine'
THLA.manager.objects[0].create_HLA_instance = True
THLA.manager.objects[0].thread_ids          = "1"
THLA.manager.objects[0].packing             = A.packing
THLA.manager.objects[0].lag_comp            = A.lag_compensation
THLA.manager.objects[0].lag_comp_type       = trick.LAG_COMPENSATION_NONE
#THLA.manager.objects[0].ownership           = A.ownership_handler
THLA.manager.objects[0].deleted             = A.obj_deleted_callback
THLA.manager.objects[0].attr_count          = 8
THLA.manager.objects[0].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[0].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[0].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[0].attributes[0].trick_name      = 'A.packing.time'
THLA.manager.objects[0].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[0].publish         = True
THLA.manager.objects[0].attributes[0].locally_owned   = True
THLA.manager.objects[0].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[0].attributes[1].trick_name      = 'A.packing.value'
THLA.manager.objects[0].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[1].publish         = True
THLA.manager.objects[0].attributes[1].subscribe       = True
THLA.manager.objects[0].attributes[1].locally_owned   = True
THLA.manager.objects[0].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[0].attributes[2].trick_name      = 'A.packing.dvdt'
THLA.manager.objects[0].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[2].publish         = True
THLA.manager.objects[0].attributes[2].locally_owned   = True
THLA.manager.objects[0].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[0].attributes[3].trick_name      = 'A.packing.phase_deg' # using packed data instead of 'A.sim_data.phase'
THLA.manager.objects[0].attributes[3].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[3].publish         = True
THLA.manager.objects[0].attributes[3].locally_owned   = True
THLA.manager.objects[0].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[0].attributes[4].trick_name      = 'A.packing.freq'
THLA.manager.objects[0].attributes[4].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[4].publish         = True
THLA.manager.objects[0].attributes[4].locally_owned   = True
THLA.manager.objects[0].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[0].attributes[5].trick_name      = 'A.packing.amp'
THLA.manager.objects[0].attributes[5].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[5].publish         = True
THLA.manager.objects[0].attributes[5].locally_owned   = True
THLA.manager.objects[0].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[0].attributes[6].trick_name      = 'A.packing.tol'
THLA.manager.objects[0].attributes[6].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[6].publish         = True
THLA.manager.objects[0].attributes[6].locally_owned   = True
THLA.manager.objects[0].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[0].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[0].attributes[7].trick_name      = 'A.packing.name'
THLA.manager.objects[0].attributes[7].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[0].attributes[7].publish         = True
THLA.manager.objects[0].attributes[7].locally_owned   = True
THLA.manager.objects[0].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING

# Configure the object this federate subscribes to but will not create an
# HLA instance for.
THLA.manager.objects[1].FOM_name            = 'Test'
THLA.manager.objects[1].name                = 'P-side-Federate.Sine'
THLA.manager.objects[1].create_HLA_instance = False
THLA.manager.objects[1].thread_ids          = "2"
THLA.manager.objects[1].packing             = P.packing
THLA.manager.objects[1].lag_comp            = P.lag_compensation
THLA.manager.objects[1].lag_comp_type       = trick.LAG_COMPENSATION_NONE
THLA.manager.objects[1].deleted             = P.obj_deleted_callback
THLA.manager.objects[1].attr_count          = 8
THLA.manager.objects[1].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[1].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[1].attributes[0].FOM_name        = 'Time'
THLA.manager.objects[1].attributes[0].trick_name      = 'P.packing.time'
THLA.manager.objects[1].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[0].subscribe       = True
THLA.manager.objects[1].attributes[0].locally_owned   = False
THLA.manager.objects[1].attributes[0].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[1].FOM_name        = 'Value'
THLA.manager.objects[1].attributes[1].trick_name      = 'P.packing.value'
THLA.manager.objects[1].attributes[1].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[1].publish         = True
THLA.manager.objects[1].attributes[1].subscribe       = True
THLA.manager.objects[1].attributes[1].locally_owned   = False
THLA.manager.objects[1].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[2].FOM_name        = 'dvdt'
THLA.manager.objects[1].attributes[2].trick_name      = 'P.packing.dvdt'
THLA.manager.objects[1].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[2].publish         = True
THLA.manager.objects[1].attributes[2].subscribe       = True
THLA.manager.objects[1].attributes[2].locally_owned   = False
THLA.manager.objects[1].attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[3].FOM_name        = 'Phase'
THLA.manager.objects[1].attributes[3].trick_name      = 'P.packing.phase_deg' # using packed data instead of 'P.sim_data.phase'
THLA.manager.objects[1].attributes[3].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[3].subscribe       = True
THLA.manager.objects[1].attributes[3].locally_owned   = False
THLA.manager.objects[1].attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[4].FOM_name        = 'Frequency'
THLA.manager.objects[1].attributes[4].trick_name      = 'P.packing.freq'
THLA.manager.objects[1].attributes[4].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[4].subscribe       = True
THLA.manager.objects[1].attributes[4].locally_owned   = False
THLA.manager.objects[1].attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[5].FOM_name        = 'Amplitude'
THLA.manager.objects[1].attributes[5].trick_name      = 'P.packing.amp'
THLA.manager.objects[1].attributes[5].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[5].subscribe       = True
THLA.manager.objects[1].attributes[5].locally_owned   = False
THLA.manager.objects[1].attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[6].FOM_name        = 'Tolerance'
THLA.manager.objects[1].attributes[6].trick_name      = 'P.packing.tol'
THLA.manager.objects[1].attributes[6].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[6].subscribe       = True
THLA.manager.objects[1].attributes[6].locally_owned   = False
THLA.manager.objects[1].attributes[6].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[1].attributes[7].FOM_name        = 'Name'
THLA.manager.objects[1].attributes[7].trick_name      = 'P.packing.name'
THLA.manager.objects[1].attributes[7].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
THLA.manager.objects[1].attributes[7].subscribe       = True
THLA.manager.objects[1].attributes[7].locally_owned   = False
THLA.manager.objects[1].attributes[7].rti_encoding    = trick.ENCODING_UNICODE_STRING


#---------------------------------------------
# Set up simulation termination time.
#---------------------------------------------
trick.sim_services.exec_set_terminate_time( run_duration )
