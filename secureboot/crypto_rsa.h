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
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
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
//    MStar Software in conjunction with your or your customer's product
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

#ifndef __CRYPTO_RSA_H__
#define __CRYPTO_RSA_H__

#include "bignum.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
// Type definition
//-------------------------------------------------------------------------------------------------
typedef unsigned int                DIGIT_T;
#define DIGIT_S                     sizeof(DIGIT_T)

#define RSA_KEY_DIGI_LEN            (256)                                // DIGIT_T count

#define REG_OTPKEY_RSA_N_LEN        (64*DIGIT_S)                        // 2048
#define REG_OTPKEY_RSA_E_LEN        (1 *DIGIT_S)                        // 32

typedef struct _Integer2048
{
    unsigned char                   Byte[256];
} Integer2048;

typedef struct _Integer128
{
    unsigned char                   Byte[16];
} Integer128;

typedef struct _Integer32
{
    unsigned char                   Byte[4];
} Integer32;

#define POLARSSL_ERR_RSA_BAD_INPUT_DATA                    -0x0400
#define POLARSSL_ERR_RSA_INVALID_PADDING                   -0x0410
#define POLARSSL_ERR_RSA_KEY_GEN_FAILED                    -0x0420
#define POLARSSL_ERR_RSA_KEY_CHECK_FAILED                  -0x0430
#define POLARSSL_ERR_RSA_PUBLIC_FAILED                     -0x0440
#define POLARSSL_ERR_RSA_PRIVATE_FAILED                    -0x0450
#define POLARSSL_ERR_RSA_VERIFY_FAILED                     -0x0460
#define POLARSSL_ERR_RSA_OUTPUT_TO_LARGE                   -0x0470
#define POLARSSL_ERR_RSA_ENCODING_ERROR                    -0x0480
#define POLARSSL_ERR_RSA_INCONSISTENT                      -0x0490
#define POLARSSL_ERR_RSA_RNG_FAILED                        -0x4480

/*
 * PKCS#1 constants
 */
#define RSA_RAW         0
#define RSA_MD2         2
#define RSA_MD4         3
#define RSA_MD5         4
#define RSA_SHA1        5
#define RSA_SHA256      6

#define RSA_PUBLIC      0
#define RSA_PRIVATE     1

#define RSA_PKCS_V15    0
#define RSA_PKCS_V21    1

#define RSA_SIGN        1
#define RSA_CRYPT       2

/*
 * DigestInfo ::= SEQUENCE {
 *   digestAlgorithm DigestAlgorithmIdentifier,
 *   digest Digest }
 *
 * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
 *
 * Digest ::= OCTET STRING
 */
#define ASN1_STR_CONSTRUCTED_SEQUENCE   "\x30"
#define ASN1_STR_NULL                   "\x05"
#define ASN1_STR_OID                    "\x06"
#define ASN1_STR_OCTET_STRING           "\x04"

#define OID_DIGEST_ALG_MDX              "\x2A\x86\x48\x86\xF7\x0D\x02\x00"
#define OID_HASH_ALG_SHA1               "\x2b\x0e\x03\x02\x1a"
#define OID_HASH_ALG_SHA2X              "\x60\x86\x48\x01\x65\x03\x04\x02\x00"

#define OID_ISO_MEMBER_BODIES           "\x2a"
#define OID_ISO_IDENTIFIED_ORG          "\x2b"


#define ASN1_HASH_MDX                       \
    "\x30\x20\x30\x0C\x06\x08\x2A\x86\x48"  \
    "\x86\xF7\x0D\x02\x00\x05\x00\x04\x10"

#define ASN1_HASH_SHA1                      \
    "\x30\x21\x30\x09\x06\x05\x2B\x0E\x03"  \
    "\x02\x1A\x05\x00\x04\x14"

#define ASN1_HASH_SHA2X                         \
    ASN1_STR_CONSTRUCTED_SEQUENCE "\x11"        \
      ASN1_STR_CONSTRUCTED_SEQUENCE "\x0d"      \
        ASN1_STR_OID "\x09"                     \
      OID_HASH_ALG_SHA2X                        \
        ASN1_STR_NULL "\x00"                    \
      ASN1_STR_OCTET_STRING "\x00"

#define HEAD_NUM 32

typedef struct _rawdata_header_ {
    unsigned int offset;
    unsigned int size;
    unsigned int crc;
    char flag;
    char res1;
    char res2;
    char res3;
    char name[112];
}rawdata_header;

/**
 * \brief           RSA context structure
 */
typedef struct
{
    int ver;                    /*!<  always 0            */
    int len;                    /*!<  size(N) in chars    */

    mpi N;                        /*!<  public modulus    */
    mpi E;                        /*!<  public exponent    */

    mpi D;                        /*!<  private exponent    */
    mpi P;                        /*!<  1st prime factor    */
    mpi Q;                        /*!<  2nd prime factor    */
    mpi DP;                     /*!<  D % (P - 1)        */
    mpi DQ;                     /*!<  D % (Q - 1)        */
    mpi QP;                     /*!<  1 / (Q % P)        */

    mpi RN;                     /*!<  cached R^2 mod N    */
    mpi RP;                     /*!<  cached R^2 mod P    */
    mpi RQ;                     /*!<  cached R^2 mod Q    */

    int padding;                /*!<  1.5 or OAEP/PSS    */
    int hash_id;                /*!<  hash identifier    */
    int (*f_rng)(void *);        /*!<  RNG function        */
    void *p_rng;                /*!<  RNG parameter     */
}
rsa_context;

//-------------------------------------------------------------------------------------------------
// Extern prototype
//-------------------------------------------------------------------------------------------------

int _mpConvFromOctets(DIGIT_T a[], int ndigits, const unsigned char *c, int nbytes);
int _mpModExp(DIGIT_T yout[], const DIGIT_T x[], const DIGIT_T e[], const DIGIT_T m[], int ndigits);
int _mpEqual(const DIGIT_T a[], const DIGIT_T b[], int ndigits);
int _mpSquare(DIGIT_T w[], const DIGIT_T x[], int ndigits);
int _mpMultiply(DIGIT_T w[], const DIGIT_T u[], const DIGIT_T v[], int ndigits);
int _mpDivide(DIGIT_T q[], DIGIT_T r[], const DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits);
DIGIT_T _mpShortDiv(DIGIT_T q[], const DIGIT_T u[], DIGIT_T v, int ndigits);
DIGIT_T _mpShiftLeft(DIGIT_T a[], const DIGIT_T *b, int shift, int ndigits);
DIGIT_T _mpShiftRight(DIGIT_T a[], const DIGIT_T b[], int shift, int ndigits);
int _spMultiply(DIGIT_T p[2], DIGIT_T x, DIGIT_T y);
DIGIT_T _spDivide(DIGIT_T *q, DIGIT_T *r, const DIGIT_T u[2], DIGIT_T v);

int RSA2048_encrypt(char *digest, unsigned char *rsa_sign);
int RSA2048_decrypt(unsigned char *Signature, unsigned char *PublicKey_N, unsigned char *PublicKey_E, unsigned char *Sim_SignOut);

void RSA_test(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __CRYPTO_RSA_H__