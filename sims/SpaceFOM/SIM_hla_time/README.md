# SIM_hla_time

SIM_hla_time is a simulation that demonstrates the a simple SpaceFOM federate that implements the minimum to participate as a time constrained and time regulating federate. The distributed simulation is comprised of this simulation and any other SpaceFOM simulation. An example where this might be useful is if you have a simulation using [TrickCFS](https://github.com/nasa/TrickCFS) and need to synchronize it with other SpaceFOM simulations.

---
### Building the Simulation
In the SIM_hla_time directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
The SIM_hla_time simulation can be configured at runtime with command line settings to run in conjunction with any of the other SpaceFOM example simulations.

#### Example that does not use a Central Timing Equipment (CTE) timeline.

In the SIM_hla_time directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_hla_time
./S_main_*.exe RUN_test/input.py --fex_name SpaceFOM_sine --cte off --verbose on
```

From another terminal, in the SIM_sine directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine
./S_main_*.exe RUN_a_side_mpr/input.py --fex_name SpaceFOM_sine --verbose on
```

From another terminal, in the SIM_sine directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine
./S_main_*.exe RUN_p_side/input.py --fex_name SpaceFOM_sine --verbose on
```

#### Example using a CTE timeline.

In the SIM_hla_time directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_hla_time
./S_main_*.exe RUN_test/input.py --fex_name SpaceFOM_sine --cte on --verbose on
```

From another terminal, in the SIM_sine_cte directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine_cte
./S_main_*.exe RUN_a_side_mpr/input.py --fex_name SpaceFOM_sine --verbose on
```

From another terminal, in the SIM_sine_cte directory:

```
cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine_cte
./S_main_*.exe RUN_p_side/input.py --fex_name SpaceFOM_sine --verbose on
```
