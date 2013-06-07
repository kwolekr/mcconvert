/*-
 * Copyright (c) 2013 Ryan Kwolek <kwolekr@minetest.net>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MCCONVERT_HEADER
#define MCCONVERT_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sqlite3.h>

#define MAP_BLOCKSIZE 16
#define MAP_BLOCKNUMNODES (MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE)

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#define SWAP2(num) ((((num) >> 8) & 0x00FF) | \
					(((num) << 8) & 0xFF00))
#define SWAP4(num) ((((num) >> 24) & 0x000000FF) | \
					(((num) >> 8) & 0x0000FF00) | \
					(((num) << 8) & 0x00FF0000) | \
					(((num) << 24) & 0xFF000000))
					
#define OUTPUT_FILENAME "tehmap.sqlite"

// These parameters need to be guessed
#define MCC_MAP_CX 256
#define MCC_MAP_CY 64
#define MCC_MAP_CZ 256

#define MCC_MAPDATA_OFFSET 20630

#define MAP_NNODES (MCC_MAP_CX * MCC_MAP_CY * MCC_MAP_CZ)
#define MAP_NBLOCKS_X (MCC_MAP_CX / MAP_BLOCKSIZE)
#define MAP_NBLOCKS_Y (MCC_MAP_CY / MAP_BLOCKSIZE)
#define MAP_NBLOCKS_Z (MCC_MAP_CZ / MAP_BLOCKSIZE)

#define MINDEX(x,y,z) ((x) + ((y) * MCC_MAP_CX * MCC_MAP_CZ) + ((z) * MCC_MAP_CX))

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;					
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct _v2s16 {
	s16 X;
	s16 Y;
} v2s16;

typedef struct _v3s16 {
	s16 X;
	s16 Y;
	s16 Z;
} v3s16;

typedef struct _MapNode {
	u16 param0;
	u8 param1;
	u8 param2;
} MapNode;

typedef struct _MapBlock {
	v3s16 pos;
	MapNode data[MAP_BLOCKNUMNODES];
} MapBlock;


static inline void WriteU64(u8 *data, u64 i) {
	data[0] = ((i >> 56) & 0xff);
	data[1] = ((i >> 48) & 0xff);
	data[2] = ((i >> 40) & 0xff);
	data[3] = ((i >> 32) & 0xff);
	data[4] = ((i >> 24) & 0xff);
	data[5] = ((i >> 16) & 0xff);
	data[6] = ((i >>  8) & 0xff);
	data[7] = ((i >>  0) & 0xff);
}


static inline void WriteU32(u8 *data, u32 i) {
	data[0] = ((i >> 24) & 0xff);
	data[1] = ((i >> 16) & 0xff);
	data[2] = ((i >>  8) & 0xff);
	data[3] = ((i >>  0) & 0xff);
}


static inline void WriteU16(u8 *data, u16 i) {
	data[0] = ((i >> 8) & 0xff);
	data[1] = ((i >> 0) & 0xff);
}


static inline void WriteU8(u8 *data, u8 i) {
	data[0] = ((i >> 0) & 0xff);
}

#define InsertU8(data, i) { \
	WriteU8(data, i); \
	data += sizeof(u8); \
}

#define InsertU16(data, i) { \
	WriteU16(data, i); \
	data += sizeof(u16); \
}

#define InsertU32(data, i) { \
	WriteU32(data, i); \
	data += sizeof(u32); \
}

#define InsertU64(data, i) { \
	WriteU64(data, i); \
	data += sizeof(u64); \
}

int ZLibCompress(u8 *data, size_t datalen, u8 **out, size_t *outlen);
int ZLibDecompress(u8 *data, size_t datalen, u8 **out, size_t *outlen);
void CopyMapBlockFromMC(u8 *mcdata, s16 bx, s16 by, s16 bz, MapNode *blockdata);
void ConvertMCToMT(u8 *mcdata);
void CreateWalls();

#endif //MCCONVERT_HEADER
