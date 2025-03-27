#
# Setup the active vehicle.
#
veh1.dyn_body.set_name( "Enterprise" )
veh1.dyn_body.integ_frame_name = "Moon.inertial"
veh1.dyn_body.translational_dynamics = True
veh1.dyn_body.rotational_dynamics = True

# Setup gravity model.
veh1.sun_grav_control.source_name = "Sun"
veh1.sun_grav_control.active      = True
veh1.sun_grav_control.spherical   = True
veh1.sun_grav_control.gradient    = False

veh1.earth_grav_control.source_name = "Earth"
veh1.earth_grav_control.active      = True
veh1.earth_grav_control.spherical   = True
veh1.earth_grav_control.gradient    = False

veh1.moon_grav_control.source_name = "Moon"
veh1.moon_grav_control.active      = True
veh1.moon_grav_control.spherical   = True
veh1.moon_grav_control.gradient    = False
veh1.moon_grav_control.degree      = 60
veh1.moon_grav_control.order       = 60

veh1.mars_grav_control.source_name = "Mars"
veh1.mars_grav_control.active      = False
veh1.mars_grav_control.spherical   = True
veh1.mars_grav_control.gradient    = False

veh1.dyn_body.grav_interaction.add_control(veh1.sun_grav_control)
veh1.dyn_body.grav_interaction.add_control(veh1.earth_grav_control)
veh1.dyn_body.grav_interaction.add_control(veh1.moon_grav_control)
veh1.dyn_body.grav_interaction.add_control(veh1.mars_grav_control)

# Setup mass properties.
#defaults units are SI unless otherwise stated.
veh1.mass_init.set_subject_body( veh1.dyn_body.mass )
veh1.mass_init.properties.mass = 424.0
veh1.mass_init.properties.pt_orientation.data_source =    trick.Orientation.InputEigenRotation
veh1.mass_init.properties.pt_orientation.eigen_angle = 0.0
veh1.mass_init.properties.pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]
veh1.mass_init.properties.position    = [ 0.0, 0.0, 0.0]
veh1.mass_init.properties.inertia[0]  = [ 1.0, 0.0, 0.0]
veh1.mass_init.properties.inertia[1]  = [ 0.0, 1.0, 0.0]
veh1.mass_init.properties.inertia[2]  = [ 0.0, 0.0, 1.0]

# Create the docking port mass point interface.
veh1.mass_init.set_subject_body( veh1.dyn_body.mass )
veh1.mass_init.allocate_points(1)

veh1.mass_init.get_mass_point(0).set_name("Active docking port")
veh1.mass_init.get_mass_point(0).pt_frame_spec              = trick.MassPointInit.StructToBody
veh1.mass_init.get_mass_point(0).position                   = trick.attach_units( "m",[ 1.0, 0.0, 0.0])
veh1.mass_init.get_mass_point(0).pt_orientation.data_source = trick.Orientation.InputEigenRotation

veh1.mass_init.get_mass_point(0).pt_orientation.eigen_angle = trick.attach_units( "degree",0.0)
veh1.mass_init.get_mass_point(0).pt_orientation.eigen_axis  = [ 0.0, 0.0, 1.0]

# Set initial state.
veh1.trans_init.set_subject_body( veh1.dyn_body )
veh1.trans_init.reference_ref_frame_name = "Moon.inertial"
veh1.trans_init.body_frame_id            = "composite_body"

veh1.trans_init.position = trick.attach_units( "km",[  1296.944012, -1060.824450, 2522.289146])
#veh1.trans_init.velocity = trick.attach_units( "km/s",[ -.930578, -.439312, .862075])
veh1.trans_init.velocity = trick.attach_units( "km/s",[ 0.0, 0.0, 0.0 ])

#veh1.rot_init.state_items = trick.DynBodyInitRotState.Rate
veh1.rot_init.set_subject_body( veh1.dyn_body )
veh1.rot_init.reference_ref_frame_name = "Moon.inertial"
veh1.rot_init.body_frame_id = "composite_body"
veh1.rot_init.ang_velocity  = [ 0.0, 0.0, 0.0]
veh1.rot_init.orientation.data_source = trick.Orientation.InputEulerRotation
veh1.rot_init.orientation.euler_sequence  = trick.Orientation.Pitch_Yaw_Roll
veh1.rot_init.orientation.euler_angles  = trick.attach_units( "degree",[  180.0, 0.0, 0.0])

# Configure vehicle integration information.
veh1.dyn_body.set_name( 'Enterprise' )
veh1.dyn_body.integ_frame_name       = "Moon.inertial"
veh1.dyn_body.translational_dynamics = True
veh1.dyn_body.rotational_dynamics    = True

# Setup Dynamics Manager info.
dynamics.dyn_manager.add_body_action(veh1.mass_init)
dynamics.dyn_manager.add_body_action(veh1.trans_init)
dynamics.dyn_manager.add_body_action(veh1.rot_init)

