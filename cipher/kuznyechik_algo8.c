/**
 * @file kuznyechik.c
 * @brief Kuznyechik 128bit cipher as specified in GOST R 34-12.2015
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2015  Markku-Juhani O. Saarinen <mjos@iki.fi>
 * Copyright (c) 2017  Vlasta Vesely <vlastavesely@protonmail.ch>
 * Copyright (C) 2010-2024 Oryx Embedded SARL. All rights reserved.
 * Copyright (c) 2024 Valery Novikov <novikov.val@gmail.com>
 *
 * This file is part of CycloneCRYPTO Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @section Description
 *
 * This implementation is Saarinen's code kuznechik_8bit.c 
 * with small optimize. The original code can be found on Github:
 * https://github.com/mjosaarinen/kuznechik/blob/master/kuznechik_8bit.c
 *
  * @author Valery Novikov <novikov.val@gmail.com>
 * @version 2.4.0
 **/

/**
 * TODO: 
 *       OIDs
 *       pem_common
 *       pkcs4_common
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL CRYPTO_TRACE_LEVEL

//Dependencies
#include "core/crypto.h"
#include "cipher/kuznyechik.h"

//Check crypto library configuration
#if (KUZNYECHIK_SUPPORT == ENABLED)

// The S-Box from section 5.1.1

static const uint8_t kuz_pi[0x100] = {
	0xFC, 0xEE, 0xDD, 0x11, 0xCF, 0x6E, 0x31, 0x16, 	// 00..07
	0xFB, 0xC4, 0xFA, 0xDA, 0x23, 0xC5, 0x04, 0x4D, 	// 08..0F
	0xE9, 0x77, 0xF0, 0xDB, 0x93, 0x2E, 0x99, 0xBA, 	// 10..17
	0x17, 0x36, 0xF1, 0xBB, 0x14, 0xCD, 0x5F, 0xC1, 	// 18..1F
	0xF9, 0x18, 0x65, 0x5A, 0xE2, 0x5C, 0xEF, 0x21, 	// 20..27
	0x81, 0x1C, 0x3C, 0x42, 0x8B, 0x01, 0x8E, 0x4F, 	// 28..2F
	0x05, 0x84, 0x02, 0xAE, 0xE3, 0x6A, 0x8F, 0xA0, 	// 30..37
	0x06, 0x0B, 0xED, 0x98, 0x7F, 0xD4, 0xD3, 0x1F, 	// 38..3F
	0xEB, 0x34, 0x2C, 0x51, 0xEA, 0xC8, 0x48, 0xAB, 	// 40..47
	0xF2, 0x2A, 0x68, 0xA2, 0xFD, 0x3A, 0xCE, 0xCC, 	// 48..4F
	0xB5, 0x70, 0x0E, 0x56, 0x08, 0x0C, 0x76, 0x12, 	// 50..57
	0xBF, 0x72, 0x13, 0x47, 0x9C, 0xB7, 0x5D, 0x87, 	// 58..5F
	0x15, 0xA1, 0x96, 0x29, 0x10, 0x7B, 0x9A, 0xC7, 	// 60..67
	0xF3, 0x91, 0x78, 0x6F, 0x9D, 0x9E, 0xB2, 0xB1, 	// 68..6F
	0x32, 0x75, 0x19, 0x3D, 0xFF, 0x35, 0x8A, 0x7E, 	// 70..77
	0x6D, 0x54, 0xC6, 0x80, 0xC3, 0xBD, 0x0D, 0x57, 	// 78..7F
	0xDF, 0xF5, 0x24, 0xA9, 0x3E, 0xA8, 0x43, 0xC9, 	// 80..87
	0xD7, 0x79, 0xD6, 0xF6, 0x7C, 0x22, 0xB9, 0x03, 	// 88..8F
	0xE0, 0x0F, 0xEC, 0xDE, 0x7A, 0x94, 0xB0, 0xBC, 	// 90..97
	0xDC, 0xE8, 0x28, 0x50, 0x4E, 0x33, 0x0A, 0x4A, 	// 98..9F
	0xA7, 0x97, 0x60, 0x73, 0x1E, 0x00, 0x62, 0x44, 	// A0..A7
	0x1A, 0xB8, 0x38, 0x82, 0x64, 0x9F, 0x26, 0x41, 	// A8..AF
	0xAD, 0x45, 0x46, 0x92, 0x27, 0x5E, 0x55, 0x2F, 	// B0..B7
	0x8C, 0xA3, 0xA5, 0x7D, 0x69, 0xD5, 0x95, 0x3B, 	// B8..BF
	0x07, 0x58, 0xB3, 0x40, 0x86, 0xAC, 0x1D, 0xF7, 	// C0..C7
	0x30, 0x37, 0x6B, 0xE4, 0x88, 0xD9, 0xE7, 0x89, 	// C8..CF
	0xE1, 0x1B, 0x83, 0x49, 0x4C, 0x3F, 0xF8, 0xFE, 	// D0..D7
	0x8D, 0x53, 0xAA, 0x90, 0xCA, 0xD8, 0x85, 0x61, 	// D8..DF
	0x20, 0x71, 0x67, 0xA4, 0x2D, 0x2B, 0x09, 0x5B, 	// E0..E7
	0xCB, 0x9B, 0x25, 0xD0, 0xBE, 0xE5, 0x6C, 0x52, 	// E8..EF
	0x59, 0xA6, 0x74, 0xD2, 0xE6, 0xF4, 0xB4, 0xC0, 	// F0..F7
	0xD1, 0x66, 0xAF, 0xC2, 0x39, 0x4B, 0x63, 0xB6, 	// F8..FF
};

// Inverse S-Box

static const uint8_t kuz_pi_inv[0x100] = {
	0xA5, 0x2D, 0x32, 0x8F, 0x0E, 0x30, 0x38, 0xC0, 	// 00..07
	0x54, 0xE6, 0x9E, 0x39, 0x55, 0x7E, 0x52, 0x91, 	// 08..0F
	0x64, 0x03, 0x57, 0x5A, 0x1C, 0x60, 0x07, 0x18, 	// 10..17
	0x21, 0x72, 0xA8, 0xD1, 0x29, 0xC6, 0xA4, 0x3F, 	// 18..1F
	0xE0, 0x27, 0x8D, 0x0C, 0x82, 0xEA, 0xAE, 0xB4, 	// 20..27
	0x9A, 0x63, 0x49, 0xE5, 0x42, 0xE4, 0x15, 0xB7, 	// 28..2F
	0xC8, 0x06, 0x70, 0x9D, 0x41, 0x75, 0x19, 0xC9, 	// 30..37
	0xAA, 0xFC, 0x4D, 0xBF, 0x2A, 0x73, 0x84, 0xD5, 	// 38..3F
	0xC3, 0xAF, 0x2B, 0x86, 0xA7, 0xB1, 0xB2, 0x5B, 	// 40..47
	0x46, 0xD3, 0x9F, 0xFD, 0xD4, 0x0F, 0x9C, 0x2F, 	// 48..4F
	0x9B, 0x43, 0xEF, 0xD9, 0x79, 0xB6, 0x53, 0x7F, 	// 50..57
	0xC1, 0xF0, 0x23, 0xE7, 0x25, 0x5E, 0xB5, 0x1E, 	// 58..5F
	0xA2, 0xDF, 0xA6, 0xFE, 0xAC, 0x22, 0xF9, 0xE2, 	// 60..67
	0x4A, 0xBC, 0x35, 0xCA, 0xEE, 0x78, 0x05, 0x6B, 	// 68..6F
	0x51, 0xE1, 0x59, 0xA3, 0xF2, 0x71, 0x56, 0x11, 	// 70..77
	0x6A, 0x89, 0x94, 0x65, 0x8C, 0xBB, 0x77, 0x3C, 	// 78..7F
	0x7B, 0x28, 0xAB, 0xD2, 0x31, 0xDE, 0xC4, 0x5F, 	// 80..87
	0xCC, 0xCF, 0x76, 0x2C, 0xB8, 0xD8, 0x2E, 0x36, 	// 88..8F
	0xDB, 0x69, 0xB3, 0x14, 0x95, 0xBE, 0x62, 0xA1, 	// 90..97
	0x3B, 0x16, 0x66, 0xE9, 0x5C, 0x6C, 0x6D, 0xAD, 	// 98..9F
	0x37, 0x61, 0x4B, 0xB9, 0xE3, 0xBA, 0xF1, 0xA0, 	// A0..A7
	0x85, 0x83, 0xDA, 0x47, 0xC5, 0xB0, 0x33, 0xFA, 	// A8..AF
	0x96, 0x6F, 0x6E, 0xC2, 0xF6, 0x50, 0xFF, 0x5D, 	// B0..B7
	0xA9, 0x8E, 0x17, 0x1B, 0x97, 0x7D, 0xEC, 0x58, 	// B8..BF
	0xF7, 0x1F, 0xFB, 0x7C, 0x09, 0x0D, 0x7A, 0x67, 	// C0..C7
	0x45, 0x87, 0xDC, 0xE8, 0x4F, 0x1D, 0x4E, 0x04, 	// C8..CF
	0xEB, 0xF8, 0xF3, 0x3E, 0x3D, 0xBD, 0x8A, 0x88, 	// D0..D7
	0xDD, 0xCD, 0x0B, 0x13, 0x98, 0x02, 0x93, 0x80, 	// D8..DF
	0x90, 0xD0, 0x24, 0x34, 0xCB, 0xED, 0xF4, 0xCE, 	// E0..E7
	0x99, 0x10, 0x44, 0x40, 0x92, 0x3A, 0x01, 0x26, 	// E8..EF
	0x12, 0x1A, 0x48, 0x68, 0xF5, 0x81, 0x8B, 0xC7, 	// F0..F7
	0xD6, 0x20, 0x0A, 0x08, 0x00, 0x4C, 0xD7, 0x74	 	// F8..FF
};

// Linear vector from sect 5.1.2

static const uint8_t kuz_lvec[16] = {
	0x94, 0x20, 0x85, 0x10, 0xC2, 0xC0, 0x01, 0xFB, 
	0x01, 0xC0, 0xC2, 0x10, 0x85, 0x20, 0x94, 0x01
};

//Common interface for encryption algorithms
const CipherAlgo kuznyechikCipherAlgo =
{
   "Kuznyechik",
   sizeof(KuznyechikContext),
   CIPHER_ALGO_TYPE_BLOCK,
   KUZNYECHIK_BLOCK_SIZE,
   (CipherAlgoInit)kuznyechikInit,
   NULL,
   NULL,
   (CipherAlgoEncryptBlock)kuznyechikEncryptBlock,
   (CipherAlgoDecryptBlock)kuznyechikDecryptBlock,
   (CipherAlgoDeinit)kuznyechikDeinit
};

// poly multiplication mod p(x) = x^8 + x^7 + x^6 + x + 1
// totally not constant time

static uint8_t kuz_mul_gf256(uint8_t x, uint8_t y)
{
/*	uint8_t z = 0;

	while( y ) {
		z ^= x * (y & 1);
		x = (x << 1) ^ (0xC3 * (x >> 7));
		y >>= 1;
	}
	return z;
*/
	uint8_t z;
	
	z = 0;
	while (y) {		
		if (y & 1)
			z ^= x;
		x = (x << 1) ^ (x & 0x80 ? 0xC3 : 0x00);
		y >>= 1;
	}
	return z;
}

// linear operation l

static void kuz_l(w128_t *w)
{
	int i, j;
	uint8_t x;
	
	// 16 rounds
	for (j = 0; j < 16; j++) {

		// An LFSR with 16 elements from GF(2^8)
		x = w->b[15];	// since lvec[15] = 1

		for (i = 14; i >= 0; i--) {
			w->b[i + 1] = w->b[i];
			x ^= kuz_mul_gf256(w->b[i], kuz_lvec[i]);
		}
		w->b[0] = x;
	}
}

// inverse of linear operation l

static void kuz_l_inv(w128_t *w)
{
	int i, j;
	uint8_t x;
	
	// 16 rounds
	for (j = 0; j < 16; j++) {

		x = w->b[0];
		for (i = 0; i < 15; i++) {
			w->b[i] = w->b[i + 1];	
			x ^= kuz_mul_gf256(w->b[i], kuz_lvec[i]);
		}
		w->b[15] = x;
	}
}

/*
//AES128-ECB OID (2.16.840.1.101.3.4.1.1)
const uint8_t AES128_ECB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x01};
//AES128-CBC OID (2.16.840.1.101.3.4.1.2)
const uint8_t AES128_CBC_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x02};
//AES128-OFB OID (2.16.840.1.101.3.4.1.3)
const uint8_t AES128_OFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x03};
//AES128-CFB OID (2.16.840.1.101.3.4.1.4)
const uint8_t AES128_CFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x04};
//AES128-GCM OID (2.16.840.1.101.3.4.1.6)
const uint8_t AES128_GCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x06};
//AES128-CCM OID (2.16.840.1.101.3.4.1.7)
const uint8_t AES128_CCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x07};

//AES192-ECB OID (2.16.840.1.101.3.4.1.21)
const uint8_t AES192_ECB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x15};
//AES192-CBC OID (2.16.840.1.101.3.4.1.22)
const uint8_t AES192_CBC_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x16};
//AES192-OFB OID (2.16.840.1.101.3.4.1.23)
const uint8_t AES192_OFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x17};
//AES192-CFB OID (2.16.840.1.101.3.4.1.24)
const uint8_t AES192_CFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x18};
//AES192-GCM OID (2.16.840.1.101.3.4.1.26)
const uint8_t AES192_GCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x1A};
//AES192-CCM OID (2.16.840.1.101.3.4.1.27)
const uint8_t AES192_CCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x1B};

//AES256-ECB OID (2.16.840.1.101.3.4.1.41)
const uint8_t AES256_ECB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x29};
//AES256-CBC OID (2.16.840.1.101.3.4.1.42)
const uint8_t AES256_CBC_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2A};
//AES256-OFB OID (2.16.840.1.101.3.4.1.43)
const uint8_t AES256_OFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2B};
//AES256-CFB OID (2.16.840.1.101.3.4.1.44)
const uint8_t AES256_CFB_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2C};
//AES256-GCM OID (2.16.840.1.101.3.4.1.46)
const uint8_t AES256_GCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2E};
//AES256-CCM OID (2.16.840.1.101.3.4.1.47)
const uint8_t AES256_CCM_OID[9] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2F};
*/


/**
 * @brief Key expansion
 * @param[in] context Pointer to the KUZNYECHIK context to initialize
 * @param[in] key Pointer to the key
 * @param[in] keyLen Length of the key
 * @return Error code
 **/

__weak_func error_t kuznyechikInit(KuznyechikContext *context, const uint8_t *key, size_t keyLen)
{
	int i, j;
	w128_t c, x, y, z;

   //Check parameters
   if(context == NULL || key == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check the length of the key
   if(keyLen != 16)
      //Report an error
      return ERROR_INVALID_KEY_LENGTH;
		
	for (i = 0; i < 16; i++) {
		// this will be have to changed for little-endian systems
		x.b[i] = key[i];
		y.b[i] = key[i + 16];
	}

	context->ek[0].q[0] = x.q[0];
	context->ek[0].q[1] = x.q[1];
	context->ek[1].q[0] = y.q[0];
	context->ek[1].q[1] = y.q[1];

	for (i = 1; i <= 32; i++) {

		// C Value
		c.q[0] = 0;
		c.q[1] = 0;
		c.b[15] = i;		// load round in lsb
		kuz_l(&c);
		
		z.q[0] = x.q[0] ^ c.q[0];
		z.q[1] = x.q[1] ^ c.q[1];
		for (j = 0; j < 16; j++)
			z.b[j] = kuz_pi[z.b[j]];
		kuz_l(&z);
		z.q[0] ^= y.q[0];
		z.q[1] ^= y.q[1];
		
		y.q[0] = x.q[0];
		y.q[1] = x.q[1];

		x.q[0] = z.q[0];
		x.q[1] = z.q[1];

		if ((i & 7) == 0) {
			context->ek[(i >> 2)].q[0] = x.q[0];
			context->ek[(i >> 2)].q[1] = x.q[1];
			context->ek[(i >> 2) + 1].q[0] = y.q[0];
			context->ek[(i >> 2) + 1].q[1] = y.q[1];
		}
	}
	for (i = 0; i < 10; i++)
		context->dk[i] = context->ek[i];
  //No error to report
  return NO_ERROR;
}


/**
 * @brief Encrypt a 16-byte block using KUZNYECHIK algorithm
 * @param[in] context Pointer to the KUZNYECHIK context
 * @param[in] input Plaintext block to encrypt
 * @param[out] output Ciphertext block resulting from encryption
 **/

void kuznyechikEncryptBlock(KuznyechikContext *context, const uint8_t *input, uint8_t *output)
{
	int i, j;
	w128_t x;

	x.q[0] = ((uint64_t *)input)[0];
	x.q[1] = ((uint64_t *)input)[1];

	for (i = 0; i < 9; i++) {

		x.q[0] ^= context->ek[i].q[0];
		x.q[1] ^= context->ek[i].q[1];

		for (j = 0; j < 16; j++)
			x.b[j] = kuz_pi[x.b[j]];
		kuz_l(&x);
	}
	((uint64_t *)output)[0] = x.q[0] ^ context->ek[9].q[0];
	((uint64_t *)output)[1] = x.q[1] ^ context->ek[9].q[1];
}


/**
 * @brief Decypt a 16-byte block using KUZNYECHIK algorithm
 * @param[in] context Pointer to the KUZNYECHIK context
 * @param[in] input Plaintext block to encrypt
 * @param[out] output Ciphertext block resulting from encryption
 **/

void kuznyechikDecryptBlock(KuznyechikContext *context, const uint8_t *input, uint8_t *output)
{
	int i, j;
	w128_t x;

	x.q[0] = ((uint64_t *)input)[0] ^ context->dk[9].q[0];
	x.q[1] = ((uint64_t *)input)[1] ^ context->dk[9].q[1];

	for (i = 8; i >= 0; i--) {
	
		kuz_l_inv(&x);
		for (j = 0; j < 16; j++)
			x.b[j] = kuz_pi_inv[x.b[j]];
	
		x.q[0] ^= context->dk[i].q[0];	
		x.q[1] ^= context->dk[i].q[1];
	}
	((uint64_t *)output)[0] = x.q[0];
	((uint64_t *)output)[1] = x.q[1];
}


/**
 * @brief Release KUZNYECHIK context
 * @param[in] context Pointer to the KUZNYECHIK context
 **/

__weak_func void kuznyechikDeinit(KuznyechikContext *context)
{
   //Clear KUZNYECHIK context
   osMemset(context, 0, sizeof(KuznyechikContext));
}

#endif
