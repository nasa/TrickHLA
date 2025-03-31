


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
express_frame = frames_list[3]
native_frame = frames_list[4]

#---------------------------------------------------------------------------
# Set up the frame states.
#---------------------------------------------------------------------------
# Earth inertial
earth_inertial_frame.frame.data.name = frames_list[3]
earth_inertial_frame.frame.data.parent_name = frames_list[2]
earth_inertial_frame.frame.data.state.pos = [1.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
earth_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "earth_inertial_frame.print_frame_state", 1, True )

# Earth fixed
earth_fixed_frame.frame.data.name = frames_list[4]
earth_fixed_frame.frame.data.parent_name = frames_list[3]
earth_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 45.0] )
earth_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.001 * deg2rad]
# Set the environmental acceleration values.
earth_fixed_frame.frame.accel_env = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.ang_accel_env = [0.0, 0.0, 0.0000 * deg2rad]
# Control print job.
trick.exec_set_job_onoff( "earth_fixed_frame.print_frame_state", 1, True )


#---------------------------------------------------------------------------
# Set up the dynamics parameters for the vehicle test entity.
#---------------------------------------------------------------------------
vehicle.entity.pe_data.parent_frame = native_frame

# Initial translational state.
vehicle.entity.pe_data.state.pos = [ 1.0, 0.0, 0.0 ]
vehicle.entity.pe_data.state.vel = [ 0.0, 0.0, 0.0 ]

# Initial rotational state.
vehicle.entity.pe_data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
vehicle.entity.pe_data.state.ang_vel = [ 0.0, 0.0, 0.0 ]

#
# Basic mass properties.
#
vehicle_mass = 100.0
vehicle.entity.pe_data.cm        = [0.0, 0.0, 0.0]
vehicle.entity.de_data.mass      = vehicle_mass
vehicle.entity.de_data.mass_rate = 0.0
vehicle.entity.pe_data.body_wrt_struct.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )


