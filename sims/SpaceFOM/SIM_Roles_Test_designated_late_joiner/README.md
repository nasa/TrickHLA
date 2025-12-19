# SIM_Roles_Test_designated_late_joiner

SIM_Roles_Test_designated_late_joiner is a simulation that demonstrates
the designated late joiner configuration.

---
### Designated Late Joiner configuration

Configure the federate to be a late joiner.

RUN_other_designated_late_joiner/input.py:

```
# Configure to be a designated late jointer federate.
federate.set_designated_late_joiner( True )
```

---
### Building the Simulation
In the SIM_Roles_Test_designated_late_joiner directory, type **trick-CP**
to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

All the commands are run from the SIM_Roles_Test_designated_late_joiner directory.

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
#### Designated Late Joining Federate:

Run the commands for the one role per federate case above and click
"Start" on the simulation control panel to start the simulation, then
run this command for the designated late joining "Other" federate:

```
./S_main_*.exe RUN_other_designated_late_joiner/input.py --verbose on
```

---
#### Running with Master, Pacing, and Root Reference Frame (MPR) roles combined:

Master, Pacing, and Root Reference Frame (MPR) role federate:

```
./S_main_*.exe RUN_mpr/input.py --verbose on
```

Designated late joining Other federate:

```
./S_main_*.exe RUN_other_designated_late_joiner/input.py --verbose on
```
