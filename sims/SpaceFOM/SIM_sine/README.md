# SIM_sine

SIM_sine is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation is comprised
of two simulations. The A-side federate owns the analytic sine state and
the P-side federate owns the propagated sine state.

---
### Building the Simulation
In the SIM_sine directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From a another console, in the SIM_sine directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
