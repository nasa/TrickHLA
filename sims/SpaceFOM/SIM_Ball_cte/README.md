# SIM_Ball

This example shows the use of Central Timing Equipment (CTE) and HLA time management to allow a hard realtime simulation not be impacted by overruns by the other two simulations.

Ball1: (Elastic realtime)
- Master Role: Yes
- Pacing Role: Yes
- Realtime: Yes
- CTE: Yes
- Time Regulating: Yes
- Time Constrained: Yes

Ball2: (Hard realtime)
- Master Role: No
- Pacing Role: No
- Realtime: Yes
- CTE: Yes
- Time Regulating: No
- Time Constrained: No

Ball3: (Disciplined to HLA timeline)
- Master Role: No
- Pacing Role: No
- Realtime: No
- CTE: No
- Time Regulating: Yes
- Time Constrained: Yes


---
### Building the Simulation

In the $TRICKHLA_HOME/models/Ball/graphics directory, build the graphics.

```
cd $TRICKHLA_HOME/models/Ball/graphics
make
```

In the SIM_Ball directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

In the SIM_Ball directory:

Run federate for ball 1:

```
./S_main_*.exe RUN_ball1/input.py
```

Run federate for ball 2:

```
./S_main_*.exe RUN_ball2/input.py
```

Run federate for ball 3:

```
./S_main_*.exe RUN_ball3/input.py
```
