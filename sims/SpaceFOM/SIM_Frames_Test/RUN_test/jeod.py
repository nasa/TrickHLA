


#frames_list = [
#0   'SolarSystemBarycentricInertial',
#1   'SunCentricInertial',
#2   'EarthMoonBarycentricInertial',
#3   'EarthCentricInertial',
#4   'EarthCentricFixed',
#5   'MoonCentricInertial',
#6   'MoonCentricFixed',
#7   'MarsCentricInertial',
#8   'MarsCentricFixed'
#   ]

# Set the default frames.
express_frame = frames_list[6]
native_frame = frames_list[5]

#---------------------------------------------------------------------------
# Set up the frame states.
#---------------------------------------------------------------------------
# Moon inertial
moon_inertial_frame.frame.data.name = frames_list[5]
moon_inertial_frame.frame.data.parent_name = frames_list[2]
moon_inertial_frame.frame.data.state.pos = [331640799.817171, -208675093.057267, -80107944.1063125]
moon_inertial_frame.frame.data.state.vel = [522.402340923877, 704.817797190995, 398.454770779652]
moon_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
moon_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "moon_inertial_frame.print_frame_state", 1, True )

# Earth fixed
moon_fixed_frame.frame.data.name = frames_list[6]
moon_fixed_frame.frame.data.parent_name = frames_list[5]
moon_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.att.scalar = -0.270119215788571
moon_fixed_frame.frame.data.state.att.vector = [0.0634622113468803, -0.183899591907175, 0.94296823758232]
moon_fixed_frame.frame.data.state.ang_vel = [-1.05728117046177e-09, 4.90210558152287e-10, 2.6618546609035e-06]
# Set the environmental acceleration values.
moon_fixed_frame.frame.accel_env = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.ang_accel_env = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "moon_fixed_frame.print_frame_state", 1, True )


#---------------------------------------------------------------------------
# Set up the dynamics parameters for the vehicle test entity.
#---------------------------------------------------------------------------
vehicle.entity.pe_data.parent_frame = native_frame

# Initial translational state.
vehicle.entity.pe_data.state.pos = [ 1296944.012, -1060824.45, 2522289.146]
vehicle.entity.pe_data.state.vel = [ 0.0, 0.0, 0.0 ]

# Initial rotational state.
#vehicle.entity.pe_data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
vehicle.entity.pe_data.state.att.scalar = 6.12323399573677e-17
vehicle.entity.pe_data.state.att.vector = [0.0, -1.0, 0.0]
vehicle.entity.pe_data.state.ang_vel = [ 0.0, 0.0, 0.0 ]

#
# Basic mass properties.
#
vehicle_mass = 100.0
vehicle.entity.pe_data.cm        = [0.0, 0.0, 0.0]
vehicle.entity.de_data.mass      = vehicle_mass
vehicle.entity.de_data.mass_rate = 0.0
vehicle.entity.pe_data.body_wrt_struct.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )


