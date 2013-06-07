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
	Disk serialized MapBlock format:
	
	(u8) version = 8
	
	basic information
	(u8) flags         = 0
	(u8) content width = 2
	(u8) params width  = 2
	
	node data
	ZLib deflate {
		for each node:
			(u16) param0
		for each node:
			(u8) param1
		for each node:
			(u8) param2
	}
	
	ZLib deflate {
		node metadata list
		(u8) version = 1
		(s16) count
		for each nodemetadata:
			()
	}
	
	static object list
	(u8) version = 0
	(u16) count
	for each stored object:
		()
	for each active object:
		()
	
	name-id mapping
	(u8) version = 0
	(u16) count
	for each name-id mapping:
		(u16) id          = ids are of what first occur in the block, start at 0
		(u16) name length
		(u8[]) name
	
	node timer list
		(u8) version = 10
		(u16) count
*/

/*
	Sample data:
	
	19	;mapblock version
	08	;flags
	02	;content width
	02	;params width
	789CEDC13101000000C2A0F54F6D0C1FA000000000000000000000000000000080B70140000001789C63000000010001000000
	FFFFFFFF ;timestamp
	00 ;nameidmapping version
	0001 ;count
	0000 ;id
	0006 ;strlen("ignore")
	69 67 6E 6F 72 65 ;string "ignore"
	0A   ;node timer version
	0000 ;node timer count
*/

#ifndef MAPCONTENT_HEADER
#define MAPCONTENT_HEADER

#include "vector.h"

LPVECTOR MapBlockCreateMappingTableAndFixNodes(MapNode *nodes, size_t nnodes);
size_t MapBlockSerialize(MapBlock *block, u8 *outbuf);
void MapNodeSerializeBulk(const MapNode *nodes, u32 nodecount,
						u8 **outbuf, size_t *outlen);
sqlite3_int64 MapBlockPosToInteger(const v3s16 pos);


#endif // MAPCONTENT_HEADER
