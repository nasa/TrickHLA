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
THLA.federate.name             = 'Ball1-Federate'
THLA.federate.FOM_modules      = 'FOMs/SimpleSimConfig.xml,FOMs/BallFOM.xml'
THLA.federate.federation_name  = 'BallFederation'
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
THLA.federate.known_feds_count       = 1
THLA.federate.known_feds             = trick.sim_services.alloc_type( THLA.federate.known_feds_count, 'TrickHLA::KnownFederate' )
THLA.federate.known_feds[0].name     = 'Ball1-Federate'
THLA.federate.known_feds[0].required = True
#THLA.federate.known_feds[1].name     = 'Ball2-Federate'
#THLA.federate.known_feds[1].required = True


#---------------------------------------------
# Set up for simulation configuration.
#---------------------------------------------
THLA.simple_sim_config.owner        = 'Ball1-Federate'
THLA.simple_sim_config.run_duration = run_duration