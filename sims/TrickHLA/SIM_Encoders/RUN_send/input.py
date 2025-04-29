#---------------------------------------------------------------------------
# Set up the Trick executive parameters
#---------------------------------------------------------------------------
trick.frame_log_on()
trick.real_time_enable()
trick.exec_set_software_frame(0.1)
trick.itimer_enable()

trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)

trick.sim_control_panel_set_enabled( True )
#trick.var_server_set_port( 7000 )


#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
run_duration = 0.2


# =========================================================================
# Set up HLA interoperability.
# =========================================================================
# Show or hide the TrickHLA debug messages.
# Use Level-3 to show the ownership transfer debug messages.
THLA.federate.debug_level = trick.DEBUG_LEVEL_3_TRACE


# Configure the CRC.
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
# MAK specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'
THLA.federate.lookahead_time = 0.100

# Configure the federate.
THLA.federate.name             = 'Sender'
THLA.federate.FOM_modules      = 'FOMs/TrickHLA/SimpleSimConfig.xml,FOMs/SpaceFOM/SISO_SpaceFOM_datatypes.xml,FOMs/Encoders_Test_FOM.xml'
THLA.federate.federation_name  = 'Encoders_Test'
THLA.federate.time_regulating  = True
THLA.federate.time_constrained = True

# Configure ExecutionControl.
# Set the multiphase initialization synchronization points.
#THLA.execution_control.multiphase_init_sync_points = 'Phase1, Phase2'

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
THLA.federate.known_feds[0].name     = 'Sender'
THLA.federate.known_feds[0].required = True
THLA.federate.known_feds[1].name     = 'Receiver'
THLA.federate.known_feds[1].required = True


#---------------------------------------------
# Set up simple simulation configuration.
#---------------------------------------------
THLA.simple_sim_config.owner        = 'Sender'
THLA.simple_sim_config.run_duration = run_duration


#---------------------------------------------------------------------------
# Set the Quaternion test information.
#---------------------------------------------------------------------------
quat_encoder_test.packing.debug = True
quat_encoder_test.packing.publish()

quat_encoder_test.data.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [45.0, 30.0, 10.0] )


#---------------------------------------------------------------------------
# Set the SpaceTimeCoordinateState test information.
#---------------------------------------------------------------------------
stc_encoder_test.packing.debug = True
stc_encoder_test.packing.publish()

stc_encoder_test.data.pos = [ 1.0, 2.0, 3.0 ]
stc_encoder_test.data.vel = [ 0.1, 0.2, 0.3 ]
stc_encoder_test.data.att.scalar = 1.0
stc_encoder_test.data.att.vector = [ 0.0, 0.0, 0.0 ]
stc_encoder_test.data.ang_vel = [ 0.01, 0.02, 0.03 ]


#---------------------------------------------------------------------------
# Manage simulation termination time.
#---------------------------------------------------------------------------
if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

