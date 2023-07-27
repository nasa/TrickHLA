# SIM\_wheelbot

---

SIM\_wheelbot is a simulation of a two-wheeled electric vehicle that follows a set of waypoints.

![Picture of Vehicle](images/Figure1.png)
---
### Building the Simulation
In the SIM\_wheelbot directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
=== Trick build process complete ===
```

Now **cd** into models/Graphics/ and type **make**. This builds the display client for the simulation. Then **cd** into models/guicontrol1 and type **make**. This builds the gui control that allows user to command the wheelbot to go to home point.

### Running the Simulation
In the SIM_wheelbot directory:

```
% S_main_*.exe RUN_test/input.py
```

---

### Running the HLA Compatible Version
In order to run a federation, the user must have PitchRTI installed, as well as the latest version  of the NASA/TrickHLA repository from GitHub.com.
The SIM_Roles_Test RUN_mpr serves as the Master, Pacing, and Root Reference Frame Publisher (MPR) for the federation. Wheelbot-1 (RUN_wb1) publishes its location to the RTI as it navigates to its waypoints. Wheelbot-2 (RUN_wb2) creates its waypoints by subscribing to this location data, and follows Wheelbot-1 around.

When running SIM_Roles_Test RUN_mpr, the user must specify the name of the federation and the name of the known, required federate in order for the federation to form properly.

In the SIM_Roles_Test directory:

```
% S_main_*.exe RUN_mpr/input.py --federation Wheelbot_Test --knownfederate Wheelbot-1
```

In the SIM_wheelbot directory:

```
% S_main_*.exe RUN_wb1/input.py
```
```
% S_main_*.exe RUN_wb2/input.py
```

### If running with student version of pRTI
The student version of PitchRTI only allows two federates to be run at once. RUN_OnlyWB was created to serve the purpose of RUN_mpr AND RUN_wb1.

In the SIM_wheelbot directory:

```
% S_main_*.exe RUN_OnlyWB/input.py
```
```
% S_main_*.exe RUN_wb2/input.py
```

### Inputs
Variable                                       | Type           | Units | Default Value
-----------------------------------------------|----------------|-------|--------------
veh.vehicle.distanceBetweenWheels              | double         | m     | 0.183
veh.vehicle.wheelRadius                        | double         | m     | 0.045
veh.vehicle.wheelSpeedLimit                    | double         | rad/s | 8.880
veh.vehicle.headingRateLimit                   | double         | rad/s | 𝛑/4
veh.vehicle.wheelDragConstant                  | double         | --    | 1.875
veh.vehicle.corningStiffness                   | double         | --    | 10.0
veh.vehicle.slowDownDistance                   | double         | --    | 0.5
veh.vehicle.arrivalDistance                    | double         | --    | 0.1

![Picture of Vehicle](images/Figure2.png)

#### Adding Waypoints
Waypoints, for the vehicle to follow, are added with a call to

veh.vehicle.add_waypoint( double N, double W )

#### Adding Home point
A home point is designated by the last waypoint in the waypoint file.


### Input/Output
Variable                                       | Type           | Units
-----------------------------------------------|----------------|-------
veh.vehicle.position                           | double[2]      | m
veh.vehicle.velocity                           | double[2]      | m
veh.vehicle.heading                            | double         | rad
veh.vehicle.headingRate                        | double         | rad


### Outputs
Variable                                      | Type           | Units
----------------------------------------------|----------------|--------
veh.vehicle.acceleration                      | double[2]      | m
veh.vehicle.headingAccel                      | double         | rad
veh.vehicle.driveForce                        | double[2]      | N
veh.vehicle.lateralTireForce                  | double[2]      | N
veh.vehicle.rollingResistForce                | double[2]      | N
veh.vehicle.forceTotal                        | double[2]      | N
veh.vehicle.vehicleZTorque                    | double         | N*m