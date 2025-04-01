


#---------------------------------------------------------------------------
# Set up the dynamics parameters for the vehicle test entity.
#---------------------------------------------------------------------------
vehicle.entity.pe_data.name         = 'Enterprise'
vehicle.entity.pe_data.type         = 'Constitution-class Starship'
vehicle.entity.pe_data.status       = '5 year mission'
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

# Principal inertia of a solid sphere.
vehicle_radius = 1.0
Ixx = Iyy = Izz = (2.0 / 5.0) * vehicle_mass * vehicle_radius * vehicle_radius
vehicle.entity.de_data.inertia[0] = [ Ixx, 0.0, 0.0 ]
vehicle.entity.de_data.inertia[1] = [ 0.0, Iyy, 0.0 ]
vehicle.entity.de_data.inertia[2] = [ 0.0, 0.0, Izz ]
vehicle.entity.de_data.inertia_rate[0] = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.inertia_rate[1] = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.inertia_rate[2] = [ 0.0, 0.0, 0.0 ]

# Base propagation parameters.
vehicle.entity.de_data.force  = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.torque = [ 0.0, 0.0, 0.0 ]
