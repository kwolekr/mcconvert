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

/* 
 * mcconvert.c - 
 *    A utility to convert an extracted Minecraft Classic map to
 *    Minetest's 0.4.x SQLite3 format.
 */

#include "mcconvert.h"
#include "vector.h"
#include "mapcontent.h"
#include "db.h"

#include <zlib.h>


///////////////////////////////////////////////////////////////////////////////


int main(int argc, char *argv[]) {
	char fn_output_buf[256];
	char *fn_input, *fn_output;
	FILE *fin;
	uint32_t magic;
	uint8_t version;
	
	if (argc < 2) {
		fprintf(stderr, "Insufficient number of arguments\n");
		return 1;
	}
	
	fn_input  = argv[1];
	fn_output = fn_output_buf;
	strlcpy(fn_output_buf, fn_input, sizeof(fn_output_buf));
	
	fin = fopen(fn_input, "rb");
	if (!fin) {
		perror("Could not open input file for read");
		return 1;
	}
	
	fread(&magic, 1, sizeof(magic), fin);
	magic = SWAP4(magic);
	if (magic != 0x271bb788) {
		fprintf(stderr, "Header mismatch, invalid Minecraft Classic map file\n");
		fclose(fin);
		return 1;
	}
	
	fread(&version, 1, sizeof(version), fin);
	if (version != 2) {
		fprintf(stderr, "Unhandled map file version\n");
		fclose(fin);
		return 1;
	}
	
#if 0
	fseek(fin, 6, SEEK_CUR); // ac ed 00 05 73 72
	for (i = 0; i != 3; i++) {
		char *str;
		uint16_t slen;
		
		fread(&slen, 1, sizeof(slen), fin);
		slen = SWAP2(slen);
		str = malloc(slen + 1);
		
		fread(str, slen, 1, fin);
		str[slen] = 0;
		
		printf("str: %s\n", str);
		free(str);
	}
#endif
	
	
	fseek(fin, MCC_MAPDATA_OFFSET, SEEK_SET);
	u8 *mcdata = malloc(MAP_NNODES);
	int nread = fread(mcdata, MAP_NNODES, 1, fin);
	if (nread != 1) {
		fprintf(stderr, "Failed to read node data\n");
		return 1;
	}

	ConvertMCToMT(mcdata);
	CreateWalls();
	//free(mcdata);

	printf("done!\n");

	if (m_database_read)
		sqlite3_finalize(m_database_read);
	if (m_database_write)
		sqlite3_finalize(m_database_write);
	if (m_database)
		sqlite3_close(m_database);

	return 0;
}


void ConvertMCToMT(u8 *mcdata) {
	int bx, by, bz;
	MapBlock *block = malloc(sizeof(MapBlock));
	
	for (by = 0; by != MAP_NBLOCKS_Y; by++) {
		for (bz = 0; bz != MAP_NBLOCKS_Z; bz++) {
			for (bx = 0; bx != MAP_NBLOCKS_X; bx++) {
				
				CopyMapBlockFromMC(mcdata, bx, by, bz, block->data);
				block->pos.X = bx;
				block->pos.Y = by;
				block->pos.Z = bz;
				DBSaveMapBlock(block);
			}
		}
	}
	free(block);
}


void CopyMapBlockFromMC(u8 *mcdata, s16 bx, s16 by, s16 bz, MapNode *blockdata) {
	int x, y, z;
	int i = 0;
	
	bx *= MAP_BLOCKSIZE;
	by *= MAP_BLOCKSIZE;
	bz *= MAP_BLOCKSIZE;
	
	bx = 255 - bx;
	
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			// X coordinate needs to be inverted for some reason
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				int index = MINDEX(bx - x, by + y, bz + z);
				blockdata[i].param0 = mcdata[index];
				blockdata[i].param1 = 0x0F; // full lighting in daytime for now
				blockdata[i].param2 = 0;
				i++;
			}
		}
	}
}


void CreateWalls() {
	int bx, by, bz;
	int x, y, z;
	int i;
	MapBlock *mblock = malloc(sizeof(MapBlock));
	MapBlock *block  = malloc(sizeof(MapBlock));
	
	////////////////-y
	i = 0;
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				mblock->data[i].param0 = (y == MAP_BLOCKSIZE - 1) ? 7 : 0;
				mblock->data[i].param1 = 0x0F;
				mblock->data[i].param2 = 0;
				i++;
			}
		}
	}
	for (bz = 0; bz != MAP_NBLOCKS_Z; bz++) {
		for (bx = 0; bx != MAP_NBLOCKS_X; bx++) {
			block->pos.X = bx;
			block->pos.Y = -1;
			block->pos.Z = bz;
			
			memcpy(block->data, mblock->data, sizeof(block->data));
			DBSaveMapBlock(block);
		}
	}
	
	////////////////-x
	i = 0;
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				mblock->data[i].param0 = (x == MAP_BLOCKSIZE - 1) ? 7 : 0;
				mblock->data[i].param1 = 0x0F;
				mblock->data[i].param2 = 0;
				i++;
			}
		}
	}
	for (bz = 0; bz != MAP_NBLOCKS_Z; bz++) {
		for (by = 0; by != MAP_NBLOCKS_Y / 2; by++) {
			block->pos.X = -1;
			block->pos.Y = by;
			block->pos.Z = bz;
			
			memcpy(block->data, mblock->data, sizeof(block->data));
			DBSaveMapBlock(block);
		}
	}
	
	////////////////+x
	i = 0;
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				mblock->data[i].param0 = (x == 0) ? 7 : 0;
				mblock->data[i].param1 = 0x0F;
				mblock->data[i].param2 = 0;
				i++;
			}
		}
	}
	for (bz = 0; bz != MAP_NBLOCKS_Z; bz++) {
		for (by = 0; by != MAP_NBLOCKS_Y / 2; by++) {
			block->pos.X = MAP_NBLOCKS_X;
			block->pos.Y = by;
			block->pos.Z = bz;
			
			memcpy(block->data, mblock->data, sizeof(block->data));
			DBSaveMapBlock(block);
		}
	}
	
	////////////////-z
	i = 0;
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				mblock->data[i].param0 = (z == MAP_BLOCKSIZE - 1) ? 7 : 0;
				mblock->data[i].param1 = 0x0F;
				mblock->data[i].param2 = 0;
				i++;
			}
		}
	}
	for (by = 0; by != MAP_NBLOCKS_Y / 2; by++) {
		for (bx = 0; bx != MAP_NBLOCKS_X; bx++) {
			block->pos.X = bx;
			block->pos.Y = by;
			block->pos.Z = -1;
			
			memcpy(block->data, mblock->data, sizeof(block->data));
			DBSaveMapBlock(block);
		}
	}
	
	////////////////+z
	i = 0;
	for (z = 0; z != MAP_BLOCKSIZE; z++) {
		for (y = 0; y != MAP_BLOCKSIZE; y++) {
			for (x = 0; x != MAP_BLOCKSIZE; x++) {
				mblock->data[i].param0 = (z == 0) ? 7 : 0;
				mblock->data[i].param1 = 0x0F;
				mblock->data[i].param2 = 0;
				i++;
			}
		}
	}
	for (by = 0; by != MAP_NBLOCKS_Y / 2; by++) {
		for (bx = 0; bx != MAP_NBLOCKS_X; bx++) {
			block->pos.X = bx;
			block->pos.Y = by;
			block->pos.Z = MAP_NBLOCKS_Z;
			
			memcpy(block->data, mblock->data, sizeof(block->data));
			DBSaveMapBlock(block);
		}
	}
	
	free(block);
	free(mblock);
}


/////////////////// Zlib wrappers


int ZLibCompress(u8 *data, size_t datalen, u8 **out, size_t *outlen) {
	z_stream z;
	int status = 0;
	int ret;
	unsigned char *outbuf;
	size_t outbuflen;
	
	*out    = NULL;
	*outlen = 0;

	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	ret = deflateInit(&z, -1);
	if (ret != Z_OK) {
		fprintf(stderr, "compressZlib: deflateInit failed");
		return 0;
	}
	
	outbuflen = deflateBound(&z, datalen);
	outbuf    = malloc(outbuflen);

	z.next_in   = (Bytef *)data;
	z.avail_in  = datalen;
	z.next_out  = outbuf;
	z.avail_out = outbuflen;
	status = deflate(&z, Z_FINISH);

	deflateEnd(&z);

	if (status != Z_STREAM_END) {
		fprintf(stderr, "compressZlib: deflate failed");
		return 0;
	}

	*outlen = outbuflen - z.avail_out;
	*out    = outbuf;
	return 1;
}


int ZLibDecompress(u8 *data, size_t datalen, u8 **out, size_t *outlen) {
	z_stream z;
	int status = 0;
	int ret;
	unsigned char *outbuf;
	size_t outbuflen;
	
	*out    = NULL;
	*outlen = 0;

	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	ret = deflateInit(&z, -1);
	if (ret != Z_OK) {
		fprintf(stderr, "compressZlib: deflateInit failed");
		return 0;
	}
	
	outbuflen = deflateBound(&z, datalen);
	outbuf    = malloc(outbuflen);

	z.next_in   = (Bytef *)data;
	z.avail_in  = datalen;
	z.next_out  = outbuf;
	z.avail_out = outbuflen;
	status = deflate(&z, Z_FINISH);

	deflateEnd(&z);

	if (status != Z_STREAM_END) {
		fprintf(stderr, "compressZlib: deflate failed");
		return 0;
	}

	*outlen = outbuflen - z.avail_out;
	*out    = outbuf;
	return 1;
}
