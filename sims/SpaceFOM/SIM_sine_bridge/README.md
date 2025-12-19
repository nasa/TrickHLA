# SIM_sine_bridge

SIM_sine_bridge is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation is comprised
of two simulations joined to two federations. The A-side federate owns
the analytic sine state and the P-side federate owns the propagated
sine state.

Demonstrates a single simulation being joined to two separate federation
executions as a bridge between them. The SIM_sine_bridge simulation
will join a federation that uses both the Integrated Mission
Simulation (IMSim) and SpaceFOM execution control schemes.

---
### Building the Simulation
In the SIM_sine_bridge directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation

In the SIM_sine_bridge directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From another terminal, in the SIM_sine_bridge directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
