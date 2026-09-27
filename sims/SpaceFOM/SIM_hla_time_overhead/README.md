# SIM_hla_time_overhead

SIM_hla_time_overhead is a simulation that collects and prints statistics for the HLA time overhead. The distributed simulation is comprised of two simulations with no data being exchanged but HLA time management is enabled. Time statistics are collected for the elapsed time waiting for the Time Advance Grant (TAG) and for the elapsed time from the Time Advance Request (TAR) to the TAG. The -DTRICKHLA_PRINT_HLA_TIME_STATS value has been added to the TRICK_CFLAGS and TRICK_CXXFLAGS in the S_overrides.mk file to enable printing of the HLA time statistics even when debug verbose comments are turned off. We need to run without verbose debug comments turned on to keep the console output from affecting the time statistics.


---
### Building the Simulation
In the SIM_hla_time_overhead directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_hla_time_overhead directory:

```
./S_main_*.exe RUN_fed1_mpr/input.py
```

From another terminal, in the SIM_hla_time_overhead directory:

```
./S_main_*.exe RUN_fed2/input.py
```

Example HLA time statistics for Federate 1:


```
INFO: TrickHLA::ElapsedTimeStats::to_string():191
Wait for Time Advance Grant (TAG) Statistics:
    sample-count: 10001
             min: 0 milliseconds
             max: 3.403 milliseconds
            mean: 1.12806 milliseconds
  sample-std-dev: 0.127996 milliseconds
 margin-of-error: 0.373396% (0.00421212 milliseconds) with 99.9% confidence
 min-sample-size: 22311
    jitter-count: 10000
     jitter-mean: 0.135037 milliseconds
      jitter-min: 0 milliseconds
      jitter-max: 2.826 milliseconds
        guidance: To estimate the average elapsed time between measurements to
within a 0.25% (0.00282015 milliseconds) margin of error with a 99.9% confidence,
at least 22311 samples are needed based on the statistics.
```

```
INFO: TrickHLA::ElapsedTimeStats::to_string():191
Time Advance Request (TAR) to Time Advance Grant (TAG) Elapsed Time Statistics:
    sample-count: 10000
             min: 0.237 milliseconds
             max: 3.425 milliseconds
            mean: 1.14812 milliseconds
  sample-std-dev: 0.126528 milliseconds
 margin-of-error: 0.362685% (0.00416405 milliseconds) with 99.9% confidence
 min-sample-size: 21047
    jitter-count: 9999
     jitter-mean: 0.13464 milliseconds
      jitter-min: 0 milliseconds
      jitter-max: 2.811 milliseconds
        guidance: To estimate the average elapsed time between measurements to
within a 0.25% (0.00287029 milliseconds) margin of error with a 99.9% confidence,
at least 21047 samples are needed based on the statistics.
```
