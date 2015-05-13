//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef _MSC_VER
#include <basetsd.h>
#if (_MSC_VER <= 1200)
typedef   signed short  int16_t;
typedef unsigned short uint16_t;
#else
typedef  INT16  int16_t;
typedef UINT16 uint16_t;
#endif
typedef  INT32  int32_t;
typedef  INT64  int64_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
#else
#include <inttypes.h>
#endif

#define POLARSSL_ERR_MPI_FILE_IO_ERROR                     -0x0002
#define POLARSSL_ERR_MPI_BAD_INPUT_DATA                    -0x0004
#define POLARSSL_ERR_MPI_INVALID_CHARACTER                 -0x0006
#define POLARSSL_ERR_MPI_BUFFER_TOO_SMALL                  -0x0008
#define POLARSSL_ERR_MPI_NEGATIVE_VALUE                    -0x000A
#define POLARSSL_ERR_MPI_DIVISION_BY_ZERO                  -0x000C
#define POLARSSL_ERR_MPI_NOT_ACCEPTABLE                    -0x000E
#define POLARSSL_ERR_MPI_MALLOC_FAILED                     -0x0010  /**< Memory allocation failed. */

#define MPI_CHK(f) if( ( ret = f ) != 0 ) goto cleanup

/*
 * Maximum size MPIs are allowed to grow to in number of limbs.
 */
#define POLARSSL_MPI_MAX_LIMBS                             10000

/*
 * Maximum window size used for modular exponentiation. Default: 6
 * Minimum value: 1. Maximum value: 6.
 *
 * Result is an array of ( 2 << POLARSSL_MPI_WINDOW_SIZE ) MPIs used
 * for the sliding window calculation. (So 64 by default)
 *
 * Reduction in size, reduces speed.
 */
#define POLARSSL_MPI_WINDOW_SIZE                           6        /**< Maximum windows size used. */

/*
 * Maximum size of MPIs allowed in bits and bytes for user-MPIs.
 * ( Default: 512 bytes => 4096 bits, Maximum tested: 2048 bytes => 16384 bits )
 *
 * Note: Calculations can results temporarily in larger MPIs. So the number
 * of limbs required (POLARSSL_MPI_MAX_LIMBS) is higher.
 */
#define POLARSSL_MPI_MAX_SIZE                              512      /**< Maximum number of bytes for usable MPIs. */
#define POLARSSL_MPI_MAX_BITS                              ( 8 * POLARSSL_MPI_MAX_SIZE )    /**< Maximum number of bits for usable MPIs. */

/*
 * When reading from files with mpi_read_file() and writing to files with
 * mpi_write_file() the buffer should have space
 * for a (short) label, the MPI (in the provided radix), the newline
 * characters and the '\0'.
 *
 * By default we assume at least a 10 char label, a minimum radix of 10
 * (decimal) and a maximum of 4096 bit numbers (1234 decimal chars).
 * Autosized at compile time for at least a 10 char label, a minimum radix
 * of 10 (decimal) for a number of POLARSSL_MPI_MAX_BITS size.
 *
 * This used to be statically sized to 1250 for a maximum of 4096 bit
 * numbers (1234 decimal chars).
 *
 * Calculate using the formula:
 *  POLARSSL_MPI_RW_BUFFER_SIZE = ceil(POLARSSL_MPI_MAX_BITS / ln(10) * ln(2)) +
 *                                LabelSize + 6
 */
#define POLARSSL_MPI_MAX_BITS_SCALE100          ( 100 * POLARSSL_MPI_MAX_BITS )
#define LN_2_DIV_LN_10_SCALE100                 332
#define POLARSSL_MPI_RW_BUFFER_SIZE             ( ((POLARSSL_MPI_MAX_BITS_SCALE100 + LN_2_DIV_LN_10_SCALE100 - 1) / LN_2_DIV_LN_10_SCALE100) + 10 + 6 )

/*
 * Define the base integer type, architecture-wise
 */
#if defined(POLARSSL_HAVE_INT8)
typedef   signed char  t_sint;
typedef unsigned char  t_uint;
typedef uint16_t       t_udbl;
#define POLARSSL_HAVE_UDBL
#else
#if defined(POLARSSL_HAVE_INT16)
typedef  int16_t t_sint;
typedef uint16_t t_uint;
typedef uint32_t t_udbl;
#define POLARSSL_HAVE_UDBL
#else
  #if ( defined(_MSC_VER) && defined(_M_AMD64) )
    typedef  int64_t t_sint;
    typedef uint64_t t_uint;
  #else
    #if ( defined(__GNUC__) && (                          \
          defined(__amd64__) || defined(__x86_64__)    || \
          defined(__ppc64__) || defined(__powerpc64__) || \
          defined(__ia64__)  || defined(__alpha__)     || \
          (defined(__sparc__) && defined(__arch64__))  || \
          defined(__s390x__) ) )
       typedef  int64_t t_sint;
       typedef uint64_t t_uint;
       typedef unsigned int t_udbl __attribute__((mode(TI)));
       #define POLARSSL_HAVE_UDBL
    #else
       typedef  int32_t t_sint;
       typedef uint32_t t_uint;
       #if ( defined(_MSC_VER) && defined(_M_IX86) )
         typedef uint64_t t_udbl;
         #define POLARSSL_HAVE_UDBL
       #else
         #if defined( POLARSSL_HAVE_LONGLONG )
           typedef unsigned long long t_udbl;
           #define POLARSSL_HAVE_UDBL
         #endif
       #endif
    #endif
  #endif
#endif
#endif

/**
 * \brief          MPI structure
 */
typedef struct
{
    int s;              /*!<  integer sign      */
    size_t n;           /*!<  total # of limbs  */
    t_uint *p;          /*!<  pointer to limbs  */
}
mpi;

/**
 * \brief          Initialize one or more mpi
 */
void mpi_init( mpi *X );

/**
 * \brief          Unallocate one or more mpi
 */
void mpi_free( mpi *X );

/**
 * \brief          Enlarge to the specified number of limbs
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_grow( mpi *X, size_t nblimbs );

/**
 * \brief          Copy the contents of Y into X
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_copy( mpi *X, const mpi *Y );

/**
 * \brief          Swap the contents of X and Y
 */
void mpi_swap( mpi *X, mpi *Y );

/**
 * \brief          Set value from integer
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_lset( mpi *X, t_sint z );

/**
 * \brief          Get a specific bit from X
 *
 * \param X        MPI to use
 * \param pos      Zero-based index of the bit in X
 *
 * \return         Either a 0 or a 1
 */
int mpi_get_bit( const mpi *X, size_t pos );

/**
 * \brief          Set a bit of X to a specific value of 0 or 1
 *
 * \note           Will grow X if necessary to set a bit to 1 in a not yet
 *                 existing limb. Will not grow if bit should be set to 0
 *
 * \param X        MPI to use
 * \param pos      Zero-based index of the bit in X
 * \param val      The value to set the bit to (0 or 1)
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_MALLOC_FAILED if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if val is not 0 or 1
 */
int mpi_set_bit( mpi *X, size_t pos, unsigned char val );

/**
 * \brief          Return the number of zero-bits before the least significant
 *                 '1' bit
 *
 * Note: Thus also the zero-based index of the least significant '1' bit
 *
 * \param X        MPI to use
 */
size_t mpi_lsb( const mpi *X );

/**
 * \brief          Return the number of most significant bits
 */
size_t mpi_msb( const mpi *X );

/**
 * \brief          Return the total size in bytes
 */
size_t mpi_size( const mpi *X );

/**
 * \brief          Import from an ASCII string
 *
 * \param X        destination mpi
 * \param radix    input numeric base
 * \param s        null-terminated string buffer
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code
 */
int mpi_read_string( mpi *X, int radix, const char *s );

/**
 * \brief          Export into an ASCII string
 *
 * \param X        source mpi
 * \param radix    output numeric base
 * \param s        string buffer
 * \param slen     string buffer size
 *
 * \return         0 if successful, or an POLARSSL_ERR_MPI_XXX error code
 *
 * \note           Call this function with *slen = 0 to obtain the
 *                 minimum required buffer size in *slen.
 */
int mpi_write_string( const mpi *X, int radix, char *s, size_t *slen );


/**
 * \brief          Import X from unsigned binary data, big endian
 *
 * \param X        destination mpi
 * \param buf      input buffer
 * \param buflen   input buffer size
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_read_binary( mpi *X, const unsigned char *buf, size_t buflen );

/**
 * \brief          Export X into unsigned binary data, big endian
 *
 * \param X        source mpi
 * \param buf      output buffer
 * \param buflen   output buffer size
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_BUFFER_TOO_SMALL if buf isn't large enough
 *
 * \note           Call this function with *buflen = 0 to obtain the
 *                 minimum required buffer size in *buflen.
 */
int mpi_write_binary( const mpi *X, unsigned char *buf, size_t buflen );

/**
 * \brief          Left-shift: X <<= count
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_shift_l( mpi *X, size_t count );

/**
 * \brief          Right-shift: X >>= count
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_shift_r( mpi *X, size_t count );

/**
 * \brief          Compare unsigned values
 *
 * \return         1 if |X| is greater than |Y|,
 *                -1 if |X| is lesser  than |Y| or
 *                 0 if |X| is equal to |Y|
 */
int mpi_cmp_abs( const mpi *X, const mpi *Y );

/**
 * \brief          Compare signed values
 *
 * \return         1 if X is greater than Y,
 *                -1 if X is lesser  than Y or
 *                 0 if X is equal to Y
 */
int mpi_cmp_mpi( const mpi *X, const mpi *Y );

/**
 * \brief          Compare signed values
 *
 * \return         1 if X is greater than z,
 *                -1 if X is lesser  than z or
 *                 0 if X is equal to z
 */
int mpi_cmp_int( const mpi *X, t_sint z );

/**
 * \brief          Unsigned addition: X = |A| + |B|
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_abs( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Unsigned substraction: X = |A| - |B|
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_NEGATIVE_VALUE if B is greater than A
 */
int mpi_sub_abs( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed addition: X = A + B
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed substraction: X = A - B
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_sub_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Signed addition: X = A + b
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_add_int( mpi *X, const mpi *A, t_sint b );

/**
 * \brief          Signed substraction: X = A - b
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_sub_int( mpi *X, const mpi *A, t_sint b );

/**
 * \brief          Baseline multiplication: X = A * B
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_mul_mpi( mpi *X, const mpi *A, const mpi *B );

/**
 * \brief          Baseline multiplication: X = A * b
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_mul_int( mpi *X, const mpi *A, t_sint b );

/**
 * \brief          Division by mpi: A = Q * B + R
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0
 *
 * \note           Either Q or R can be NULL.
 */
int mpi_div_mpi( mpi *Q, mpi *R, const mpi *A, const mpi *B );

/**
 * \brief          Division by int: A = Q * b + R
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 *
 * \note           Either Q or R can be NULL.
 */
int mpi_div_int( mpi *Q, mpi *R, const mpi *A, t_sint b );

/**
 * \brief          Modulo: R = A mod B
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if B == 0
 */
int mpi_mod_mpi( mpi *R, const mpi *A, const mpi *B );

/**
 * \brief          Modulo: r = A mod b
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_DIVISION_BY_ZERO if b == 0
 */
int mpi_mod_int( t_uint *r, const mpi *A, t_sint b );

/**
 * \brief          Sliding-window exponentiation: X = A^E mod N
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or even
 *
 * \note           _RR is used to avoid re-computing R*R mod N across
 *                 multiple calls, which speeds up things a bit. It can
 *                 be set to NULL if the extra performance is unneeded.
 */
int mpi_exp_mod( mpi *X, const mpi *A, const mpi *E, const mpi *N, mpi *_RR );

/**
 * \brief          Fill an MPI X with size bytes of random
 *
 * \param X        Destination MPI
 * \param size     Size in bytes
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 *
 * \return         0 if successful,
 *                 POLARSSL_ERR_MPI_MALLOC_FAILED if memory allocation failed
 */
int mpi_fill_random( mpi *X, size_t size,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng );

/**
 * \brief          Greatest common divisor: G = gcd(A, B)
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed
 */
int mpi_gcd( mpi *G, const mpi *A, const mpi *B );

/**
 * \brief          Modular inverse: X = A^-1 mod N
 *
 * \return         0 if successful,
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if N is negative or nil
 *                 POLARSSL_ERR_MPI_NOT_ACCEPTABLE if A has no inverse mod N
 */
int mpi_inv_mod( mpi *X, const mpi *A, const mpi *N );

/**
 * \brief          Miller-Rabin primality test
 *
 * \return         0 if successful (probably prime),
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_NOT_ACCEPTABLE if X is not prime
 */
int mpi_is_prime( mpi *X,
                  int (*f_rng)(void *, unsigned char *, size_t),
                  void *p_rng );

/**
 * \brief          Prime number generation
 *
 * \param X        destination mpi
 * \param nbits    required size of X in bits
 * \param dh_flag  if 1, then (X-1)/2 will be prime too
 * \param f_rng    RNG function
 * \param p_rng    RNG parameter
 *
 * \return         0 if successful (probably prime),
 *                 1 if memory allocation failed,
 *                 POLARSSL_ERR_MPI_BAD_INPUT_DATA if nbits is < 3
 */
int mpi_gen_prime( mpi *X, size_t nbits, int dh_flag,
                   int (*f_rng)(void *, unsigned char *, size_t),
                   void *p_rng );

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mpi_self_test( int verbose );

#ifdef __cplusplus
}
#endif