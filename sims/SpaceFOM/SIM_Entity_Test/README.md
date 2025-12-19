# SIM_Entity_Test


---
### Building the Simulation
In the SIM_Entity_Test directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

From the $TRICKHLA_HOME/sims/SpaceFOM/SIM_Roles_Test directory, run the
Master, Pacing, and Root Reference Frame federate:

```
./S_main_*.exe RUN_mpr/input.py --verbose on
```

From the SIM_Entity_Test directory, run the Dynamical Entity federate:

```
./S_main_*.exe RUN_DE/input.py --verbose on
```

From the SIM_Entity_Test directory, run the Physical Entity federate:

```
./S_main_*.exe RUN_PE/input.py --verbose on
```

From the $TRICKHLA_HOME/sims/SpaceFOM/SIM_Roles_Test directory, run the
Other federate:

```
./S_main_*.exe RUN_other/input.py --verbose on
```
