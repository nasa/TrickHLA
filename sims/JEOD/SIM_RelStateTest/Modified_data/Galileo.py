#
# Setup the active vehicle.
#
veh.dyn_body.set_name( vehicle_name )
veh.dyn_body.integ_frame_name = frames_dict[native_frame]
veh.dyn_body.translational_dynamics = True
veh.dyn_body.rotational_dynamics = True

# Fill in additional SpaceFOM::PhysicalEntity data parameters.
veh_physical_entity.entity_packing.set_type( 'Starship' )
veh_physical_entity.entity_packing.set_status( 'Active' )
veh_physical_entity.entity_packing.set_parent_frame( native_frame )

# Setup gravity model.
veh.sun_grav_control.source_name = "Sun"
veh.sun_grav_control.active      = True
veh.sun_grav_control.spherical   = True
veh.sun_grav_control.gradient    = False

veh.earth_grav_control.source_name = "Earth"
veh.earth_grav_control.active      = True
veh.earth_grav_control.spherical   = True
veh.earth_grav_control.gradient    = False

veh.moon_grav_control.source_name = "Moon"
veh.moon_grav_control.active      = True
veh.moon_grav_control.spherical   = True
veh.moon_grav_control.gradient    = False
veh.moon_grav_control.degree      = 60
veh.moon_grav_control.order       = 60

veh.mars_grav_control.source_name = "Mars"
veh.mars_grav_control.active      = False
veh.mars_grav_control.spherical   = True
veh.mars_grav_control.gradient    = False

veh.dyn_body.grav_interaction.add_control(veh.sun_grav_control)
veh.dyn_body.grav_interaction.add_control(veh.earth_grav_control)
veh.dyn_body.grav_interaction.add_control(veh.moon_grav_control)
veh.dyn_body.grav_interaction.add_control(veh.mars_grav_control)

# Setup mass properties.
#defaults units are SI unless otherwise stated.
veh.mass_init.set_subject_body( veh.dyn_body.mass )
veh.mass_init.properties.mass = 424.0
veh.mass_init.properties.pt_orientation.data_source =    trick.Orientation.InputEigenRotation
veh.mass_init.properties.pt_orientation.eigen_angle = 0.0
veh.mass_init.properties.pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]
veh.mass_init.properties.position    = [ 0.0, 0.0, 0.0]
veh.mass_init.properties.inertia[0]  = [ 1.0, 0.0, 0.0]
veh.mass_init.properties.inertia[1]  = [ 0.0, 1.0, 0.0]
veh.mass_init.properties.inertia[2]  = [ 0.0, 0.0, 1.0]

# Create the docking port mass point interface.
veh.mass_init.set_subject_body( veh.dyn_body.mass )
veh.mass_init.allocate_points(1)

veh.mass_init.get_mass_point(0).set_name('Active docking port')
veh.mass_init.get_mass_point(0).pt_frame_spec              = trick.MassPointInit.StructToBody
veh.mass_init.get_mass_point(0).position                   = trick.attach_units( 'm',[ 1.0, 0.0, 0.0])
veh.mass_init.get_mass_point(0).pt_orientation.data_source = trick.Orientation.InputEigenRotation

veh.mass_init.get_mass_point(0).pt_orientation.eigen_angle = trick.attach_units( 'degree',0.0)
veh.mass_init.get_mass_point(0).pt_orientation.eigen_axis  = [ 0.0, 0.0, 1.0]

# Set initial state.
veh.trans_init.set_subject_body( veh.dyn_body )
veh.trans_init.reference_ref_frame_name = frames_dict[native_frame]
veh.trans_init.body_frame_id            = 'composite_body'

veh.trans_init.position = trick.attach_units( 'm',[-334423049.924868, 210180979.97992, 83615563.9315283])
veh.trans_init.velocity = trick.attach_units( 'm/s',[ -528.827909063827, -713.487082198582, -403.355779216569 ])

#veh.rot_init.state_items = trick.DynBodyInitRotState.Rate
veh.rot_init.set_subject_body( veh.dyn_body )
veh.rot_init.reference_ref_frame_name = frames_dict[native_frame]
veh.rot_init.body_frame_id = 'composite_body'
veh.rot_init.ang_velocity  = [ 0.0, 0.0, 0.0]
veh.rot_init.orientation.data_source = trick.Orientation.InputEulerRotation
veh.rot_init.orientation.euler_sequence  = trick.Orientation.Pitch_Yaw_Roll
veh.rot_init.orientation.euler_angles  = trick.attach_units( 'degree',[  180.0, 0.0, 0.0])

# Configure vehicle integration information.
veh.dyn_body.set_name( vehicle_name )
veh.dyn_body.integ_frame_name       = frames_dict[native_frame]
veh.dyn_body.translational_dynamics = True
veh.dyn_body.rotational_dynamics    = True

# Setup Dynamics Manager info.
dynamics.dyn_manager.add_body_action(veh.mass_init)
dynamics.dyn_manager.add_body_action(veh.trans_init)
dynamics.dyn_manager.add_body_action(veh.rot_init)

