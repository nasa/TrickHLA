# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */
#

#
# Define the command line options to configure the simulation.
#
def print_usage_message( ):
   
   global crc_host
   global crc_port
   global local_settings

   print(' ')
   print('Simulation Command Line Configuration Options:')
   print('  -h --help              : Print this help message.')
   print('  -r --crcHost [name]    : Name of RTI CRC Host, currently:', crc_host )
   print('  -p --crcPort [number]  : Port number for the RTI CRC, currently:', crc_port )
   print('  -s --settings [string] : RTI CRC local settings:', local_settings )
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.')
   return
# END: print_usage_message


def parse_command_line( ):
   
   global print_usage
   global crc_host
   global crc_port
   global local_settings
   global override_settings
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc):
            
      if ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')):
         print_usage = True
      
      elif ((str(argv[index]) == '-r') | (str(argv[index]) == '--crcHost')):
         index = index + 1
         if (index < argc):
            crc_host = str(argv[index])
         else:
            print('ERROR: Missing --crcHost [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-p') | (str(argv[index]) == '--crcPort')):
         index = index + 1
         if (index < argc):
            crc_port = int(str(argv[index]))
         else:
            print('ERROR: Missing --crcPort [port] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-s') | (str(argv[index]) == '--settings')):
         index = index + 1
         if (index < argc):
            local_settings = str(argv[index])
            override_settings = True
         else:
            print('ERROR: Missing --settings [string] argument.')
            print_usage = True
         
      else:
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
   
      index = index + 1
   
   return
# END: parse_command_line


# Global configuration variables

# Default: Don't show usage:
print_usage = False

# Default: CRC Host string:
crc_host = "localhost"

# Default: CRC port value.
crc_port = 8989

# Default: local settings string:
override_settings = False
local_settings = 'crcHost = ' + crc_host + '\n crcPort = ' + str(crc_port)

# Parse the command line.
parse_command_line()

# Check to see if setting were overridden.
# If not, then form the local_settings string since host and port may have been set.
if not override_settings:
   local_settings = 'crcHost = ' + crc_host + '\n crcPort = ' + str(crc_port)

if (print_usage == True):
   print_usage_message()


#trick setup
trick.sim_services.exec_set_trap_sigfpe(1)
simControlPanel = trick.SimControlPanel()
trick.add_external_application(simControlPanel)
trickView = trick.TrickView()
trick.add_external_application(trickView)
trickView.set_auto_open_file('TV_HLA_modelA.tv')
trick.real_time_enable()
trick.sim_services.exec_set_terminate_time(86400)
trick.exec_set_software_frame(0.1)
trick.exec_set_time_tic_value(100000000)
trick.TMM_reduced_checkpoint(False)
trick_mm.mm.set_expanded_arrays(True)
trick_sys.sched.set_enable_freeze(True)
#trick_sys.sched.set_freeze_command(True)

# Custom initial conditions:
cabinAtmo.modelConfigA.mImvFanOn = False
cabinAtmo.modelConfigA.mImvValveOpen = False
cabinAtmo.modelConfigA.mHatchOpen = False
cabinAtmo.modelConfigA.mMpevOpen = False
cabinAtmo.modelConfigA.mGrillValveOpen = False
cabinAtmo.modelConfigA.mCabin.mTemperature = 292.261
cabinAtmo.modelConfigA.mCabinMixture[0] = 0.84
cabinAtmo.modelConfigA.mCabinMixture[1] = 0.15
cabinAtmo.modelConfigA.mCabinMixture[2] = 0.005
cabinAtmo.modelConfigA.mCabinMixture[3] = 0.005
cabinAtmo.modelConfigA.mImvDuctMixture[0] = 1.0
cabinAtmo.modelConfigA.mImvDuctMixture[1] = 0.0
cabinAtmo.modelConfigA.mImvDuctMixture[2] = 0.0
cabinAtmo.modelConfigA.mImvDuctMixture[3] = 0.0
cabinAtmo.modelConfigA.mVestibuleMixture[0] = 0.0
cabinAtmo.modelConfigA.mVestibuleMixture[1] = 0.0
cabinAtmo.modelConfigA.mVestibuleMixture[2] = 0.0
cabinAtmo.modelConfigA.mVestibuleMixture[3] = 1.0

cabinAtmo.modelConfigB.mImvFanOn = False
cabinAtmo.modelConfigB.mImvValveOpen = False
cabinAtmo.modelConfigB.mHatchOpen = False
cabinAtmo.modelConfigB.mMpevOpen = False
cabinAtmo.modelConfigB.mGrillValveOpen = False
cabinAtmo.modelConfigB.mCabin.mVolume = 20.0
cabinAtmo.modelConfigB.mCabin.mIsIfMaster = False
cabinAtmo.modelConfigB.mImvDuct.mIsIfMaster = False
cabinAtmo.modelConfigB.mVestibule.mIsIfMaster = False
cabinAtmo.modelConfigB.mCabin.mPressure = 104325.0
cabinAtmo.modelConfigB.mCabin.mTemperature = 295.261
cabinAtmo.modelConfigB.mImvDuctMixture[0] = 1.0
cabinAtmo.modelConfigB.mImvDuctMixture[1] = 0.0
cabinAtmo.modelConfigB.mImvDuctMixture[2] = 0.0
cabinAtmo.modelConfigB.mImvDuctMixture[3] = 0.0
cabinAtmo.modelConfigB.mVestibuleMixture[0] = 0.0
cabinAtmo.modelConfigB.mVestibuleMixture[1] = 0.0
cabinAtmo.modelConfigB.mVestibuleMixture[2] = 0.0
cabinAtmo.modelConfigB.mVestibuleMixture[3] = 1.0

# Configure interfaces to use energy as specific enthalpy
#cabinAtmo.modelConfigA.mVestibule.mIsIfEnthalpy = True;
#cabinAtmo.modelConfigB.mVestibule.mIsIfEnthalpy = True;
#cabinAtmo.modelConfigA.mImvDuct.mIsIfEnthalpy = True;
#cabinAtmo.modelConfigB.mImvDuct.mIsIfEnthalpy = True;

# Demonstrate energy or temperature error caused by different specific heats in models
# These values are from the NIST reference and differ from the default model values.
# With these differences, and transferring energy as enthalpy (above), this causes a
# standing temperature difference across the interface, but conserves energy.  In
# contrast, transferring energy as temperature causes temperatures to match, but
# causes error in conservation of energy.
#cabinAtmo.modelConfigA.mCompoundCp[0] = 29.161
#cabinAtmo.modelConfigA.mCompoundCp[1] = 29.368
#cabinAtmo.modelConfigA.mCompoundCp[2] = 33.831
#cabinAtmo.modelConfigA.mCompoundCp[3] = 36.963

# Set up lag amount in the lag ring buffers
# Loop latency will be 2 + (2 * mDelayFrames)
cabinAtmo.lagBufferVest.mDelayFrames = 0
cabinAtmo.lagBufferImv.mDelayFrames = 0

# Establish reference conservation parameter values at time = 5 second, to give
# HLA federations time to join and send data across.
trick.add_read(5.0, """cabinAtmo.conservation.setReference = True""")

################################################################################
# HLA Setup
################################################################################
# For HLA, disable the internal buffer connecting models A & B
cabinAtmo.exchangeData = False
cabinAtmo.conservation.isBsideHla = True

# When using HLA, modelA in this sim is tied to modelB in the other federate,
# not to the modelB in this sim, so the conservation checks model doesn't work.
#trick.add_read(1.0, """cabinAtmo.conservation.setReference = True""")

# Configure the CRC, the default is the local host but can be overridden
# with command line arguments --crcHost and --crcPort or --settings.
#THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
THLA.federate.local_settings = local_settings

# Configure the federate
Federation_name = "FLUID_DIST_IF_DEMO"
Federate_name   = "FED_1"

# THLA configuration
from Modified_data.TrickHLA.TrickHLAFederateConfig import *
federate = TrickHLAFederateConfig( thla_federate        = THLA.federate,
                                   thla_manager         = THLA.manager,
                                   thla_control         = THLA.execution_control,
                                   thla_config          = THLA.simple_sim_config,
                                   thla_federation_name = Federation_name,
                                   thla_federate_name   = Federate_name,
                                   thla_enabled         = True )

# Add required federates.
federate.add_known_federate( True, "FED_1")
federate.add_known_federate( True, "FED_2")

# Set time management parameters
federate.set_HLA_base_time_units( trick.HLA_BASE_TIME_10_NANOSECONDS )
federate.set_lookahead_time( 0.1 )
federate.set_least_common_time_step( 0.1 )
federate.set_time_padding( 3 * 0.1 )
federate.set_time_regulating( True )
federate.set_time_constrained( True )

# Add the sim configuration FOM.
federate.add_FOM_module( 'FOMs/DistIf/SimpleSimConfigFOM.xml' )

# Configure the Fluid Distributed Interface messages
federate.add_FOM_module( 'FOMs/DistIf/FluidDistIfFOM.xml' )
from Modified_data.DistIf.FluidDistIfObjectConfig import *

# Interfaces between modelA in this federate (FED_1) to modelB in the other FED_2:
data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_Vestibule_1A_to_2B',
   bus_name           = 'cabinAtmo.modelA.mVestibule.mIf',
   isBusA             = True,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_Vestibule_2B_to_1A',
   bus_name           = 'cabinAtmo.modelA.mVestibule.mIf',
   isBusA             = False,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_IMV_1A_to_2B',
   bus_name           = 'cabinAtmo.modelA.mImvDuct.mIf',
   isBusA             = True,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_IMV_2B_to_1A',
   bus_name           = 'cabinAtmo.modelA.mImvDuct.mIf',
   isBusA             = False,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

# Interfaces between modelB in this federate (FED_1) to modelA in the other FED_2:
data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_Vestibule_2A_to_1B',
   bus_name           = 'cabinAtmo.modelB.mVestibule.mIf',
   isBusA             = True,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_Vestibule_1B_to_2A',
   bus_name           = 'cabinAtmo.modelB.mVestibule.mIf',
   isBusA             = False,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_IMV_2A_to_1B',
   bus_name           = 'cabinAtmo.modelB.mImvDuct.mIf',
   isBusA             = True,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

data_obj = FluidDistIfAToBObjectConfig(
   thla_federate_name = 'FluidDistIf_IMV_1B_to_2A',
   bus_name           = 'cabinAtmo.modelB.mImvDuct.mIf',
   isBusA             = False,
   FOM_type           = 'FluidDistIfDataBase.FluidDistIfData_6_4' )
federate.add_fed_object( data_obj )

# Configure the Conservation parameters HLA data messages
from Modified_data.DistIf.ConserveParamsObjectConfig import *
data_obj = ConserveParamsObjectConfig(
   thla_federate_name = 'ConservationData_1_to_2',
   model_name         = 'cabinAtmo.modelB.mConserveParams',
   isOwner            = True,
   FOM_type           = 'ConservationParams' )
federate.add_fed_object( data_obj )

data_obj = ConserveParamsObjectConfig(
   thla_federate_name = 'ConservationData_2_to_1',
   model_name         = 'cabinAtmo.conservation.modelBConserveParams',
   isOwner            = False,
   FOM_type           = 'ConservationParams' )
federate.add_fed_object( data_obj )

# After all configuration is defined, initialize the federate
federate.initialize()
