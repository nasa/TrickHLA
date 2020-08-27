/*!
@file TrickHLA/CompileConfig.hh
@ingroup TrickHLA
@brief TrickHLA compile time configuration settings.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, February 2009, --, Consolidated config settings.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_COMPILE_CONFIG_H_
#define _TRICKHLA_COMPILE_CONFIG_H_

// For blocking cyclic reads, do a check on the send and received data counts.
// Default: NO_THLA_CHECK_SEND_AND_RECEIVE_COUNTS
#define NO_THLA_CHECK_SEND_AND_RECEIVE_COUNTS

// We support 3 different techniques to block waiting for data.
// 1) Thread wait on a conditional variable with a timeout when
//    THLA_THREAD_WAIT_FOR_DATA and THLA_THREAD_TIMED_WAIT_FOR_DATA are defined.
// 2) Thread wait on a conditional variable with no timeout when
//    THLA_THREAD_WAIT_FOR_DATA is defined and THLA_THREAD_TIMED_WAIT_FOR_DATA
//    is not defined.
// 3) Spin-lock with timeout when THLA_THREAD_WAIT_FOR_DATA is not defined.
//
// Set to THLA_THREAD_WAIT_FOR_DATA to use a thread conditional variable wait
// for data to arrive.
// Default: NO_THLA_THREAD_WAIT_FOR_DATA (i.e. Use a spin-lock.)
#define NO_THLA_THREAD_WAIT_FOR_DATA

// Set to THLA_THREAD_TIMED_WAIT_FOR_DATA to use a timed thread wait so that if
// data does not come in the thread wait will timeout.
// Default: THLA_THREAD_TIMED_WAIT_FOR_DATA
#define THLA_THREAD_TIMED_WAIT_FOR_DATA

// Set to THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA to use a 10 second timeout
// while waiting for data during a blocked read.
// Default: THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA
#define THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA

// Set to THLA_QUEUE_REFLECTED_ATTRIBUTES to enable the queue of the reflected
// attributes values.
// Default: THLA_QUEUE_REFLECTED_ATTRIBUTES
#define THLA_QUEUE_REFLECTED_ATTRIBUTES

// Set to THLA_INTRAFRAME_BLOCKING_READ_CONFIG to enable the settings for
// blocking HLA data reads within the same frame.
// Default: THLA_INTRAFRAME_BLOCKING_READ_CONFIG
#define THLA_INTRAFRAME_BLOCKING_READ_CONFIG
#ifdef THLA_INTRAFRAME_BLOCKING_READ_CONFIG

// Make sure object time logging is not defined since we don't want to
// use that feature.
#   ifdef THLA_OBJECT_TIME_LOGGING
#      undef THLA_OBJECT_TIME_LOGGING
#   endif

// Use a 10-second timeout while waiting for the data to arrive.
#   ifndef THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA
#      define THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA
#   endif

// Configure to use the spin-lock, which is slightly faster than the
// thread wait with timeout.
#   ifdef THLA_THREAD_WAIT_FOR_DATA
#      undef THLA_THREAD_WAIT_FOR_DATA
#   endif

#   if ( defined( __i386__ ) || defined( __x86_64__ ) )
#      if defined( __APPLE__ )
// For a Mac, use usleep() for the spin-lock delay instead of using
// multiple NOP assembly instructions, which is much faster since it
// yields the CPU so that the Fed-Ambassador callback thread has a
// chance to run.
#         ifndef THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#            define THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#         endif
#      else
// For Linux, use multiple NOP assembly instructions for the spin-lock
// delay instead of using usleep() because it is much faster.
#         ifdef THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#            undef THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#         endif
#      endif
#   else
// Otherwise for an unknown Processor, use usleep() for the spin-lock delay.
#      ifndef THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#         define THLA_USLEEP_DELAY_FOR_SPIN_LOCK
#      endif
#   endif

#endif // THLA_INTRAFRAME_BLOCKING_READ_CONFIG

// Insert a compile time error if an unsupported version of Trick 17 is used.
// Minimum supported Trick 17 version: 17.5.0
#define MIN_TRICK_VER 17  // Set to the minimum supported Trick Major version.
#define MIN_TRICK_MINOR 5 // Set to the minimum supported Trick Minor version.
#define MIN_TRICK_PATCH 0 // Set to the minimum supported Trick Patch version.
// Make sure the TRICK_VER compiler define is set.
#if ( !defined( TRICK_VER ) )
#   error "The Trick version variable \"TRICK_VER\" is not set!"
#   error "The minimum acceptable version of Trick is 17.5.0!"
#endif
// Check for the minimum major version of Trick.
#if ( TRICK_VER < MIN_TRICK_VER )
#   error "The minimum acceptable version of Trick is 17.5.0!"
#else // Check the minor version if set.
#   if ( TRICK_VER == MIN_TRICK_VER )
#      if ( defined( TRICK_MINOR ) && ( TRICK_MINOR < MIN_TRICK_MINOR ) )
#         error "The minimum acceptable version of Trick is 17.5.0!"
#      else // Check the patch version if set.
#         if ( TRICK_MINOR == MIN_TRICK_MINOR ) && ( defined( TRICK_PATCH ) && ( TRICK_PATCH < MIN_TRICK_PATCH ) )
#            error "The minimum acceptable version of Trick is 17.5.0!"
#         endif
#      endif
#   endif
#endif
#undef MIN_TRICK_PATCH
#undef MIN_TRICK_MINOR
#undef MIN_TRICK_VER

// Define newlines, this was meant to abstract out the difference between how
// new-lines were handled in send_hs between Trick 07 and newer Trick versions.
#define THLA_ENDL std::endl
#define THLA_NEWLINE '\n'

// The Pitch Runtime Infrastructure (RTI) API calls will cause the
// floating-point control word register to change the Mantissa Precision Control
// Bits from 64-bit (extended double precision) to 53-bit (double precision)
// because the Pitch RTI runs Java under the hood which causes the problem.
// This is only supported for Intel CPU's.
#if ( !defined( FPU_CW_PROTECTION ) && ( defined( __i386__ ) || defined( __x86_64__ ) ) )
#   define FPU_CW_PROTECTION
#endif

// Define TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION to enabled FPU control
// word tests that have been sprinkled throughout the TrickHLA code. This only
// has an effect if FPU_CW_PROTECTION is defined as well.
// Default: NO_TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION
#define NO_TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION

// Define TRICKHLA_ENABLE_FOM_DUMP to enable a debug dump of the parsed FOMs.
// Default: NO_TRICKHLA_ENABLE_FOM_DUMP
#define NO_TRICKHLA_ENABLE_FOM_DUMP

#endif // _TRICKHLA_COMPILE_CONFIG_H_
