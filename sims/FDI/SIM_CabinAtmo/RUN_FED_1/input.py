# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */
#

#
# Define the command line options to configure the simulation.
#
def print_usage_message( ):

   print(' ')
   print('TrickHLA SpaceFOM JEOD Master Simulation Command Line Configuration Options:')
   print('  -h --help            : Print this help message.')
   print('  -r --crcHost [name]  : Name of RTI CRC Host, currently:', crcHost )
   print('  -p --crcPort [number]: Port number for the RTI CRC, currently:', crcPort )
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.')
   return
# END: print_usage_message


def parse_command_line( ) :
   
   global print_usage
   global crc_host
   global crc_port
   global local_settings
   global run_duration
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc) :
            
      if ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')) :
         print_usage = True
      
      elif ((str(argv[index]) == '-r') | (str(argv[index]) == '--crcHost')) :
         index = index + 1
         if (index < argc) :
            crc_host = str(argv[index])
         else :
            print('ERROR: Missing --crcHost [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-p') | (str(argv[index]) == '--crcPort')) :
         index = index + 1
         if (index < argc) :
            crc_port = int(str(argv[index]))
         else :
            print('ERROR: Missing -crcPort [port] argument.')
            print_usage = True
         
      else :
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
   
      index = index + 1
   
   return
# END: parse_command_line


# Global configuration variables

# Default: Don't show usage.
print_usage = False

# Default: CRC Host string
crc_host = "localhost"

# Default: CRC port value.
crc_port = 8989

# Parse the command line.
parse_command_line()

# Form the RTI local settings string.
local_settings = 'crcHost = ' + crc_host + '\n crcPort = ' + str(crc_port)

if (print_usage == True) :
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
# with command line arguments --crcHost and --crcPort.
#THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
THLA.federate.local_settings = local_settings

# Configure the federate
Federation_name = "FLUID_DIST_IF_DEMO"
Federate_name   = "FED_1"

# THLA configuration
from Modified_data.TrickHLA.TrickHLAFederateConfig import *
federate = TrickHLAFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.simple_sim_config,
                                   Federation_name,
                                   Federate_name,
                                   True )

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
federate.add_FOM_module( 'FOMs/FDI/SimpleSimConfigFOM.xml' )

# Configure the Fluid Distributed Interface messages
federate.add_FOM_module( 'FOMs/FDI/FluidDistIfFOM.xml' )
from Modified_data.FDI.FluidDistIfObjectConfig import *

# Interfaces between modelA in this federate (FED_1) to modelB in the other FED_2:
data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_1A_to_2B',
                                       'cabinAtmo.modelA.mVestibule.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_2B_to_1A',
                                       'cabinAtmo.modelA.mVestibule.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_1A_to_2B',
                                       'cabinAtmo.modelA.mImvDuct.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_2B_to_1A',
                                       'cabinAtmo.modelA.mImvDuct.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

# Interfaces between modelB in this federate (FED_1) to modelA in the other FED_2:
data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_2A_to_1B',
                                       'cabinAtmo.modelB.mVestibule.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_1B_to_2A',
                                       'cabinAtmo.modelB.mVestibule.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_2A_to_1B',
                                       'cabinAtmo.modelB.mImvDuct.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_1B_to_2A',
                                       'cabinAtmo.modelB.mImvDuct.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

# Configure the Conservation parameters HLA data messages
from Modified_data.FDI.ConserveParamsObjectConfig import *
data_obj = ConserveParamsObjectConfig('ConservationData_1_to_2',
                                      'cabinAtmo.modelB.mConserveParams',
                                      True,
                                      'ConservationParams')
federate.add_fed_object(data_obj)

data_obj = ConserveParamsObjectConfig('ConservationData_2_to_1',
                                      'cabinAtmo.conservation.modelBConserveParams',
                                      False,
                                      'ConservationParams')
federate.add_fed_object(data_obj)

# After all configuration is defined, initialize the federate
federate.initialize()
