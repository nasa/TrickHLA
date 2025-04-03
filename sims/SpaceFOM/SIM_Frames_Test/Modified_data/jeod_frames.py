

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
sun_frame.frame.data.state.pos = [54497420.2529776, -547014626.223842, -227657771.743029]
sun_frame.frame.data.state.vel = [6.63589955065778, 7.31895812958038, 2.99555951338398]
sun_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
sun_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

#
# Earth-Moon system
#
embary_frame.debug_frame_state = verbose
embary_frame.frame.data.name = frames_list[2]
embary_frame.frame.data.parent_name = frames_list[1]
embary_frame.frame.data.state.pos = [122582440501.768, -82243635093.0704, -35642071660.4583]
embary_frame.frame.data.state.vel = [17034.002004586, 22014.6742387964, 9542.58668628563]
embary_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
embary_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Earth inertial
earth_inertial_frame.debug_frame_state = verbose
earth_inertial_frame.frame.data.name = frames_list[3]
earth_inertial_frame.frame.data.parent_name = frames_list[2]
earth_inertial_frame.frame.data.state.pos = [-4079194.11969718, 2566711.37265299, 985330.679215729]
earth_inertial_frame.frame.data.state.vel = [-6.42556813994991, -8.6692850075878, -4.90100843691637]
earth_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
earth_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Earth fixed
earth_fixed_frame.debug_frame_state = verbose
earth_fixed_frame.frame.data.name = frames_list[4]
earth_fixed_frame.frame.data.parent_name = frames_list[3]
earth_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.att.scalar = 0.29830103677317
earth_fixed_frame.frame.data.state.att.vector = [-0.001290897530824, -0.00041407157205325, -0.954470876238951]
earth_fixed_frame.frame.data.state.ang_vel = [0, 0, 7.29211514670639e-05]

# Moon inertial
moon_inertial_frame.debug_frame_state = verbose
moon_inertial_frame.frame.data.name = frames_list[5]
moon_inertial_frame.frame.data.parent_name = frames_list[2]
moon_inertial_frame.frame.data.state.pos = [331640799.817171, -208675093.057267, -80107944.1063125]
moon_inertial_frame.frame.data.state.vel = [522.402340923877, 704.817797190995, 398.454770779652]
moon_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
moon_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Moon fixed
moon_fixed_frame.debug_frame_state = verbose
moon_fixed_frame.frame.data.name = frames_list[6]
moon_fixed_frame.frame.data.parent_name = frames_list[5]
moon_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.att.scalar = -0.270119215788571
moon_fixed_frame.frame.data.state.att.vector = [0.0634622113468803, -0.183899591907175, 0.94296823758232]
moon_fixed_frame.frame.data.state.ang_vel = [-1.05728117046177e-09, 4.90210558152287e-10, 2.6618546609035e-06]

#
# Mars system
#
# Mars inertial
mars_inertial_frame.debug_frame_state = verbose
mars_inertial_frame.frame.data.name = frames_list[7]
mars_inertial_frame.frame.data.parent_name = frames_list[2]
mars_inertial_frame.frame.data.state.pos = [-141056572685.408, -168330087644.195, -73381242639.612]
mars_inertial_frame.frame.data.state.vel = [20104.8515600042, -11356.7552765221, -5751.49113503887]
mars_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
mars_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]

# Mars fixed
mars_fixed_frame.debug_frame_state = verbose
mars_fixed_frame.frame.data.name = frames_list[8]
mars_fixed_frame.frame.data.parent_name = frames_list[7]
mars_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.att.scalar = 0.440173190065843
mars_fixed_frame.frame.data.state.att.vector = [-0.307990248174647, 0.080693180678851, -0.839570235518452]
mars_fixed_frame.frame.data.state.ang_vel = [0, 0, 7.08821812777419e-05]
