import os
import sys

#---------------------------------------------
# Set up path so the sim modules can be found.
#---------------------------------------------
# Find the TrickHLA home location and append the path.
trickhla_home = os.environ.get( "TRICKHLA_HOME" )
if trickhla_home is None:
   sys.exit( '\033[91m'+'Environment variable TRICKHLA_HOME is not defined!'+'\033[0m\n' )
else:
   if os.path.isdir( trickhla_home ) is False:
      sys.exit( '\033[91m'+'TRICKHLA_HOME not found: '+trickhla_home+'\033[0m\n' )

# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trickhla_home not in sys.path :
   sys.path.append( trickhla_home )

#---------------------------------------------------------------------------
# Set up the Trick executive parameters
#---------------------------------------------------------------------------
# Import and configure the TrickHLA base Simulation Configuration class.
from TrickHLA_data.TrickHLA.TrickHLASimConfig import *
encoder_sim_config = TrickHLASimConfig( 'encoder' )
encoder_sim_config.realtime( software_frame_time = 0.1 )
encoder_sim_config.sim_control_panel()
encoder_sim_config.start_in_freeze()


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
THLA.federate.local_settings = 'crcHost = localhost\:8989'
# MAK specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'

# Configure the federate.
THLA.federate.name             = 'Sender'
THLA.federate.FOM_modules      = 'FOMs/TrickHLA/SimpleSimConfig.xml,FOMs/SpaceFOM/SISO_SpaceFOM_datatypes.xml,FOMs/Encoders_Test_FOM.xml'
THLA.federate.federation_name  = 'Encoders_Test'

# Set the lookahead value, time constrained, and time regulating states.
THLA.federate.setup_time_management( 0.100, True, True )

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
fedSender = trick.KnownFederate()
fedSender.name     = 'Sender'
fedSender.type     = 'Sender'
fedSender.required = True
fedReceiver = trick.KnownFederate()
fedReceiver.name     = 'Receiver'
fedReceiver.type     = 'Receiver'
fedReceiver.required = True
THLA.federate.enable_known_feds = True
THLA.federate.known_federates.push_back( fedSender )
THLA.federate.known_federates.push_back( fedReceiver )


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

