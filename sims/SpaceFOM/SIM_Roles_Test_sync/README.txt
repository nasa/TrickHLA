This simulation will be used to demonstrate three techniques for synchronizing
the dynamics/physics of the federates using the SpaceFOM standard.

Techniques for synchronizing the dynamics of the federates:
1) HLA Time Management with Time Constrained and Time Regulating Federates.
2) Blocking cyclic data reads.
3) Common Clock.

An advantage of the HLA Time Management and blocking cyclic data read approaches
is the distributed simulation can run both realtime and faster than realtime
and still remain synchronized.

The disadvantage of the Common Clock approach is only realtime execution of the
distributed simulation is possible. 


-------------------------------------------------------------------------------
1) Configuring all federates to use HLA Time Management with Time Constrained
and Time Regulating enabled will achieve dynamics synchronization for all the
federates.

An advantage of this approach is that distributed simulation can run both
realtime and faster than realtime and still remain synchronized.

Master/Pacing/Root-Reference-Frame Federate:
When using the SpaceFOM, the Master/Pacing role will always need to use HLA time
management and be configured to be time constrained and time regulating.

For this test, we will run for 30,000 seconds without a realtime clock to
demonstrate the number of data items sent matches the number of data items
received. We disable the realtime clock in the Master federate so that we don't
have to wait the 30,000 seconds for this simulation to complete.

RESULTS:
The first result is the number of reference frames sent by the Master Federate
matches the number received by the "Other" federate. Had the "Other" federate
not been synchronized to the Master federate then the frame counts would not have
matched.

Master/Pacing Federate:
./S_main_Darwin_18.exe RUN_mpr/input.py \
 -realtime off \
 -hla-time-mgt on \
 -regulating on \
 -constrained on \
 -stop 30000 \
 -verbose off

Master Federate Console Output:
-------------------------------
Realtime Clock Disabled.
HLA Time Management Enabled.
HLA Time Regulating Enabled.
HLA Time Constrained Enabled.
Run duration: 30000.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:120003 receive_count:0
Federate::shutdown():4484 Object[1]:'FrameA' send_count:120002 receive_count:0
|L   0|2020/08/28,09:19:40|wasp.local| |T 0|30000.000000| 
     REALTIME SHUTDOWN STATS:
            ACTUAL INIT TIME:       15.061
         ACTUAL ELAPSED TIME:      157.650
|L   0|2020/08/28,09:19:40|wasp.local| |T 0|30000.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:    30000.000
     SIMULATION ELAPSED TIME:    30000.000
        ACTUAL CPU TIME USED:       28.963
       SIMULATION / CPU TIME:     1035.814
     INITIALIZATION CPU TIME:        2.208


----------
The "Other" federate that consumes reference frames will be run with HLA time
management, time constrained and time regulating enabled.

"Other" Federate:
./S_main_Darwin_18.exe RUN_other/input.py \
 -hla-time-mgt on \
 -regulating on \
 -constrained on \
 -stop 30000 \
 -verbose off

"Other" Federate Console Output:
--------------------------------
Realtime Clock Disabled.
HLA Time Management Enabled.
HLA Time Regulating Enabled.
HLA Time Constrained Enabled.
Blocking cyclic reads for RootFrame Disabled.
Blocking cyclic reads for FrameA Disabled.
Run duration: 30000.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:0 receive_count:120003
Federate::shutdown():4484 Object[1]:'FrameA' send_count:0 receive_count:120002
|L   0|2020/08/28,09:19:38|wasp.local| |T 0|30000.000000| 
     REALTIME SHUTDOWN STATS:
            ACTUAL INIT TIME:        0.766
         ACTUAL ELAPSED TIME:      141.348
|L   0|2020/08/28,09:19:38|wasp.local| |T 0|30000.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:    30000.000
     SIMULATION ELAPSED TIME:    30000.000
        ACTUAL CPU TIME USED:       30.231
       SIMULATION / CPU TIME:      992.348
     INITIALIZATION CPU TIME:        0.983


-------------------------------------------------------------------------------
2) Blocking cyclic data reads.

If all Federates block waiting to receive data from each of the other federates
and for the same data exchange rate between them, then the federates will be
synchronized. Typically a federate will send data at the end of a frame and wait
to receive data at the top of a frame.

An advantage of this approach is that distributed simulation can run both
realtime and faster than realtime and still remain synchronized.

HLA Time Management should be enabled so HLA Logical Timeline is propagated
through the use of the Time Advance Request (TAR) and Time Advance Grant (TAG)
API's.

Typically we would configure the federates to not be time constrained or time
regulating because the blocking reads among all federates would achieve
synchronization. However, in this example the Master federate only publishes
reference frame data and does not receive any data and the "Other" federate only
receives reference data and does not publish any data. Essentially we have the
case of data flowing from one federate to another.

How can blocking data reads be used to synchronize the federates if data is not
sent between all federates? The answer is a combination of blocking reads with
configuring federates to be HLA Time Regulating. A Time regulating,
non-constrained federate will regulate the advancement of time for other
regulating federates. Time will not advance until a federate receiving data for
a blocking read. This configuration will prevent one federate from walking away 
(i.e. being out of sync) with the other federates. It's a hybrid approach that
works and had the benefit of less overhead due to Federation HLA time management.

RESULTS:
The first thing to notice is the simulation ran almost twice as fast using
blocking cyclic reads with the "Other" federate being time regulating and
not time constrained as compared to the previous example where it was both
time regulation and time constrained. Please note in this hybrid blocking read
with HLA time regulating example, there is an inherent race condition to the
"Other" federate receiving the last piece of data before the Master federate
shutdown. This results in one less receive of data than what was sent, but
only at the end.


Master/Pacing/Root-Reference-Frame Federate:
When using the SpaceFOM, the Master/Pacing role will always need to use HLA time
management and be configured to be time constrained and time regulating.

For this test, we will run for 30,000 seconds without a realtime clock to
demonstrate the number of data items sent matches the number of data items
received. We disable the realtime clock in the Master federate so that we don't
have to wait the 30,000 seconds for this simulation to complete.

Master/Pacing Federate:
./S_main_Darwin_18.exe RUN_mpr/input.py \
 -realtime off \
 -hla-time-mgt on \
 -regulating on \
 -constrained on \
 -stop 30000 \
 -verbose off

Master Federate Console Output:
-------------------------------
Realtime Clock Disabled.
HLA Time Management Enabled.
HLA Time Regulating Enabled.
HLA Time Constrained Enabled.
Run duration: 30000.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:120003 receive_count:0
Federate::shutdown():4484 Object[1]:'FrameA' send_count:120002 receive_count:0
|L   0|2020/08/28,09:54:57|wasp.local| |T 0|30000.000000| 
     REALTIME SHUTDOWN STATS:
            ACTUAL INIT TIME:        5.316
         ACTUAL ELAPSED TIME:       84.968
|L   0|2020/08/28,09:54:57|wasp.local| |T 0|30000.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:    30000.000
     SIMULATION ELAPSED TIME:    30000.000
        ACTUAL CPU TIME USED:       23.758
       SIMULATION / CPU TIME:     1262.721
     INITIALIZATION CPU TIME:        1.359


----------
The "Other" federate that consumes reference frames will be run with blocking
cyclic reads, HLA time management enabled, time regulating enabled and time
constrained disabled.

"Other" Federate:
./S_main_Darwin_18.exe RUN_other/input.py \
 -blocking-reads on \
 -hla-time-mgt on \
 -regulating on \
 -constrained off \
 -stop 30000 \
 -verbose off
 
"Other" Federate Console Output:
--------------------------------
Realtime Clock Disabled.
HLA Time Management Enabled.
HLA Time Regulating Enabled.
HLA Time Constrained Disabled.
Blocking cyclic reads for RootFrame Enabled.
Blocking cyclic reads for FrameA Enabled.
Run duration: 30000.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:0 receive_count:120002
Federate::shutdown():4484 Object[1]:'FrameA' send_count:0 receive_count:120001
|L   0|2020/08/28,09:54:55|wasp.local| |T 0|30000.000000| 
     REALTIME SHUTDOWN STATS:
            ACTUAL INIT TIME:        0.719
         ACTUAL ELAPSED TIME:       78.365
|L   0|2020/08/28,09:54:55|wasp.local| |T 0|30000.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:    30000.000
     SIMULATION ELAPSED TIME:    30000.000
        ACTUAL CPU TIME USED:       86.278
       SIMULATION / CPU TIME:      347.711
     INITIALIZATION CPU TIME:        0.961


-------------------------------------------------------------------------------
3) Common Clock.

If all federates used the same realtime clock then they would all advance time
at the same rate and be synchronized.

A disadvantage of this approach is the federation cannot be run faster than
realtime.

Master/Pacing/Root-Reference-Frame Federate:
When using the SpaceFOM, the Master/Pacing role will always need to use HLA time
management and be configured to be time constrained and time regulating.

For this test, we will only run for 30 seconds with a realtime clock to
demonstrate the number of data items sent matches the number of data items
received.

RESULTS:
There were zero overruns by both federates so they remained synchronized.
Please note in this example, there is an inherent race condition for the "Other"
federate receive the last piece of data before the Master federate shutdown.
This results in one less receive of data than what was sent, but only at the end.


Master/Pacing Federate:
./S_main_Darwin_18.exe RUN_mpr/input.py \
 -realtime on \
 -hla-time-mgt on \
 -regulating on \
 -constrained on \
 -stop 30 \
 -verbose off

Master Federate Console Output:
-------------------------------
Realtime Clock Enabled.
HLA Time Management Enabled.
HLA Time Regulating Enabled.
HLA Time Constrained Enabled.
Run duration: 30.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:123 receive_count:0
Federate::shutdown():4484 Object[1]:'FrameA' send_count:122 receive_count:0
|L   0|2020/08/28,10:15:44|wasp.local| |T 0|30.000000| 
     REALTIME SHUTDOWN STATS:
     REALTIME TOTAL OVERRUNS:            0
            ACTUAL INIT TIME:        3.340
         ACTUAL ELAPSED TIME:       35.364
|L   0|2020/08/28,10:15:44|wasp.local| |T 0|30.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:       30.000
     SIMULATION ELAPSED TIME:       30.000
        ACTUAL CPU TIME USED:       21.502
       SIMULATION / CPU TIME:        1.395
     INITIALIZATION CPU TIME:        1.205


----------
The "Other" federate that consumes reference frames will be run with realtime
enabled, blocking cyclic reads disabled, HLA time management enabled and both
time regulating and time constrained disabled.

"Other" Federate:
./S_main_Darwin_18.exe RUN_other/input.py \
 -realtime on \
 -blocking-reads off \
 -hla-time-mgt on \
 -regulating off \
 -constrained off \
 -stop 30 \
 -verbose off
 
"Other" Federate Console Output:
--------------------------------
Realtime Clock Enabled.
HLA Time Management Enabled.
HLA Time Regulating Disabled.
HLA Time Constrained Disabled.
Blocking cyclic reads for RootFrame Disabled.
Blocking cyclic reads for FrameA Disabled.
Run duration: 30.0
Federate::shutdown():4484 Object[0]:'RootFrame' send_count:0 receive_count:122
Federate::shutdown():4484 Object[1]:'FrameA' send_count:0 receive_count:121
|L   0|2020/08/28,10:15:42|wasp.local| |T 0|30.000000| 
     REALTIME SHUTDOWN STATS:
     REALTIME TOTAL OVERRUNS:            0
            ACTUAL INIT TIME:        0.719
         ACTUAL ELAPSED TIME:       30.735
|L   0|2020/08/28,10:15:42|wasp.local| |T 0|30.000000| 
SIMULATION TERMINATED IN
  PROCESS: 0
  ROUTINE: Executive_loop_single_thread.cpp:98
  DIAGNOSTIC: Reached termination time

       SIMULATION START TIME:        0.000
        SIMULATION STOP TIME:       30.000
     SIMULATION ELAPSED TIME:       30.000
        ACTUAL CPU TIME USED:       21.618
       SIMULATION / CPU TIME:        1.388
     INITIALIZATION CPU TIME:        0.962

