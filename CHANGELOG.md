# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Notable Additions

- Added support for IEEE 1516-2025 (HLA 4).
- Added support for Fixed Records including direct encoding and decoding of Trick simulation variables from memory.
- Added support for Trick enumerations, but HLA encoding is limited to 32-bit integers as either Big or Little Endian (i.e. ENCODING_ENUM_INT32_BE or ENCODING_ENUM_INT32_LE) (issue #181).
- Improved the variable array encoder efficiency for dynamic arrays.
- Shutdown on notification of SpaceFOM Execution Configuration Object (ExCO) deletion (SpaceFOM PCR-002).
- Added support for designated late joining federates (SpaceFOM PCR-003).
- Added support for specifying the HLA time resolution at runtime using the SpaceFOM ExCO (SpaceFOM PCR-007).

### Breaking Changes

- Using std::string for TrickHLA settings instead of char *.
- Renamed *ObjectDeleted* class to *ObjectDeletedHandler* to be consistent with the naming convention used for other handler APIs.
  - FROM: ObjectDeleted
  - TO: ObjectDeletedHandler
- Renamed *Modified_data* to *TrickHLA_data* to better support Default Data.
  - FROM: Modified_data
  - TO: TrickHLA_data
- Scaling the Trick executive tics to the HLA base time multiplier is now combined with setting the HLA base time multiplier to resolve a time check dependency.
  - FROM: federate.scale_trick_tics_to_base_time_unit()
  - TO: federate.set_HLA_base_time_unit_and_scale_trick_tics( unit )
     or federate.set_HLA_base_time_multiplier_and_scale_trick_tics( multiplier )
- Configure and initialize TrickHLA from default data instead of the input file.
  - Adding S_models/THLAPackingBase.sm to your simulation S_define file will allow you to configure and initialize the TrickHLA settings using default data instead of using an input.py file.
  - Support for Default Data initialization will now require the *Conditional*, *LagCompensation*, and *Packing* implementations to make a call to set the sim-data before calling the no-argument *configure()* and *initialize()* functions in the S_define file.
  - If you do not intend to use S_models/THLAPackingBase.sm to add support for using Default Data to configure and initialize TrickHLA, then you will need to explicitly call *set_sim_data()*, *configure()*, and *initialize()* functions from the S_define file.

```
    FROM:
      P50 ("initialization") lag_compensation.configure( &sim_data );
      P50 ("initialization") lag_compensation.initialize();
      
      P50 ("initialization") packing.configure( &lag_compensation );
      P50 ("initialization") packing.initialize();

      P50 ("initialization") conditional.configure( &packing );
      P50 ("initialization") conditional.initialize();

    TO:
      # $TRICKHLA_HOME/sims/SpaceFOM/SIM_sinw/S_define
      P50 ("initialization") lag_compensation.set_sim_data( &sim_data );
      P50 ("initialization") lag_compensation.configure();
      P50 ("initialization") lag_compensation.initialize();

      P50 ("initialization") packing.set_sim_data( &lag_compensation );
      P50 ("initialization") packing.configure();
      P50 ("initialization") packing.initialize();

      P50 ("initialization") conditional.set_sim_data( &packing );
      P50 ("initialization") conditional.configure();
      P50 ("initialization") conditional.initialize();
```

### Added

- Added this CHANGELOG file.
- Added Ball simulation based off the Trick tutorial Ball simulation.
- Added a check to the THLABaseSimObject to set the interaction cycle time to the data cycle time if a time is not set.
- Added a check to the THLABaseSimObject to verify the interaction cycle time is an integer multiple of the data cycle time.
- Added functions to TrickHLAFederateConfig.py to allow the HLA base time multiplier to be set and automatically scale the Trick time tics value.
  - 

### Changed

- Changed over to use the IEEE 1516 HLA encoder helpers instead of custom written encoders.
- Changed the Execution Control to perform the Least Common Time Step (LCTS) time checks when an Execution Configuration Object (ExCO) change is processed (issue #179).
- Changed the Timeline functions to be more const friendly (issue #180).


## [v3.1.18] - 2025-04-10

### Added

- Added checks for duplicate interactions for the same FOM name.
- Added missing includes.

### Changed

- Updated job phasing.
- Handle ExecutionControl interactions first before user interactions.
- Updated SIM_Frame_Test and SIM_RelStateTest simulations.
- Cleaned up and tested the RefFrameTree, RefFrameData, and RefFrameDataState code base.

## [v3.1.17] - 2025-03-27

### Changed

- More cleanup for frame test simulations.


## [v3.1.16] - 2025-03-26

### Changed

- Workaround swig 3.x and 4.0 issue with sim object instance variable references by making them private.


## [v3.1.15] - 2025-03-26

### Changed

- Refactored clock reset.
- Refactored clock to set clock tics per second.
- Using Trick message_publish with message level instead of send_hs for console output.
- Moved exit_freeze job to the last phase to ensure it runs last because it does a clock reset.
- Workaround swig 3.x and 4.0 issue with sim object instance variable references by making them private.


## [v3.1.14] - 2025-03-23

### Changed

- Fixed Trick clock reset time when using CTE.


## [v3.1.13] - 2025-03-22

### Added

- Added support for TSync CTE card.
- Added check for time padding against freeze frame time if CTE is used.

### Changed

- Add a check for how long we wait for CTE go to run time and detect if the time padding is not enough.
- Accounting for Trick freeze frame time and padding time constraints.
- Merge pull requests #167 and #168.


## [v3.1.12] - 2025-03-17

### Added

- Added a Central Timing Equipment (CTE) example SpaceFOM simulation (issue #165).

### Changed

- Detect when multiphase init sync-points are not configured (issue #163).
- Account for the Trick time-tic value when using clock_wall_time (issue #164).
- Updated TrickHLA clock implementation.


## [v3.1.11] - 2025-02-26

### Added

- Added initial implementation of building the relative state transformation from the transform path.
- Added the SISO SIW Time Constraints paper.

### Changed

- Adjusted the JEOD examples input file to support new JEOD 5.2 input file requirements.
- Fixed some LRTree and RefFrame issues.
- Fixed interface issue with JEODRefFrameState.


## [v3.1.10] - 2025-01-29

### Changed

- Updated some messages to be warnings and not errors since we don't terminate.
- Calculate a reasonable padding time based on the least common time step (LCTS) (issue #162).


## [v3.1.9] - 2025-01-23

### Changed

- Use a fixed pause time for MTR shutdown propagation for SpaceFOM (issue #161).
- Updated check on time padding of 2 seconds or more does not need to be 3 times LCTS.


## [v3.1.8] - 2025-01-18

### Added

- Added a check for time constraints coming out of freeze to reverify the Trick software frame against the LCTS.
- Trick software frame needs to be configured for all federates to be able to freeze on a LCTS boundary (issue #160).


## [v3.1.7] - 2025-01-17

### Changed

- Removed unnecessary setting of the Trick software frame in some input files.


## [v3.1.6] - 2025-01-16

### Changed

- Freeze time is sim-time on LCTS boundary computed from scenario time (issue #159).
- Updated check for realtime for time constraint verification.


## [v3.1.5] - 2025-01-15

### Added

- Added header dependencies explicitly to sim-module for forward declared classes that don't have the header file included anywhere.

### Changed

- Updated sim-module includes to work around Trick issue of not generating source code for all classes for friend void init_attr<namespace>__<class name>();


## [v3.1.4] - 2025-01-12

### Changed

- Renamed RT local variable to address collision with macro in users code (issue #157).


## [v3.1.3] - 2025-01-10

### Added

- Added script for running clang-tidy on TrickHLA source code.
- Added function to determine if an instance handle is a match to the object.

### Changed

- Skip some time constraint verifications if thread not associated or association is disabled.


## [v3.1.2] - 2024-12-26

### Changed

- Refactored verification of timing constraints.


## [v3.1.1] - 2024-12-20

### Changed

- Ensure freeze time is on LCTS boundary (issue #156).
- Version v3.1.1 release including LCTS freeze boundary fix (issue #156).


## [v3.1.0] - 2024-12-09

### Added

- Added blocking I/O APIs.
- Added wheelbot TrickHLA example simulation.
- Created new Labeled Rooted Tree (LRTree) collection of base classes.
- Added support for a designated late joiner federate.
- Added SpaceFOM sine child thread example simulation.
- Added SpaceFOM zero-lookahead example simulation.
- Added Distributed Interface (DistIf) example simulation and models.
- Added checks for LCTS, lookahead, software frame and initialization of HLA cycle time.
- Added blocking IO example simulation.
- Added support of ownership transfer at initialization.
- Added ownership transfer at initialization example simulation.

### Changed

- Changed minimum Trick version to 19.0.0.
- Make the reflected data queue permanent and not a compile time feature (Issue #135).
- Updated SIM_IMSim_Test to build.
- Fixed the ask_MOM_for_federate_names() function from clearing the joined_federate_name_map causing reflections to fail.
- Fixed multiphase sync points not being appended in the list.
- Refactored synchronization point implementation.
- Fix for Pitch pRTI include path change.
- Refactored mutex locked critical sections for TAR and TARA.
- Fixed the handling of sync-points for a designated late joiner.
- Handle the case of an unknown sync-point getting announced more than one time.
- Updated build instructions (issue #152).

### Removed

- Dropping support for DIS and DSES.


## [v3.0.0] - 2024-12-08

### Added

- Added DSES documentation approved for public release (STIDAA #20205002034).
- Adding static code analysis script for the flawfinder tool (issue #8).
- Added a FESFA template to the SpaceFOM documents.
- Created a SpaceFOM FCD Template document.

### Changed

- Updated documentation.
- Fixed issue #6, for job_cycle_time being overridden in error message.
- Resolved null pointer checks found by cppcheck v2.1 (issue #7).
- Fixed obsolete usleep call found by flawfinder (issue #9).
- Moved cppcheck suppressions to inline in the code instead of in a file (issue #11).
- Indent case labels (issue #12).
- Refactor save and restore label types (issue #13).
- Fix shutdown problems (issue #15( and fix debug output to include package and class names (issue #16).
- Revert back to using non-c++11 API for std::ifstream::open (issue #18).
- Update script to format models and format code (issue #19).
- Rename get_double_time() to get_time_in_seconds() (issue #23).
- Refactored mutex's (issue #27).
- Refactored cyclic data functions (issue #30 and issue #31) and coding style (issue #29).
- Only update ExCO on freeze exit (issue #33) and code style fix for booleans
- Refactor spin-lock wait code into its own class (issue #35).
- Move debug_level and DebugHandler init to Federate (issue #41).
- Updated blocking-cyclic-read Object setting for blocking I/O (issue #45).
- Improved spin-lock performance (issue #47).
- Fixed time_management flag to work as expected (issue #49).
- Handle multiple attribute reflections per cyclic receive (#91).
- Fix sync-point concurrency (issue #94).
- Fixed problem of Freeze button on Trick Sim Control panel being pressed more than once resulting in more than one expected freeze action.


## [d3.0.0] - 2024-11-22


[Unreleased]: https://github.com/nasa/TrickHLA/compare/v3.1.18...HEAD
[v3.1.18]: https://github.com/nasa/TrickHLA/compare/v3.1.17...v3.1.18
[v3.1.17]: https://github.com/nasa/TrickHLA/compare/v3.1.16...v3.1.17
[v3.1.16]: https://github.com/nasa/TrickHLA/compare/v3.1.15...v3.1.16
[v3.1.15]: https://github.com/nasa/TrickHLA/compare/v3.1.14...v3.1.15
[v3.1.14]: https://github.com/nasa/TrickHLA/compare/v3.1.13...v3.1.14
[v3.1.13]: https://github.com/nasa/TrickHLA/compare/v3.1.12...v3.1.13
[v3.1.12]: https://github.com/nasa/TrickHLA/compare/v3.1.11...v3.1.12
[v3.1.11]: https://github.com/nasa/TrickHLA/compare/v3.1.10...v3.1.11
[v3.1.10]: https://github.com/nasa/TrickHLA/compare/v3.1.9...v3.1.10
[v3.1.9]: https://github.com/nasa/TrickHLA/compare/v3.1.8...v3.1.9
[v3.1.8]: https://github.com/nasa/TrickHLA/compare/v3.1.7...v3.1.8
[v3.1.7]: https://github.com/nasa/TrickHLA/compare/v3.1.6...v3.1.7
[v3.1.6]: https://github.com/nasa/TrickHLA/compare/v3.1.5...v3.1.6
[v3.1.5]: https://github.com/nasa/TrickHLA/compare/v3.1.4...v3.1.5
[v3.1.4]: https://github.com/nasa/TrickHLA/compare/v3.1.3...v3.1.4
[v3.1.3]: https://github.com/nasa/TrickHLA/compare/v3.1.2...v3.1.3
[v3.1.2]: https://github.com/nasa/TrickHLA/compare/v3.1.1...v3.1.2
[v3.1.1]: https://github.com/nasa/TrickHLA/compare/v3.1.0...v3.1.1
[v3.1.0]: https://github.com/nasa/TrickHLA/compare/v3.0.0...v3.1.0
[v3.0.0]: https://github.com/nasa/TrickHLA/compare/d3.0.0...v3.0.0
[d3.0.0]: https://github.com/nasa/TrickHLA/releases/tag/d3.0.0
