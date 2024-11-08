# @copyright Copyright 2024 United States Government as represented by the Administrator of the
#            National Aeronautics and Space Administration.  All Rights Reserved. */
#
#trick setup
trick.sim_services.exec_set_trap_sigfpe(1)
simControlPanel = trick.SimControlPanel()
trick.add_external_application(simControlPanel)
trickView = trick.TrickView()
trick.add_external_application(trickView)
trickView.set_auto_open_file('TV_standalone.tv')
trick.real_time_enable()
trick.sim_services.exec_set_terminate_time(86400)
trick.exec_set_software_frame(0.01)
trick.TMM_reduced_checkpoint(False)
trick_mm.mm.set_expanded_arrays(True)
trick_sys.sched.set_enable_freeze(True)
trick_sys.sched.set_freeze_command(True)

# Turn off the HLA sim objects since this input file is for no HLA:
trick.exec_set_sim_object_onoff( 'THLA', False )

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

# Establish reference conservation parameter values at time = 1 second
trick.add_read(1.0, """cabinAtmo.conservation.setReference = True""")
