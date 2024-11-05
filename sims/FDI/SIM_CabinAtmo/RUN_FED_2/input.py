# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */
#
#trick setup
trick.sim_services.exec_set_trap_sigfpe(1)
simControlPanel = trick.SimControlPanel()
trick.add_external_application(simControlPanel)
trickView = trick.TrickView()
trick.add_external_application(trickView)
trickView.set_auto_open_file('TV_HLA_modelB.tv')
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

# When using HLA, modelB in this sim is tied to modelA in the other federate,
# not to the modelA in this sim, so the conservation checks model doesn't work.
#trick.add_read(1.0, """cabinAtmo.conservation.setReference = True""")

# Configure the CRC, assuming we're in the Trick Lab
THLA.federate.local_settings = "crcHost = js-er7-rti.jsc.nasa.gov\n crcPort = 8989"

# Configure the federate
Federation_name = "FLUID_DIST_IF_DEMO"
Federate_name   = "FED_2"

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

# Interfaces between modelB in this federate (FED_2) to modelA in the other FED_1:
data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_1A_to_2B',
                                       'cabinAtmo.modelB.mVestibule.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_2B_to_1A',
                                       'cabinAtmo.modelB.mVestibule.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_1A_to_2B',
                                       'cabinAtmo.modelB.mImvDuct.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_2B_to_1A',
                                       'cabinAtmo.modelB.mImvDuct.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

# Interfaces between modelA in this federate (FED_2) to modelB in the other FED_1:
data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_2A_to_1B',
                                       'cabinAtmo.modelA.mVestibule.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_Vestibule_1B_to_2A',
                                       'cabinAtmo.modelA.mVestibule.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_2A_to_1B',
                                       'cabinAtmo.modelA.mImvDuct.mIf',
                                       False,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

data_obj = FluidDistIfAToBObjectConfig('FluidDistIf_IMV_1B_to_2A',
                                       'cabinAtmo.modelA.mImvDuct.mIf',
                                       True,
                                       'FluidDistIfDataBase.FluidDistIfData_6_4')
federate.add_fed_object(data_obj)

# Configure the Conservation parameters HLA data messages
from Modified_data.FDI.ConserveParamsObjectConfig import *
data_obj = ConserveParamsObjectConfig('ConservationData_2_to_1',
                                      'cabinAtmo.modelB.mConserveParams',
                                      True,
                                      'ConservationParams')
federate.add_fed_object(data_obj)

data_obj = ConserveParamsObjectConfig('ConservationData_1_to_2',
                                      'cabinAtmo.conservation.modelBConserveParams',
                                      False,
                                      'ConservationParams')
federate.add_fed_object(data_obj)

# After all configuration is defined, initialize the federate
federate.initialize()
