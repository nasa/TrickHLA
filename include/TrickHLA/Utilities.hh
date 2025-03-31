/*!
@file TrickHLA/Utilities.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA utilities.

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

@tldh
@trick_link_dependency{../../source/TrickHLA/Utilities.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, TrickHLA, Aug 2006, --, DSES TrickHLA Utilities}
@rev_entry{Dan Dexter, L3, TrickHLA, May 2008, --, IMSim: Added FPU code word protection macros}
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, December 2008, --, IMSim: Added MacOS X support.}
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Sept 2010, --, Added Mac FPU control word support.}
@rev_entry{Danny Strauss, L3, TrickHLA, June 2012, --, Add version to THLA simobject}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_UTILITIES_HH
#define TRICKHLA_UTILITIES_HH

// System include files.
#include <string>

// Trick include files.
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Types.hh"

// Certain Pitch RTI calls can cause the floating-point control word register
// to change the Mantissa Precision Control Bits from 64-bit (extended double
// precision) to 53-bit (double precision). These macros can be used to save
// and restore the FPU control word value. Note: The last semicolon is left off
// so that the macro can be used with a semicolon. This is only supported for
// Linux and Intel Macs.
//
// Precision-Control values (bits 8 and 9 of the FPU control-word)
#define _FPU_PC_MASK 0x300
#define _FPU_PC_EXTENDED 0x300
#define _FPU_PC_DOUBLE 0x200
#define _FPU_PC_UNDEFINED 0x100
#define _FPU_PC_SINGLE 0x000
#if ( defined( FPU_CW_PROTECTION ) && ( defined( __i386__ ) || defined( __x86_64__ ) ) && !defined( SWIG ) )
#   if defined( __APPLE__ )
// Mac OS support on Intel CPUs
// Some code below is from the fpu_control.h header file, which is missing
// on the Mac platform. The code has been modified to work on a Mac.
typedef unsigned int fpu_control_t __attribute__( ( __mode__( __HI__ ) ) );
extern fpu_control_t __fpu_control;

#      define _FPU_VALID_BITS 0xFFFF
#      define _FPU_GETCW( cw )                   \
         __asm__ __volatile__( "fnstcw %0"       \
                               : "=m"( *&cw ) ); \
         cw &= _FPU_VALID_BITS
#      define _FPU_SETCW( cw )            \
         cw &= _FPU_VALID_BITS;           \
         __asm__ __volatile__( "fldcw %0" \
                               :          \
                               : "m"( *&cw ) )
#      define TRICKHLA_INIT_FPU_CONTROL_WORD _FPU_GETCW( __fpu_control )
#   else
// Linux support.
#      include <fpu_control.h>               // For FPU Control Word register access.
#      define TRICKHLA_INIT_FPU_CONTROL_WORD // No need to initialize for Linux.
#   endif

#   define TRICKHLA_SAVE_FPU_CONTROL_WORD \
      fpu_control_t _fpu_cw;              \
      _FPU_GETCW( _fpu_cw )
#   define TRICKHLA_RESTORE_FPU_CONTROL_WORD _FPU_SETCW( _fpu_cw )

#   define _FPU_PC_PRINT( pc ) ( ( ( pc & _FPU_PC_MASK ) == _FPU_PC_EXTENDED ) ? "Extended Double-Precision 64-bit" : ( ( ( pc & _FPU_PC_MASK ) == _FPU_PC_DOUBLE ) ? "Double-Precision 53-bit" : ( ( ( pc & _FPU_PC_MASK ) == _FPU_PC_SINGLE ) ? "Single-Precision 24-bit" : "Undefined" ) ) )

#   if defined( TRICKHLA_ENABLE_FPU_CONTROL_WORD_VALIDATION )
#      define TRICKHLA_VALIDATE_FPU_CONTROL_WORD                                            \
         {                                                                                  \
            TRICKHLA_SAVE_FPU_CONTROL_WORD;                                                 \
            if ( ( _fpu_cw & _FPU_PC_MASK ) != ( __fpu_control & _FPU_PC_MASK ) ) {         \
               message_publish( MSG_WARNING, "%s:%d WARNING: We have detected that the current \
Floating-Point Unit (FPU) Control-Word Precision-Control value (%#x: %s) does not \
match the Precision-Control value at program startup (%#x: %s). The change in FPU \
Control-Word Precision-Control could cause the numerical values in your simulation \
to be slightly different in the 7th or 8th decimal place. Please contact the \
TrickHLA team for support.\n",                                                              \
                                __FILE__, __LINE__, ( _fpu_cw & _FPU_PC_MASK ),             \
                                _FPU_PC_PRINT( _fpu_cw ), ( __fpu_control & _FPU_PC_MASK ), \
                                _FPU_PC_PRINT( __fpu_control ) );                           \
            }                                                                               \
         }
#   else
#      define TRICKHLA_VALIDATE_FPU_CONTROL_WORD // FPU Control Word validation not enabled.
#   endif

#else
#   define TRICKHLA_INIT_FPU_CONTROL_WORD     // FPU Control Word protected not enabled.
#   define TRICKHLA_SAVE_FPU_CONTROL_WORD     // FPU Control Word protected not enabled.
#   define TRICKHLA_RESTORE_FPU_CONTROL_WORD  // FPU Control Word protected not enabled.
#   define TRICKHLA_VALIDATE_FPU_CONTROL_WORD // FPU Control Word protected not enabled.
#endif

namespace TrickHLA
{

class Utilities
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<class name>();
   friend void init_attrTrickHLA__Utilities();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Utilities class. */
   Utilities()
   {
      return;
   }

   /*! @brief Destructor for the TrickHLA Utilities class. */
   virtual ~Utilities()
   {
      return;
   }

  public:
   /*! @brief Get the endianness.
    *  @return Either TRICK_BIG_ENDIAN (char)0x00 or TRICK_LITTLE_ENDIAN (char)0x01 */
   static char const get_endianness()
   {
      static char endianness = (char)( std::max( TRICK_LITTLE_ENDIAN, TRICK_BIG_ENDIAN ) + 1 );
      if ( ( endianness != TRICK_LITTLE_ENDIAN ) && ( endianness != TRICK_BIG_ENDIAN ) ) {
         TRICK_GET_BYTE_ORDER( endianness );
      }
      return endianness;
   }

   /*! @brief Determine if the RTI data needs a byteswap before transmission.
    *  @return True if byteswap is needed.
    *  @param  rti_encoding TrickHLA RTI encoding of the data. */
   static bool is_transmission_byteswap( EncodingEnum const rti_encoding );

   /*! @brief Byteswap an short integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static short byteswap_short( short const input );

   /*! @brief Byteswap an unsigned short integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static unsigned short byteswap_unsigned_short( unsigned short const input );

   /*! @brief Byteswap an int integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static int byteswap_int( int const input );

   /*! @brief Byteswap an unsigned int integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static unsigned int byteswap_unsigned_int( unsigned int const input );

   /*! @brief Byteswap a long integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static long byteswap_long( long const input );

   /*! @brief Byteswap an unsigned long integer type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static unsigned long byteswap_unsigned_long( unsigned long const input );

   /*! @brief Byteswap a long long integer type.
    *  @details The long long type is defined in the C99 standard and is at
    *  least 64 bits.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static long long byteswap_long_long( long long const input );

   /*! @brief Byteswap an unsigned long long integer type.
    *  @details The unsigned long long type is defined in the C99 standard and
    *  is at least 64 bits.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static unsigned long long byteswap_unsigned_long_long( unsigned long long const input );

   /*! @brief Byteswap float floating-point type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static float byteswap_float( float const input );

   /*! @brief Byteswap double floating-point type.
    *  @return Byteswap value.
    *  @param  input The input value to byteswap. */
   static double byteswap_double( double const input );

   /*! @brief Round to the next positive multiple of 8.
    *  @return The value rounded to the next positive multiple of 8.
    *  @param  value The value to round to next positive multiple of 8. */
   static size_t const next_positive_multiple_of_8( size_t const value );

   /*! @brief Round to the next positive multiple of N.
    *  @return The value rounded to the next positive multiple of N.
    *  @param  value The value to round to next positive multiple of N.
    *  @param  n The number to round up the value to the next positive multiple of. */
   static size_t const next_positive_multiple_of_N( size_t const value, unsigned int const n );

   /*! @brief Sleep for the specified number of microseconds. The usleep() C
    *  function is obsolete (see CWE-676). Create a wrapper around nanosleep()
    *  to provide the same functionality as usleep().
    *  @return Error code, where a value of 0 is for no error.
    *  @param usec Time to sleep in microseconds. */
   static int micro_sleep( long const usec );

   /*! @brief Return the current TrickHLA version string from the Version.hh
    *  header file.
    *  @return TrickHKLA version string. */
   static std::string const get_version();

   /*! @brief Returns the TrickHLA release date from the Version.hh header file.
    *  @return TrickHLA release date string. */
   static std::string const &get_release_date();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Utilities class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Utilities( Utilities const &rhs );

   /*! @brief Assignment operator for Utilities class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Utilities &operator=( Utilities const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_UTILITIES_HH: Do NOT put anything after this line!
