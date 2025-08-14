# SIM_Roles_Test

SIM_Roles_Test is a simulation that demonstrates the Master, Root Reference
Frame (RRF), and Pacing roles as defined by the SpaceFOM standard.

---
### Building the Simulation
In the SIM_Roles_Test directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

All the commands are run from the SIM_Roles_Test directory.

#### Running with a single role per federate:

Master Role federate:

```
./S_main_*.exe RUN_Master/input.py --verbose on
```

Root Reference Frame Role federate:

```
./S_main_*.exe RUN_RRFP/input.py --verbose on
```

Pacing Role federate:

```
./S_main_*.exe RUN_Pacing/input.py --verbose on
```

---
#### Late Joining Federate:

Run the commands for the one role per federate case above and click
"Start" on the simulation control panel to start the simulation, then
run this command for the "Other" federate as a late joining federate:

```
./S_main_*.exe RUN_other/input.py --verbose on
```

---
#### Running with Master, Pacing, and Root Reference Frame (MPR) roles combined:

Master, Pacing, and Root Reference Frame (MPR) role federate:

```
./S_main_*.exe RUN_mpr/input.py --verbose on
```

Other federate:

```
./S_main_*.exe RUN_other/input.py --verbose on
```
