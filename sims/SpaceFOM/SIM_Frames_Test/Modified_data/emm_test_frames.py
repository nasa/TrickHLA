

# Conversions
import math
deg2rad = math.pi / 180.0

# Create reference frame set.
frames_list = [
   'SolarSystemBarycentricInertial',
   'SunCentricInertial',
   'EarthMoonBarycentricInertial',
   'EarthCentricInertial',
   'EarthCentricFixed',
   'MoonCentricInertial',
   'MoonCentricFixed',
   'MarsCentricInertial',
   'MarsCentricFixed'
   ]


#---------------------------------------------------------------------------
# Set up the frame states.
#---------------------------------------------------------------------------
# Solar system barycenter (Not propagated and ALWAYS zero.)
ssbary_frame.debug_frame_state = verbose
ssbary_frame.frame.data.name = frames_list[0]
ssbary_frame.frame.data.parent_name = ""
ssbary_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
ssbary_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
ssbary_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
ssbary_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

#
# Sun
#
sun_frame.debug_frame_state = verbose
sun_frame.frame.data.name = frames_list[1]
sun_frame.frame.data.parent_name = frames_list[0]
sun_frame.frame.data.state.pos = [0.0, 10.0, 0.0]
sun_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
sun_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
sun_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

#
# Earth-Moon system
#
embary_frame.debug_frame_state = verbose
embary_frame.frame.data.name = frames_list[2]
embary_frame.frame.data.parent_name = frames_list[1]
embary_frame.frame.data.state.pos = [1000000.0, 0.0, 0.0]
embary_frame.frame.data.state.vel = [0.0, 10.0, 0.0]
embary_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
embary_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Earth inertial
earth_inertial_frame.debug_frame_state = verbose
earth_inertial_frame.frame.data.name = frames_list[3]
earth_inertial_frame.frame.data.parent_name = frames_list[2]
earth_inertial_frame.frame.data.state.pos = [-1.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
earth_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Earth fixed
earth_fixed_frame.debug_frame_state = verbose
earth_fixed_frame.frame.data.name = frames_list[4]
earth_fixed_frame.frame.data.parent_name = frames_list[3]
earth_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, -45.0] )
earth_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.000 * deg2rad]
# Set the environmental acceleration values.
earth_fixed_frame.frame.accel_env = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.ang_accel_env = [0.0, 0.0, 0.0000 * deg2rad]

# Moon inertial
moon_inertial_frame.debug_frame_state = verbose
moon_inertial_frame.frame.data.name = frames_list[5]
moon_inertial_frame.frame.data.parent_name = frames_list[2]
moon_inertial_frame.frame.data.state.pos = [1.0, 0.0, 0.0]
moon_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
moon_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Moon fixed
moon_fixed_frame.debug_frame_state = verbose
moon_fixed_frame.frame.data.name = frames_list[6]
moon_fixed_frame.frame.data.parent_name = frames_list[5]
moon_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 45.0] )
moon_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0001 * deg2rad]

#
# Mars system
#
# Mars inertial
mars_inertial_frame.debug_frame_state = verbose
mars_inertial_frame.frame.data.name = frames_list[7]
mars_inertial_frame.frame.data.parent_name = frames_list[2]
mars_inertial_frame.frame.data.state.pos = [0.0, 2000000.0, 0.0]
mars_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
mars_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
mars_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Mars fixed
mars_fixed_frame.debug_frame_state = verbose
mars_fixed_frame.frame.data.name = frames_list[8]
mars_fixed_frame.frame.data.parent_name = frames_list[7]
mars_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 90.0] )
mars_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0001 * deg2rad]
