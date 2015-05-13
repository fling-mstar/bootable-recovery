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

#include "crypto_rsa.h"
#include "crypto_func.h"
#include "crypto_sha.h"
#include "crypto_common.h"

#if defined(BUILD_WITH_SECURE_AESDMA)
//AESDMA
#include <MsTypes.h>
#include <drvAESDMA.h>

static int SwapKey(MS_U32 u32addr,MS_U32 u32len)
{
    MS_U32 i = 0,j = 0;
    MS_U32 num = u32len/(2*4);
    MS_U8 temp = 0;
    for (i = 0;i < num;i++) {
        for (j = 0;j < 4;j++) {
           temp = ((unsigned char *)u32addr)[i*4+j];
           ((unsigned char *)u32addr)[i*4+j] = ((unsigned char *)u32addr)[u32len-(i+1)*4+j];
           ((unsigned char *)u32addr)[u32len-(i+1)*4+j] = temp;
        }
    }
    return 0;
}

static int SwapArray(MS_U32 u32addr,MS_U32 u32len)
{
    MS_U32 i;
    MS_U8 temp;
    if (0 == u32addr || 0 == u32len ) {
        DEBUG("u32addr and u32len can not be 0");
        return -1;
    }
    i = 0;
    temp = 0;
    for (i = 0;i < u32len/2;i++) {
        temp = ((unsigned char *)u32addr)[i];
       ((unsigned char *)u32addr)[i] = ((unsigned char *)u32addr)[u32len-1-i];
       ((unsigned char *)u32addr)[u32len-1-i] = temp;
    }
    return 0;
}

static int RSA2048HWencrypt(unsigned char *Signature, unsigned char *PublicKey_N, unsigned char *PublicKey_D, unsigned char *Sim_SignOut)
{
   DEBUG("IN\n");
   DRVAESDMA_RESULT result;
   DrvAESDMA_RSASig RSASig;
   DrvAESDMA_RSAOut RSAOut;
   DrvAESDMA_RSAKey RSAKey;
   result = DRVAESDMA_OK;
   MDrv_AESDMA_Init(0x00, 0x20000000, 2); // wait the AESDMA.a
   memset(&RSASig,0,sizeof(RSASig));
   memset(&RSAOut,0,sizeof(RSAOut));
   SwapArray((MS_U32)Signature,SHA256_DIGEST_SIZE);
   memcpy((char*)&RSASig+SIGNATURE_LEN-SHA256_DIGEST_SIZE,(char*)Signature,SHA256_DIGEST_SIZE);
   memset(&RSAKey,0,sizeof(RSAKey));
   memcpy((char*)(RSAKey.u32KeyN),(char*)PublicKey_N,RSA_PUBLIC_KEY_N_LEN);
   memcpy((char*)(RSAKey.u32KeyE),(char*)PublicKey_D,RSA_PUBLIC_KEY_N_LEN);
   // little and big endian change,4 bytes one time
   SwapKey((MS_U32)(RSAKey.u32KeyE),RSA_PUBLIC_KEY_N_LEN);
   result=MDrv_RSA_Calculate(&RSASig,&RSAKey,E_DRVAESDMA_RSA2048_PRIVATE);
   if (DRVAESDMA_OK != result) {
      DEBUG("RSA HW encrypt error1\n");
      return -1;
   }
   while(DRVAESDMA_OK != MDrv_RSA_IsFinished());
   result =  MDrv_RSA_Output(E_DRVAESDMA_RSA2048_PRIVATE,&RSAOut);
   if (DRVAESDMA_OK != result) {
      DEBUG("RSA HW encrypt error2\n");
      return -1;
   }
   SwapArray((MS_U32)&RSAOut,SIGNATURE_LEN);
   memcpy((char*)Sim_SignOut,(char*)&RSAOut,sizeof(RSAOut));
   DEBUG("OK\n");
   return 0;
}

static int RSA2048HWdecrypt(unsigned char *Signature, unsigned char *PublicKey_N, unsigned char *PublicKey_E, unsigned char *Sim_SignOut)
{
   DEBUG("IN\n");
   if (NULL == Signature || NULL == PublicKey_N || NULL == PublicKey_E || NULL == Sim_SignOut) {
       DEBUG("parameters Error\n");
       return -1;
   }
   DRVAESDMA_RESULT result = DRVAESDMA_OK;
   DrvAESDMA_RSASig RSASig;
   DrvAESDMA_RSAOut RSAOut;
   MDrv_AESDMA_Init(0x00, 0x20000000, 2); // wait the AESDMA.a
   memcpy(&RSASig,Signature,sizeof(RSASig));
   memset(&RSAOut,0,sizeof(RSAOut));
   DrvAESDMA_RSAKey RSAKey;
   memset(&RSAKey,0,sizeof(RSAKey));
   memcpy((unsigned char*)(RSAKey.u32KeyN),PublicKey_N,RSA_PUBLIC_KEY_N_LEN);
   memcpy((unsigned char*)(RSAKey.u32KeyE),PublicKey_E,RSA_PUBLIC_KEY_E_LEN);
   result=MDrv_RSA_Calculate(&RSASig,&RSAKey,E_DRVAESDMA_RSA2048_PUBLIC);
   if (DRVAESDMA_OK != result) {
      DEBUG("RSA HW decrypt error1\n");
      return -1;
   }
   memset((MS_U8*)&RSAOut,0,sizeof(DrvAESDMA_RSAOut));
   while(DRVAESDMA_OK != MDrv_RSA_IsFinished());
   result= MDrv_RSA_Output(E_DRVAESDMA_RSA2048_PUBLIC, &RSAOut);
   if (DRVAESDMA_OK != result) {
      DEBUG("RSA HW decrypt error2\n");
      return -1;
   }
   memcpy(Sim_SignOut,&RSAOut,sizeof(RSAOut));
   DEBUG("OK\n");
   return 0;
}
#else
//SW
#define MAX_DIGIT                   0xFFFFFFFFUL
#define MAX_HALF_DIGIT              0xFFFFUL    /* NB 'L' */
#define BITS_PER_DIGIT              32
#define HIBITMASK                   0x80000000UL

#define BITS_PER_HALF_DIGIT         (BITS_PER_DIGIT / 2)
#define BYTES_PER_DIGIT             (BITS_PER_DIGIT / 8)

#define LOHALF(x)                   ((DIGIT_T)((x) & MAX_HALF_DIGIT))
#define HIHALF(x)                   ((DIGIT_T)((x) >> BITS_PER_HALF_DIGIT & MAX_HALF_DIGIT))
#define TOHIGH(x)                   ((DIGIT_T)((x) << BITS_PER_HALF_DIGIT))

#define mpNEXTBITMASK(mask, n) do{if(mask==1){mask=HIBITMASK;n--;}else{mask>>=1;}}while(0)

int _spMultiply(DIGIT_T p[2], DIGIT_T x, DIGIT_T y)
{
    /*  Computes p = x * y */
    /*  Ref: Arbitrary Precision Computation
    http://numbers.computation.free.fr/Constants/constants.html

         high    p1                p0     low
        +--------+--------+--------+--------+
        |      x1*y1      |      x0*y0      |
        +--------+--------+--------+--------+
               +-+--------+--------+
               |1| (x0*y1 + x1*y1) |
               +-+--------+--------+
                ^carry from adding (x0*y1+x1*y1) together
                        +-+
                        |1|< carry from adding LOHALF t
                        +-+  to high half of p0
    */
    DIGIT_T x0, y0, x1, y1;
    DIGIT_T t, u, carry;

    /*  Split each x,y into two halves
        x = x0 + B*x1
        y = y0 + B*y1
        where B = 2^16, half the digit size
        Product is
        xy = x0y0 + B(x0y1 + x1y0) + B^2(x1y1)
    */

    x0 = LOHALF(x);
    x1 = HIHALF(x);
    y0 = LOHALF(y);
    y1 = HIHALF(y);

    /* Calc low part - no carry */
    p[0] = x0 * y0;
    /* Calc middle part */
    t = x0 * y1;
    u = x1 * y0;
    t += u;
    if (t < u) {
        carry = 1;
    } else {
        carry = 0;
    }

    /*  This carry will go to high half of p[1]
        + high half of t into low half of p[1] */
    carry = TOHIGH(carry) + HIHALF(t);
    /* Add low half of t to high half of p[0] */
    t = TOHIGH(t);
    p[0] += t;
    if (p[0] < t) {
        carry++;
    }
    p[1] = x1 * y1;
    p[1] += carry;

    return 0;
}

static void _spMultSub(DIGIT_T uu[2], DIGIT_T qhat, DIGIT_T v1, DIGIT_T v0)
{
    /*  Compute uu = uu - q(v1v0)
        where uu = u3u2u1u0, u3 = 0
        and u_n, v_n are all half-digits
        even though v1, v2 are passed as full digits.
    */
    DIGIT_T p0, p1, t;

    p0 = qhat * v0;
    p1 = qhat * v1;
    t = p0 + TOHIGH(LOHALF(p1));
    uu[0] -= t;
    if (uu[0] > MAX_DIGIT - t) {
        uu[1]--;    /* Borrow */
    }
    uu[1] -= HIHALF(p1);
}

DIGIT_T _spDivide(DIGIT_T *q, DIGIT_T *r, const DIGIT_T u[2], DIGIT_T v)
{   /*  Computes quotient q = u / v, remainder r = u mod v
        where u is a double digit
        and q, v, r are single precision digits.
        Returns high digit of quotient (max value is 1)
        CAUTION: Assumes normalised such that v1 >= b/2
        where b is size of HALF_DIGIT
        i.e. the most significant bit of v should be one

        In terms of half-digits in Knuth notation:
        (q2q1q0) = (u4u3u2u1u0) / (v1v0)
        (r1r0) = (u4u3u2u1u0) mod (v1v0)
        for m = 2, n = 2 where u4 = 0
        q2 is either 0 or 1.
        We set q = (q1q0) and return q2 as "overflow"
    */
    DIGIT_T qhat, rhat, t, v0, v1, u0, u1, u2, u3;
    DIGIT_T uu[2], q2;

    /* Check for normalisation */
    if (!(v & HIBITMASK)) {
    /* Stop if assert is working, else return error */
        //assert(v & HIBITMASK);
        *q = *r = 0;
        return MAX_DIGIT;
    }

    /* Split up into half-digits */
    v0 = LOHALF(v);
    v1 = HIHALF(v);
    u0 = LOHALF(u[0]);
    u1 = HIHALF(u[0]);
    u2 = LOHALF(u[1]);
    u3 = HIHALF(u[1]);

    /* Do three rounds of Knuth Algorithm D Vol 2 p272 */

    /*  ROUND 1. Set j = 2 and calculate q2 */
    /*  Estimate qhat = (u4u3)/v1  = 0 or 1
        then set (u4u3u2) -= qhat(v1v0)
        where u4 = 0.
    */
/* [Replaced in Version 2] -->
    qhat = u3 / v1;
    if (qhat > 0) {
        rhat = u3 - qhat * v1;
        t = TOHIGH(rhat) | u2;
        if (qhat * v0 > t)
            qhat--;
    }
<-- */
    qhat = (u3 < v1 ? 0 : 1);
    if (qhat > 0) {
    /* qhat is one, so no need to mult */
        rhat = u3 - v1;
        /* t = r.b + u2 */
        t = TOHIGH(rhat) | u2;
        if (v0 > t)
            qhat--;
    }

    uu[1] = 0;      /* (u4) */
    uu[0] = u[1];   /* (u3u2) */
    if (qhat > 0) {
        /* (u4u3u2) -= qhat(v1v0) where u4 = 0 */
        _spMultSub(uu, qhat, v1, v0);
        if (HIHALF(uu[1]) != 0) {
        /* Add back */
            qhat--;
            uu[0] += v;
            uu[1] = 0;
        }
    }
    q2 = qhat;

    /*  ROUND 2. Set j = 1 and calculate q1 */
    /*  Estimate qhat = (u3u2) / v1
        then set (u3u2u1) -= qhat(v1v0)
    */
    t = uu[0];
    qhat = t / v1;
    rhat = t - qhat * v1;
    /* Test on v0 */
    t = TOHIGH(rhat) | u1;
    if ((qhat == (MAX_HALF_DIGIT + 1)) || (qhat * v0 > t)) {
        qhat--;
        rhat += v1;
        t = TOHIGH(rhat) | u1;
        if ((rhat < (MAX_HALF_DIGIT + 1)) && (qhat * v0 > t))
            qhat--;
    }

    /*  Multiply and subtract
        (u3u2u1)' = (u3u2u1) - qhat(v1v0)
    */
    uu[1] = HIHALF(uu[0]);  /* (0u3) */
    uu[0] = TOHIGH(LOHALF(uu[0])) | u1; /* (u2u1) */
    _spMultSub(uu, qhat, v1, v0);
    if (HIHALF(uu[1]) != 0) {
    /* Add back */
        qhat--;
        uu[0] += v;
        uu[1] = 0;
    }

    /* q1 = qhat */
    *q = TOHIGH(qhat);

    /* ROUND 3. Set j = 0 and calculate q0 */
    /*  Estimate qhat = (u2u1) / v1
        then set (u2u1u0) -= qhat(v1v0)
    */
    t = uu[0];
    qhat = t / v1;
    rhat = t - qhat * v1;
    /* Test on v0 */
    t = TOHIGH(rhat) | u0;
    if ((qhat == (MAX_HALF_DIGIT + 1)) || (qhat * v0 > t)) {
        qhat--;
        rhat += v1;
        t = TOHIGH(rhat) | u0;
        if ((rhat < (MAX_HALF_DIGIT + 1)) && (qhat * v0 > t))
            qhat--;
    }

    /*  Multiply and subtract
        (u2u1u0)" = (u2u1u0)' - qhat(v1v0)
    */
    uu[1] = HIHALF(uu[0]);  /* (0u2) */
    uu[0] = TOHIGH(LOHALF(uu[0])) | u0; /* (u1u0) */
    _spMultSub(uu, qhat, v1, v0);
    if (HIHALF(uu[1]) != 0) {
    /* Add back */
        qhat--;
        uu[0] += v;
        uu[1] = 0;
    }

    /* q0 = qhat */
    *q |= LOHALF(qhat);

    /* Remainder is in (u1u0) i.e. uu[0] */
    *r = uu[0];
    return q2;
}

int _mpSizeof(const DIGIT_T a[], int ndigits)
{   /* Returns size of significant digits in a */

    while (ndigits--) {
        if (a[ndigits] != 0)
            return (++ndigits);
    }
    return 0;
}

void _mpSetEqual(DIGIT_T a[], const DIGIT_T b[], int ndigits)
{
    /* Sets a = b */
    int i;

    for (i = 0; i < ndigits; i++) {
        a[i] = b[i];
    }
}

void _mpSetZero(DIGIT_T a[], int ndigits)
{
    /* Sets a = 0 */

    /* Prevent optimiser ignoring this */
    volatile DIGIT_T optdummy;
    DIGIT_T *p = a;

    while (ndigits--)
        a[ndigits] = 0;

    optdummy = *p;
}

void _mpSetDigit(DIGIT_T a[], DIGIT_T d, int ndigits)
{
    /* Sets a = d where d is a single digit */
    int i;

    for (i = 1; i < ndigits; i++) {
        a[i] = 0;
    }
    a[0] = d;
}

DIGIT_T _mpShiftLeft(DIGIT_T a[], const DIGIT_T *b, int shift, int ndigits)
{
    /* Computes a = b << shift */
    /* [v2.1] Modified to cope with shift > BITS_PERDIGIT */
    int i, y, nw, bits;
    DIGIT_T mask, carry, nextcarry;

    /* Do we shift whole digits? */
    if (shift >= BITS_PER_DIGIT) {
        nw = shift / BITS_PER_DIGIT;
        i = ndigits;
        while (i--) {
            if (i >= nw)
                a[i] = b[i-nw];
            else
                a[i] = 0;
        }
        /* Call again to shift bits inside digits */
        bits = shift % BITS_PER_DIGIT;
        carry = b[ndigits-nw] << bits;
        if (bits)
            carry |= _mpShiftLeft(a, a, bits, ndigits);
        return carry;
    } else {
        bits = shift;
    }

    /* Construct mask = high bits set */
    mask = ~(~(DIGIT_T)0 >> bits);

    y = BITS_PER_DIGIT - bits;
    carry = 0;
    for (i = 0; i < ndigits; i++) {
        nextcarry = (b[i] & mask) >> y;
        a[i] = b[i] << bits | carry;
        carry = nextcarry;
    }

    return carry;
}

DIGIT_T _mpShiftRight(DIGIT_T a[], const DIGIT_T b[], int shift, int ndigits)
{
    /* Computes a = b >> shift */
    /* [v2.1] Modified to cope with shift > BITS_PERDIGIT */
    int i, y, nw, bits;
    DIGIT_T mask, carry, nextcarry;

    /* Do we shift whole digits? */
    if (shift >= BITS_PER_DIGIT) {
        nw = shift / BITS_PER_DIGIT;
        for (i = 0; i < ndigits; i++) {
            if ((i+nw) < ndigits)
                a[i] = b[i+nw];
            else
                a[i] = 0;
        }
        /* Call again to shift bits inside digits */
        bits = shift % BITS_PER_DIGIT;
        carry = b[nw-1] >> bits;
        if (bits)
            carry |= _mpShiftRight(a, a, bits, ndigits);
        return carry;
    } else {
        bits = shift;
    }

    /* Construct mask to set low bits */
    /* (thanks to Jesse Chisholm for suggesting this improved technique) */
    mask = ~(~(DIGIT_T)0 << bits);

    y = BITS_PER_DIGIT - bits;
    carry = 0;
    i = ndigits;
    while (i--) {
        nextcarry = (b[i] & mask) << y;
        a[i] = b[i] >> bits | carry;
        carry = nextcarry;
    }

    return carry;
}

int _mpBitLength(const DIGIT_T d[], int ndigits)
/* Returns no of significant bits in d */
{
    int n, i, bits;
    DIGIT_T mask;

    if (!d || ndigits == 0)
        return 0;

    n = _mpSizeof(d, ndigits);
    if (0 == n)
        return 0;

    for (i = 0, mask = HIBITMASK; mask > 0; mask >>= 1, i++) {
        if (d[n-1] & mask)
            break;
    }

    bits = n * BITS_PER_DIGIT - i;

    return bits;
}

DIGIT_T _mpShortDiv(DIGIT_T q[], const DIGIT_T u[], DIGIT_T v, int ndigits)
{
    /*  Calculates quotient q = u div v
        Returns remainder r = u mod v
        where q, u are multiprecision integers of ndigits each
        and r, v are single precision digits.

        Makes no assumptions about normalisation.

        Ref: Knuth Vol 2 Ch 4.3.1 Exercise 16 p625
    */
    int j;
    DIGIT_T t[2], r;
    int shift;
    DIGIT_T bitmask, overflow, *uu;

    if (ndigits == 0)
        return 0;
    if (v == 0)
        return 0;   /* Divide by zero error */

    /*  Normalise first */
    /*  Requires high bit of V
        to be set, so find most signif. bit then shift left,
        i.e. d = 2^shift, u' = u * d, v' = v * d.
    */
    bitmask = HIBITMASK;
    for (shift = 0; shift < BITS_PER_DIGIT; shift++) {
        if (v & bitmask)
            break;
        bitmask >>= 1;
    }

    v <<= shift;
    overflow = mpFunc->ShiftLeft(q, u, shift, ndigits);
    uu = q;

    /* Step S1 - modified for extra digit. */
    r = overflow;   /* New digit Un */
    j = ndigits;
    while (j--) {
        /* Step S2. */
        t[1] = r;
        t[0] = uu[j];
        overflow = mpFunc->SpDivide(&q[j], &r, t, v);
    }

    /* Unnormalise */
    r >>= shift;

    return r;
}

int _mpEqual(const DIGIT_T a[], const DIGIT_T b[], int ndigits)
{
    /*  Returns true if a == b, else false
    */

    if (ndigits == 0)
        return -1;

    while (ndigits--) {
        if (a[ndigits] != b[ndigits])
            return 0;   /* False */
    }
    return 1;    /* True */
}

int _mpCompare(const DIGIT_T a[], const DIGIT_T b[], int ndigits)
{
    /*  Returns sign of (a - b)
    */

    if (ndigits == 0)
        return 0;

    while (ndigits--) {
        if (a[ndigits] > b[ndigits])
            return 1;   /* GT */
        if (a[ndigits] < b[ndigits])
            return -1;  /* LT */
    }

    return 0;   /* EQ */
}

DIGIT_T _mpAdd(DIGIT_T w[], const DIGIT_T u[], const DIGIT_T v[], int ndigits)
{
    /*  Calculates w = u + v
        where w, u, v are multiprecision integers of ndigits each
        Returns carry if overflow. Carry = 0 or 1.

        Ref: Knuth Vol 2 Ch 4.3.1 p 266 Algorithm A.
    */

    DIGIT_T k;
    int j;

    //assert(w != v);

    /* Step A1. Initialise */
    k = 0;

    for (j = 0; j < ndigits; j++) {
        /*  Step A2. Add digits w_j = (u_j + v_j + k)
            Set k = 1 if carry (overflow) occurs
        */
        w[j] = u[j] + k;
        if (w[j] < k)
            k = 1;
        else
            k = 0;

        w[j] += v[j];
        if (w[j] < v[j])
            k++;

    }   /* Step A3. Loop on j */

    return k;   /* w_n = k */
}

int _mpMultiply(DIGIT_T w[], const DIGIT_T u[], const DIGIT_T v[], int ndigits)
{
    /*  Computes product w = u * v
        where u, v are multiprecision integers of ndigits each
        and w is a multiprecision integer of 2*ndigits

        Ref: Knuth Vol 2 Ch 4.3.1 p 268 Algorithm M.
    */

    DIGIT_T k, t[2];
    int i, j, m, n;

    //assert(w != u && w != v);

    m = n = ndigits;

    /* Step M1. Initialise */
    for (i = 0; i < 2 * m; i++)
        w[i] = 0;

    for (j = 0; j < n; j++) {
        /* Step M2. Zero multiplier? */
        if (v[j] == 0) {
            w[j + m] = 0;
        } else {
            /* Step M3. Initialise i */
            k = 0;
            for (i = 0; i < m; i++) {
                /* Step M4. Multiply and add */
                /* t = u_i * v_j + w_(i+j) + k */
                mpFunc->SpMultiply(t, u[i], v[j]);
                t[0] += k;
                if (t[0] < k)
                    t[1]++;
                t[0] += w[i+j];
                if (t[0] < w[i+j])
                    t[1]++;

                w[i+j] = t[0];
                k = t[1];
            }
            /* Step M5. Loop on i, set w_(j+m) = k */
            w[j+m] = k;
        }
    }   /* Step M6. Loop on j */

    return 0;
}

static DIGIT_T _mpMultSub(DIGIT_T wn, DIGIT_T w[], const DIGIT_T v[], DIGIT_T q, int n)
{   /*  Compute w = w - qv
        where w = (WnW[n-1]...W[0])
        return modified Wn.
    */
    DIGIT_T k, t[2];
    int i;

    if (q == 0) /* No change */
        return wn;

    k = 0;

    for (i = 0; i < n; i++) {
        mpFunc->SpMultiply(t, q, v[i]);
        w[i] -= k;
        if (w[i] > MAX_DIGIT - k)
            k = 1;
        else
            k = 0;
        w[i] -= t[0];
        if (w[i] > MAX_DIGIT - t[0])
            k++;
        k += t[1];
    }

    /* Cope with Wn not stored in array w[0..n-1] */
    wn -= k;

    return wn;
}

static int _QhatTooBig(DIGIT_T qhat, DIGIT_T rhat, DIGIT_T vn2, DIGIT_T ujn2)
{
    //  Returns true if Qhat is too big
    //  i.e. if (Qhat * Vn-2) > (b.Rhat + Uj+n-2)
    //
    DIGIT_T t[2];

    mpFunc->SpMultiply(t, qhat, vn2);
    if (t[1] < rhat)
        return 0;
    else if (t[1] > rhat)
        return 1;
    else if (t[0] > ujn2)
        return 1;

    return 0;
}

int _mpDivide(DIGIT_T q[], DIGIT_T r[], const DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits)
{
    /*  Computes quotient q = u / v and remainder r = u mod v
    where q, r, u are multiple precision digits
    all of udigits and the divisor v is vdigits.

    Ref: Knuth Vol 2 Ch 4.3.1 p 272 Algorithm D.

    Do without extra storage space, i.e. use r[] for
    normalised u[], unnormalise v[] at end, and cope with
    extra digit Uj+n added to u after normalisation.

    WARNING: this trashes q and r first, so cannot do
    u = u / v or v = u mod v.
    It also changes v temporarily so cannot make it const.
    */
    int shift;
    int n, m, j;
    DIGIT_T bitmask, overflow;
    DIGIT_T qhat, rhat, t[2];
    DIGIT_T *uu, *ww;
    int qhatOK, cmp;

    /* Clear q and r */
    _mpSetZero(q, udigits);
    _mpSetZero(r, udigits);

    /* Work out exact sizes of u and v */
    n = (int)_mpSizeof(v, vdigits);
    m = (int)_mpSizeof(u, udigits);
    m -= n;

    /* Catch special cases */
    if (n == 0)
        return -1;  /* Error: divide by zero */

    if (n == 1) {
        /* Use short division instead */
        r[0] = mpFunc->ShortDiv(q, u, v[0], udigits);
        return 0;
    }

    if (m < 0) {
        /* v > u, so just set q = 0 and r = u */
        _mpSetEqual(r, u, udigits);
        return 0;
    }

    if (m == 0) {
    /* u and v are the same length */
        cmp = _mpCompare(u, v, (int)n);
        if (cmp < 0) {
            /* v > u, as above */
            _mpSetEqual(r, u, udigits);
            return 0;
        } else if (cmp == 0) {
            /* v == u, so set q = 1 and r = 0 */
            _mpSetDigit(q, 1, udigits);
            return 0;
        }
    }

    /*  In Knuth notation, we have:
        Given
        u = (Um+n-1 ... U1U0)
        v = (Vn-1 ... V1V0)
        Compute
        q = u/v = (QmQm-1 ... Q0)
        r = u mod v = (Rn-1 ... R1R0)
    */

    /*  Step D1. Normalise */
    /*  Requires high bit of Vn-1
        to be set, so find most signif. bit then shift left,
        i.e. d = 2^shift, u' = u * d, v' = v * d.
    */
    bitmask = HIBITMASK;
    for (shift = 0; shift < BITS_PER_DIGIT; shift++) {
        if (v[n-1] & bitmask)
            break;
        bitmask >>= 1;
    }

    /* Normalise v in situ - NB only shift non-zero digits */
    overflow = mpFunc->ShiftLeft(v, v, shift, n);

    /* Copy normalised dividend u*d into r */
    overflow = mpFunc->ShiftLeft(r, u, shift, n + m);
    uu = r; /* Use ptr to keep notation constant */

    t[0] = overflow;    /* Extra digit Um+n */

    /* Step D2. Initialise j. Set j = m */
    for (j = m; j >= 0; j--) {
        /* Step D3. Set Qhat = [(b.Uj+n + Uj+n-1)/Vn-1]
           and Rhat = remainder */
        qhatOK = 0;
        t[1] = t[0];    /* This is Uj+n */
        t[0] = uu[j+n-1];
        overflow = mpFunc->SpDivide(&qhat, &rhat, t, v[n-1]);

        /* Test Qhat */
        if (overflow) {
            /* Qhat == b so set Qhat = b - 1 */
            qhat = MAX_DIGIT;
            rhat = uu[j+n-1];
            rhat += v[n-1];
            if (rhat < v[n-1])  /* Rhat >= b, so no re-test */
                qhatOK = 1;
        }
        /* [VERSION 2: Added extra test "qhat && "] */
        if (qhat && !qhatOK && _QhatTooBig(qhat, rhat, v[n-2], uu[j+n-2])) {
            /* If Qhat.Vn-2 > b.Rhat + Uj+n-2
               decrease Qhat by one, increase Rhat by Vn-1
            */
            qhat--;
            rhat += v[n-1];
            /* Repeat this test if Rhat < b */
            if (!(rhat < v[n-1])) {
                if (_QhatTooBig(qhat, rhat, v[n-2], uu[j+n-2]))
                    qhat--;
            }
        }


        /* Step D4. Multiply and subtract */
        ww = &uu[j];
        overflow = _mpMultSub(t[1], ww, v, qhat, (int)n);

        /* Step D5. Test remainder. Set Qj = Qhat */
        q[j] = qhat;
        if (overflow) {
            /* Step D6. Add back if D4 was negative */
            q[j]--;
            overflow = _mpAdd(ww, ww, v, (int)n);
        }

        t[0] = uu[j+n-1];   /* Uj+n on next round */

    }   /* Step D7. Loop on j */

    /* Clear high digits in uu */
    for (j = n; j < m+n; j++)
        uu[j] = 0;

    /* Step D8. Unnormalise. */

    mpFunc->ShiftRight(r, r, shift, n);
    mpFunc->ShiftRight(v, v, shift, n);

    return 0;
}

int _mpSquare(DIGIT_T w[], const DIGIT_T x[], int ndigits)
{
    /*  Computes square w = x * x
        where x is a multiprecision integer of ndigits
        and w is a multiprecision integer of 2*ndigits

        Ref: Menezes p596 Algorithm 14.16 with errata.
    */

    DIGIT_T k, p[2], u[2], cbit, carry, o1, o2;
    int i, j, t, i2, cpos;

    //assert(w != x);
    t = ndigits;

    /* 1. For i from 0 to (2t-1) do: w_i = 0 */
    i2 = t << 1;
    for (i = 0; i < i2; i++)
        w[i] = 0;
    carry = 0;
    cpos = i2-1;
    /* 2. For i from 0 to (t-1) do: */
    for (i = 0; i < t; i++) {
        /* 2.1 (uv) = w_2i + x_i * x_i, w_2i = v, c = u
           Careful, w_2i may be double-prec
        */
        i2 = i << 1; /* 2*i */

        o1 = x[i] ;
        o2 = x[i] ;

        mpFunc->SpMultiply(p, o1, o2);
        p[0] += w[i2];
        if (p[0] < w[i2])
            p[1]++;
        k = 0;  /* p[1] < b, so no overflow here */
        if (i2 == cpos && carry) {
            p[1] += carry;
            if (p[1] < carry)
                k++;
            carry = 0;
        }
        w[i2] = p[0];
        u[0] = p[1];
        u[1] = k;

        /* 2.2 for j from (i+1) to (t-1) do:
           (uv) = w_{i+j} + 2x_j * x_i + c,
           w_{i+j} = v, c = u,
           u is double-prec
           w_{i+j} is dbl if [i+j] == cpos
        */

        k = 0;
        for (j = i+1; j < t; j++) {
            /* p = x_j * x_i */
            mpFunc->SpMultiply(p, x[j], x[i]);
            //_spMultiply(p, x[j], x[i]);
            /* p = 2p <=> p <<= 1 */
            cbit = (p[0] & HIBITMASK) != 0;
            k =  (p[1] & HIBITMASK) != 0;
            p[0] <<= 1;
            p[1] <<= 1;
            p[1] |= cbit;
            /* p = p + c */
            p[0] += u[0];
            if (p[0] < u[0]) {
                p[1]++;
                if (p[1] == 0)
                    k++;
            }
            p[1] += u[1];
            if (p[1] < u[1])
                k++;
            /* p = p + w_{i+j} */
            p[0] += w[i+j];
            if (p[0] < w[i+j]) {
                p[1]++;
                if (p[1] == 0)
                    k++;
            }
            if ((i+j) == cpos && carry) {
                /* catch overflow from last round */
                p[1] += carry;
                if (p[1] < carry)
                    k++;
                carry = 0;
            }
            /* w_{i+j} = v, c = u */
            w[i+j] = p[0];
            u[0] = p[1];
            u[1] = k;
        }
        /* 2.3 w_{i+t} = u */
        w[i+t] = u[0];
        /* remember overflow in w_{i+t} */
        carry = u[1];
        cpos = i+t;
    }

    /* (NB original step 3 deleted in errata) */

    /* Return w */

    return 0;
}

/* Version 2: added new funcs with temps
to avoid having to alloc and free repeatedly
and added _mpSquare function for slight speed improvement
*/

static int _moduloTemp(DIGIT_T r[], const DIGIT_T u[], int udigits,
             DIGIT_T v[], int vdigits, DIGIT_T tqq[], DIGIT_T trr[])
{
    /*  Calculates r = u mod v
        where r, v are multiprecision integers of length vdigits
        and u is a multiprecision integer of length udigits.
        r may overlap v.

        Same as mpModulo without allocs & free.
        Requires temp mp's tqq and trr of length udigits each
    */

    mpFunc->Divide(tqq, trr, u, udigits, v, vdigits);

    /* Final r is only vdigits long */
    _mpSetEqual(r, trr, vdigits);

    return 0;
}

static int _modMultTemp(DIGIT_T a[], const DIGIT_T x[], const DIGIT_T y[],
              DIGIT_T m[], int ndigits,
              DIGIT_T temp[], DIGIT_T tqq[], DIGIT_T trr[])
{   /*  Computes a = (x * y) mod m */
    /*  Requires 3 x temp mp's of length 2 * ndigits each */

    /* Calc p[2n] = x * y */
    mpFunc->Multiply(temp, x, y, ndigits);

    /* Then modulo m */
    _moduloTemp(a, temp, ndigits * 2, m, ndigits, tqq, trr);

    return 0;
}

static int _modSquareTemp(DIGIT_T a[], const DIGIT_T x[],
              DIGIT_T m[], int ndigits,
              DIGIT_T temp[], DIGIT_T tqq[], DIGIT_T trr[])
{   /*  Computes a = (x * x) mod m */
    /*  Requires 3 x temp mp's of length 2 * ndigits each */

    /* Calc p[2n] = x^2 */
    mpFunc->Square(temp, x, ndigits);

    /* Then modulo m */
    _moduloTemp(a, temp, ndigits * 2, m, ndigits, tqq, trr);

    return 0;
}

static unsigned int t1[RSA_KEY_DIGI_LEN*2];
static unsigned int t2[RSA_KEY_DIGI_LEN*2];
static unsigned int t3[RSA_KEY_DIGI_LEN*2];
static unsigned int tm[RSA_KEY_DIGI_LEN];
static unsigned int y[RSA_KEY_DIGI_LEN];

int _mpModExp(DIGIT_T yout[], const DIGIT_T x[], const DIGIT_T e[], const DIGIT_T m[], int ndigits)
{
    /*  Computes y = x^e mod m */
    /*  Binary left-to-right method */
    DIGIT_T mask;
    unsigned int n, nn;
    //DIGIT_T *t1, *t2, *t3, *tm, *y;

    if (ndigits == 0)
        return -1;

    /* Create some temps */
    nn = ndigits * 2;

    _mpSetZero(t1, nn);
    _mpSetZero(t2, nn);
    _mpSetZero(t3, nn);
    _mpSetZero(tm, ndigits);
    _mpSetZero(y, ndigits);

    _mpSetEqual(tm, m, ndigits);

    /* Find second-most significant bit in e */
    n = _mpSizeof(e, ndigits);
    for (mask = HIBITMASK; mask > 0; mask >>= 1) {
        if (e[n-1] & mask)
            break;
    }
    mpNEXTBITMASK(mask, n);

    /* Set y = x */
    _mpSetEqual(y, x, ndigits);

    /* For bit j = k-2 downto 0 */

    while (n) {
        /* Square y = y * y mod n */
        _modSquareTemp(y, y, tm, ndigits, t1, t2, t3);
        if (e[n-1] & mask) {
            /*  if e(j) == 1 then multiply y = y * x mod n */
            _modMultTemp(y, y, x, tm, ndigits, t1, t2, t3);
        }

        /* Move to next bit */
        mpNEXTBITMASK(mask, n);
    }

    /* return y */
    _mpSetEqual(yout, y, ndigits);

    return 0;
}

int _mpConvFromOctets(DIGIT_T a[], int ndigits, const unsigned char *c, int nbytes)
/* Converts nbytes octets into big digit a of max size ndigits
   Returns actual number of digits set (may be larger than mpSizeof)
*/
{
    //mpFunc=&_mpFunc;  //-------------------------------
    int i;
    int j, k;
    DIGIT_T t;

    _mpSetZero(a, ndigits);

    /* Read in octets, least significant first */
    /* i counts into big_d, j along c, and k is # bits to shift */
    for (i = 0, j = nbytes - 1; i < ndigits && j >= 0; i++) {
        t = 0;
        for (k = 0; j >= 0 && k < BITS_PER_DIGIT; j--, k += 8) {
            t |= ((DIGIT_T)c[j]) << k;
        }
        a[i] = t;
    }

    return i;
}

int _mpConvToOctets(const DIGIT_T a[], int ndigits, unsigned char *c, int nbytes)
/* Convert big digit a into string of octets, in big-endian order,
   padding on the left to nbytes or truncating if necessary.
   Return number of octets required excluding leading zero bytes.
*/
{
    int j, k, len;
    DIGIT_T t;
    int i, noctets, nbits;

    nbits = _mpBitLength(a, ndigits);
    noctets = (nbits + 7) / 8;

    len = (int)nbytes;

    for (i = 0, j = len - 1; i < ndigits && j >= 0; i++) {
        t = a[i];
        for (k = 0; j >= 0 && k < BITS_PER_DIGIT; j--, k += 8) {
            c[j] = (unsigned char)(t >> k);
        }
    }

    for ( ; j >= 0; j--) {
        c[j] = 0;
    }

    return (int)noctets;
}

static void rsa_init( rsa_context *ctx,
               int padding,
               int hash_id,
               int (*f_rng)(void *),
               void *p_rng )
{
    memset( ctx, 0, sizeof( rsa_context ) );

    ctx->padding = padding;
    ctx->hash_id = hash_id;

    ctx->f_rng = f_rng;
    ctx->p_rng = p_rng;
}

/*
 * Do an RSA public key operation
 */
static int rsa_public( rsa_context *ctx,
                unsigned char *input,
                unsigned char *output )
{
    int ret, olen;
    mpi T;

    mpi_init( &T);

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if ( mpi_cmp_mpi(&T, ( mpi const *)&ctx->N ) >= 0 ) {
        mpi_free( &T);
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    olen = ctx->len;
    MPI_CHK( mpi_exp_mod( &T, ( mpi const *)&T, ( mpi const *)&ctx->E, ( mpi const *)&ctx->N, &ctx->RN ) );
    MPI_CHK( mpi_write_binary( ( mpi const *)&T, output, olen ) );

cleanup:

    mpi_free( &T);

    if ( 0 != ret )
        return( POLARSSL_ERR_RSA_PUBLIC_FAILED | ret );

    return( 0 );
}

/*
 * Do an RSA private key operation
 */
static int rsa_private( rsa_context *ctx,
                 unsigned char *input,
                 unsigned char *output )
{
    int ret, olen;
    mpi T, T1, T2;
    mpi_init( &T);
    mpi_init( &T1);
    mpi_init( &T2);
    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if ( mpi_cmp_mpi(( mpi const *)&T, ( mpi const *)&ctx->N ) >= 0 ) {
        mpi_free( &T);
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

#if 0
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->D, &ctx->N, &ctx->RN ) );
#else
    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( mpi_exp_mod( &T1,( mpi const *)&T,( mpi const *)&ctx->DP,( mpi const *)&ctx->P, &ctx->RP ) );
    MPI_CHK( mpi_exp_mod( &T2,( mpi const *)&T,( mpi const *)&ctx->DQ,( mpi const *)&ctx->Q, &ctx->RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( mpi_sub_mpi( &T, ( mpi const *)&T1, &T2 ) );
    MPI_CHK( mpi_mul_mpi( &T1,( mpi const *)&T,( mpi const *)&ctx->QP ) );
    MPI_CHK( mpi_mod_mpi( &T,( mpi const *)&T1,( mpi const *)&ctx->P ) );

    /*
     * output = T2 + T * Q
     */
    MPI_CHK( mpi_mul_mpi( &T1,( mpi const *)&T,( mpi const *)&ctx->Q ) );
    MPI_CHK( mpi_add_mpi( &T,( mpi const *)&T2,( mpi const *)&T1 ) );
#endif

    olen = ctx->len;
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

    if ( mpi_cmp_mpi(( mpi const *)&T, ( mpi const *)&ctx->N ) >= 0 ) {
        mpi_free( &T);
        return( POLARSSL_ERR_RSA_OUTPUT_TO_LARGE );
    }

cleanup:

    mpi_free( &T );
    mpi_free( &T1);
    mpi_free( &T2);
    if ( 0 != ret )
        return( POLARSSL_ERR_RSA_PRIVATE_FAILED | ret );

    return( 0 );
}

static int rsa_v15_sign( rsa_context *ctx,
                  int mode,
                  int hash_id,
                  unsigned int hashlen,
                  const unsigned char *hash,
                  unsigned char *sig )
{
    int nb_pad, olen;
    unsigned char *p = sig;
    if ( ctx->padding != RSA_PKCS_V15 ) {
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }
    olen = ctx->len;
    switch ( hash_id ) {
        case RSA_RAW:
            nb_pad = olen - 3 - hashlen;
            break;
        case RSA_MD2:
        case RSA_MD4:
        case RSA_MD5:
            nb_pad = olen - 3 - 34;
            break;
        case RSA_SHA1:
            nb_pad = olen - 3 - 35;
            break;
        case RSA_SHA256:
            nb_pad = olen - 3 - 51;
            break;
        default:
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
        }
    if ( ( nb_pad < 8 ) || ( nb_pad > olen ) ) {
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }
    *p++ = 0;
    *p++ = RSA_SIGN;
    memset( p, 0xFF, nb_pad );
    p += nb_pad;
    *p++ = 0;
    switch( hash_id ) {
        case RSA_RAW:
            memcpy( p, hash, hashlen );
            break;
        case RSA_MD2:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 2;
            break;
        case RSA_MD4:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 4;
            break;
        case RSA_MD5:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 5;
            break;
        case RSA_SHA1:
            memcpy( p, ASN1_HASH_SHA1, 15 );
            memcpy( p + 15, hash, 20 );
            break;
        case RSA_SHA256:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 32 );
            p[1] += 32; p[14] = 1; p[18] += 32;
            break;
        default:
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
        }
    return ( ( mode == RSA_PUBLIC )
        ? rsa_public(  ctx, sig, sig )
        : rsa_private( ctx, sig, sig ) );
}

static void get_prkey_from_boot2(rsa_context *rsa, char *N, char *D)
{
    char *boot2 = "/dev/block/mmcblk0boot1";
    FILE *fp;
    int i = 0;
    char buffer[1024];
    rawdata_header head[HEAD_NUM];
    if (NULL == (fp = fopen(boot2, "r"))) {
        printf("open failed!\n");
    }
    memset(&head, 0, HEAD_NUM * sizeof(rawdata_header));
    if (-1 == fread(&head, 1, HEAD_NUM * sizeof(rawdata_header), fp)) {
        printf("read error!\n");
    }
    for (i = 0; i < HEAD_NUM; i++) {
        fseek(fp, 0, SEEK_SET);
        memset(buffer, 0, 1024);
        head[i].name[strlen(head[i].name) - 1] = '\0';
        if (0 == strcmp(head[i].name, "N")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(N, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)N, head[i].size, NULL);
            N[head[i].size] = '\0';
            mpi_read_string( &rsa->N , 16, N );
        } else if (0 == strcmp(head[i].name, "E")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[8] = '\0';
            mpi_read_string( &rsa->E , 16, buffer );
        } else if (0 == strcmp(head[i].name, "D")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(D, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)D, head[i].size, NULL);
            D[head[i].size] = '\0';
            mpi_read_string( &rsa->D , 16, D );
        } else if (0 == strcmp(head[i].name, "P")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[head[i].size] = '\0';
            mpi_read_string( &rsa->P , 16, buffer );
        } else if (0 == strcmp(head[i].name, "Q")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[head[i].size] = '\0';
            mpi_read_string( &rsa->Q , 16, buffer);
        } else if (0 == strcmp(head[i].name, "DP")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[head[i].size] = '\0';
            mpi_read_string( &rsa->DP, 16, buffer );
        } else if (0 == strcmp(head[i].name, "DQ")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[head[i].size] = '\0';
            mpi_read_string( &rsa->DQ, 16, buffer );
        } else if (0 == strcmp(head[i].name, "QP")) {
            fseek(fp, head[i].offset, SEEK_SET);
            fread(buffer, 1, head[i].size, fp);
            Secure_AES_ECB_Decrypt((int)buffer, head[i].size, NULL);
            buffer[head[i].size] = '\0';
            mpi_read_string( &rsa->QP, 16, buffer );
        }
    }
}

// Convert binary file into HEX bytes for being included in a .c file.
static int str2hex(char* pInput, unsigned char* pOutput, int* len)
{
    char c1, c2;
    int i, length;
    length = strlen(pInput);
    if (length%2) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        pInput[i] = toupper(pInput[i]);
    }
    for (i = 0; i< strlen(pInput)/2; i++) {
        c1 = pInput[2*i];
        c2 = pInput[2*i+1];
        if (c1<'0' || (c1 > '9' && c1 <'A') || c1 > 'F') {
            continue;
        }
        if (c2<'0' || (c2 > '9' && c2 <'A') || c2 > 'F') {
            continue;
        }
        c1 = c1>'9' ? c1-'A'+10 : c1 -'0';
        c2 = c2>'9' ? c2-'A'+10 : c2 -'0';
        pOutput[i] = c1<<4 | c2;
    }
    *len = i;
    return 1;
}

static int RSA2048SWdecrypt(unsigned char *Signature, unsigned char *PublicKey_N, unsigned char *PublicKey_E, unsigned char *Sim_SignOut)
{
    DIGIT_T Sim_Signature[RSA_KEY_DIGI_LEN];
    DIGIT_T Sim_N[RSA_KEY_DIGI_LEN];
    DIGIT_T Sim_E[RSA_KEY_DIGI_LEN];

    _mpSetZero(Sim_Signature, RSA_KEY_DIGI_LEN);
    _mpSetZero(Sim_N, RSA_KEY_DIGI_LEN);
    _mpSetZero(Sim_E, RSA_KEY_DIGI_LEN);
    _mpSetZero((DIGIT_T *)Sim_SignOut, RSA_KEY_DIGI_LEN);

    mpFunc->ConvFromOctets(Sim_Signature, RSA_KEY_DIGI_LEN, Signature, RSA_KEY_DIGI_LEN); //sizeof(Signature)
    mpFunc->ConvFromOctets(Sim_N, RSA_KEY_DIGI_LEN, PublicKey_N, RSA_KEY_DIGI_LEN); //sizeof(PublicKey_N)
    mpFunc->ConvFromOctets(Sim_E, RSA_KEY_DIGI_LEN, PublicKey_E, 4); //sizeof(PublicKey_E)
    mpFunc->ModExp((DIGIT_T *)Sim_SignOut, Sim_Signature, Sim_E, Sim_N, RSA_KEY_DIGI_LEN);
    return 0;
}
#endif

int RSA2048_decrypt(unsigned char *Signature, unsigned char *PublicKey_N, unsigned char *PublicKey_E, unsigned char *Sim_SignOut)
{
    DEBUG("IN\n");
    int ret = -1;
    #if defined(BUILD_WITH_SECURE_AESDMA)
    //AESDMA
    ret = RSA2048HWdecrypt(Signature,PublicKey_N,PublicKey_E,Sim_SignOut);
    #else
    //SW
    ret = RSA2048SWdecrypt(Signature,PublicKey_N,PublicKey_E,Sim_SignOut);
    #endif
    if (-1 == ret) {
        DEBUG("RSA Decrypt Error\n");
    } else {
        DEBUG("OK\n");
    }
    return ret;
}


int RSA2048_encrypt(char *digest, unsigned char *rsa_sign)
{
    rsa_context rsa;
    char N[513], D[513];
    unsigned char N_hex[256], D_hex[256];
    int ret = -1, len = 512;

    DEBUG("IN\n");

    // get RSA key form boot2
    rsa_init( &rsa, RSA_PKCS_V15, 0, NULL, NULL );
    rsa.len = RSA_PUBLIC_KEY_N_LEN;
    memset(N, 0, 513);
    memset(D, 0, 513);
    memset(N_hex, 0, 256);
    memset(D_hex, 0, 256);

    get_prkey_from_boot2(&rsa, N, D);
    str2hex(N, N_hex, &len);
    str2hex(D, D_hex, &len);

    #if defined(BUILD_WITH_AESDMA)
    ret = RSA2048HWencrypt((unsigned char*)digest, N_hex, D_hex, rsa_sign);
    #else
    ret= rsa_v15_sign( &rsa, RSA_PRIVATE, RSA_SHA256, 32, (const unsigned char*)digest, rsa_sign);
    #endif
    if (0 != ret) {
        DEBUG("RSA Encrypt error!\n");
    } else {
        DEBUG("OK\n");
    }
    return ret;
}

void RSA_test(void)
{
    #if defined(BUILD_WITH_SECURE_AESDMA)
    // RSA key ,N+E+D
    MS_U8 u8MstarDefRSAImageKey[RSA_PUBLIC_KEY_LEN+RSA_PUBLIC_KEY_N_LEN]={
    0x84 ,0x62 ,0x76 ,0xCC ,0xBD ,0x5A ,0x5A ,0x40 ,0x30 ,0xC0 ,0x96 ,0x40 ,0x87 ,0x28 ,0xDB ,0x85 ,
    0xED ,0xED ,0x9F ,0x3E ,0xDE ,0x4E ,0x65 ,0xE6 ,0x7B ,0x1B ,0x78 ,0x17 ,0x87 ,0x9D ,0xF6 ,0x16 ,
    0xC3 ,0xD3 ,0x27 ,0xBC ,0xB4 ,0x5A ,0x3  ,0x13 ,0x35 ,0xB0 ,0x96 ,0x5A ,0x96 ,0x41 ,0x74 ,0x4E ,
    0xB9 ,0xD1 ,0x77 ,0x96 ,0xF7 ,0x8D ,0xE2 ,0xE7 ,0x15 ,0x9  ,0x65 ,0x9C ,0x46 ,0x79 ,0xEA ,0xF0 ,
    0x91 ,0x67 ,0x35 ,0xFA ,0x69 ,0x4C ,0x83 ,0xF7 ,0xDC ,0xCF ,0x97 ,0x20 ,0xF2 ,0xA5 ,0xBA ,0x72 ,
    0x80 ,0x9D ,0x55 ,0x79 ,0x17 ,0xDC ,0x6E ,0x60 ,0xA5 ,0xE7 ,0xE  ,0x9E ,0x89 ,0x9B ,0x46 ,0x6  ,
    0x52 ,0xFC ,0x64 ,0x56 ,0x2  ,0x8  ,0x9A ,0x96 ,0x41 ,0xE2 ,0x4F ,0xDB ,0xB6 ,0x60 ,0xC3 ,0x38 ,
    0xDF ,0xF4 ,0x97 ,0x81 ,0x5D ,0x12 ,0x2  ,0xAE ,0x2B ,0x9F ,0x9  ,0x29 ,0xB9 ,0x9D ,0x51 ,0x45 ,
    0xD2 ,0x9E ,0x2B ,0xAF ,0x64 ,0xCA ,0x9A ,0x6  ,0x4E ,0x94 ,0x35 ,0x67 ,0xF7 ,0x8E ,0x4  ,0x7B ,
    0x24 ,0x38 ,0xA0 ,0xDF ,0xE7 ,0x5F ,0x1E ,0x6D ,0x29 ,0x8E ,0x30 ,0xD7 ,0x83 ,0x8C ,0xB4 ,0x41 ,
    0xD2 ,0xFD ,0xBF ,0x5B ,0x18 ,0xCA ,0x50 ,0xD1 ,0x27 ,0xD1 ,0xF6 ,0x7D ,0x54 ,0x3E ,0x80 ,0x5F ,
    0x20 ,0xDC ,0x88 ,0x82 ,0xCF ,0xBE ,0xE1 ,0x46 ,0x2A ,0xD6 ,0x63 ,0xB9 ,0xB9 ,0x9D ,0xA3 ,0xC7 ,
    0x68 ,0x3E ,0x48 ,0xCE ,0x6A ,0x62 ,0x6F ,0xD1 ,0x6A ,0xC3 ,0xB6 ,0xDE ,0xF3 ,0x39 ,0x25 ,0xEC ,
    0xF6 ,0x79 ,0x20 ,0xB5 ,0xF2 ,0x30 ,0x25 ,0x6E ,0x99 ,0xAE ,0x39 ,0x56 ,0xDA ,0xAF ,0x83 ,0xD6 ,
    0xB8 ,0x49 ,0x15 ,0x78 ,0x81 ,0xCC ,0x3C ,0x4F ,0x66 ,0x5D ,0x95 ,0x7E ,0x31 ,0xD4 ,0x37 ,0x2A ,
    0xBE ,0xFC ,0xB4 ,0x66 ,0xF8 ,0x91 ,0x1  ,0xA  ,0x53 ,0x3C ,0x3C ,0xAB ,0x86 ,0xB9 ,0x80 ,0xB7 ,
    0x0  ,0x1  ,0x0  ,0x1,
    0x73 ,0xA1 ,0x79 ,0x78 ,0xCD ,0x6F ,0x8C ,0xE3 ,0x02 ,0x72 ,0x45 ,0x0A ,0xE9 ,0xC3 ,0x83 ,0x33 ,
    0x11 ,0x25 ,0xFB ,0x71 ,0x33 ,0x45 ,0xB0 ,0xF5 ,0xC6 ,0xD3 ,0xB0 ,0x6A ,0x84 ,0xF7 ,0x31 ,0x0A ,
    0xA3 ,0x52 ,0xDD ,0x23 ,0x93 ,0x38 ,0x07 ,0xF5 ,0x90 ,0x70 ,0xC4 ,0x73 ,0x2D ,0x48 ,0xD0 ,0xA9 ,
    0x2E ,0xDE ,0xCA ,0x21 ,0x1F ,0xEB ,0x5B ,0xA4 ,0x51 ,0x98 ,0x9A ,0x9B ,0x0C ,0x67 ,0xD3 ,0x10 ,
    0xFB ,0x3F ,0xF6 ,0x42 ,0xDA ,0x14 ,0x8E ,0x34 ,0x42 ,0x37 ,0x44 ,0x1E ,0xF0 ,0x32 ,0x57 ,0x8F ,
    0x49 ,0xCD ,0xA2 ,0xE9 ,0x99 ,0x30 ,0xDD ,0x6C ,0x27 ,0x9C ,0x91 ,0x05 ,0x62 ,0xD6 ,0x30 ,0x2C ,
    0xB7 ,0xF1 ,0x62 ,0x46 ,0x49 ,0x92 ,0x70 ,0x4C ,0x0C ,0x11 ,0xFB ,0x84 ,0xC2 ,0x60 ,0xF4 ,0x43 ,
    0xCA ,0x41 ,0xDE ,0xCF ,0x8C ,0x96 ,0x7D ,0xA6 ,0xD9 ,0xCD ,0x18 ,0x78 ,0xCC ,0x7A ,0x9F ,0x90 ,
    0x24 ,0x74 ,0xE2 ,0xA9 ,0x38 ,0x93 ,0x3D ,0x2B ,0x51 ,0xC8 ,0xEF ,0x06 ,0xD4 ,0x60 ,0x19 ,0xEA ,
    0x21 ,0x33 ,0x79 ,0xF6 ,0x48 ,0x93 ,0x8A ,0x95 ,0x3C ,0x12 ,0x8F ,0x45 ,0x8D ,0x42 ,0x1E ,0x0E ,
    0x86 ,0x21 ,0x44 ,0xBE ,0x86 ,0xFA ,0x87 ,0x7F ,0x68 ,0x40 ,0xF6 ,0xCF ,0xF3 ,0x6E ,0xAC ,0x3E ,
    0x64 ,0xAB ,0x68 ,0x2D ,0xAA ,0xE8 ,0xAE ,0x19 ,0x46 ,0x69 ,0xE2 ,0xB3 ,0xE2 ,0x00 ,0xE2 ,0xC6 ,
    0xE7 ,0x75 ,0x14 ,0x3C ,0xB1 ,0x3A ,0x45 ,0xD1 ,0x18 ,0x45 ,0x88 ,0x3F ,0x4B ,0xD0 ,0x6C ,0x64 ,
    0x20 ,0xBA ,0xBD ,0x38 ,0x42 ,0x69 ,0xAA ,0x08 ,0x7A ,0x3A ,0x98 ,0xDD ,0xF9 ,0x45 ,0x74 ,0xF5 ,
    0xD7 ,0x8E ,0x2B ,0xAD ,0x67 ,0x1D ,0x42 ,0x57 ,0x2E ,0x48 ,0x85 ,0x22 ,0xF0 ,0xC9 ,0x1F ,0x77 ,
    0x42 ,0xC2 ,0x28 ,0x5C ,0x8E ,0xE2 ,0x0C ,0x3D ,0xE3 ,0xB0 ,0x60 ,0xD1 ,0xAA ,0x99 ,0x87 ,0xE1};

    MS_U8 u8Signature[] = {
        0x53 ,0x45 ,0x43 ,0x55 ,0x52 ,0x49 ,0x54 ,0x59 ,0x00 ,0x00 ,0x00 ,0x00 ,0x40 ,0x03 ,0x00 ,0x00,
        0x5f ,0x09 ,0x4c ,0x87 ,0x86 ,0xdc ,0x0a ,0xa9 ,0x59 ,0x44 ,0x32 ,0x7a ,0xc8 ,0xa8 ,0x98 ,0x05};
    MS_U8 u8Signaturebackup[]= {
        0x53 ,0x45 ,0x43 ,0x55 ,0x52 ,0x49 ,0x54 ,0x59 ,0x00 ,0x00 ,0x00 ,0x00 ,0x40 ,0x03 ,0x00 ,0x00,
        0x5f ,0x09 ,0x4c ,0x87 ,0x86 ,0xdc ,0x0a ,0xa9 ,0x59 ,0x44 ,0x32 ,0x7a ,0xc8 ,0xa8 ,0x98 ,0x05};
    MS_U8 u8SignatureOut[0x100] = "\0";
    int ret = -1;
    // RSA encrypt
    ret = RSA2048HWencrypt(u8Signature,u8MstarDefRSAImageKey,u8MstarDefRSAImageKey+RSA_PUBLIC_KEY_LEN,u8SignatureOut);
    if (0 != ret) {
        DEBUG("RSA HW encrypt error\n");
        return;
    }
    DEBUG("data after RSA HW encrypt:\n");
    mem_dump((MS_U32)u8SignatureOut,0x100);
    // RSA decrypt
    ret = RSA2048HWdecrypt(u8SignatureOut,u8MstarDefRSAImageKey,u8MstarDefRSAImageKey+RSA_PUBLIC_KEY_N_LEN,u8SignatureOut);
    if (0 != ret) {
        DEBUG("RSA HW decrypt error\n");
        return;
    }
    DEBUG("data after RSA HW decrypt:\n");
    mem_dump((MS_U32)u8SignatureOut,0x100);
    // check data
    if (0 != memcmp(u8SignatureOut,u8Signaturebackup,0x20)) {
        DEBUG("data not equal\n");
        DEBUG("data bakcup is:\n");
        mem_dump((MS_U32)u8Signaturebackup,0x20);
    } else {
        DEBUG("data equal\n");
    }
    #endif
}