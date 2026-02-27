# SIM_Ball_HLA

---
### Building the Simulation

In the $TRICKHLA_HOME/models/Ball/graphics directory, build the graphics.

```
cd $TRICKHLA_HOME/models/Ball/graphics
make
```

In the SIM_Ball_HLA directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

In the SIM_Ball_HLA directory:

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

---

Run a single federate with all 3 balls:

```
./S_main_*.exe RUN_test/input.py
```
