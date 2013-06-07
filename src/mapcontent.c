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
 * mapcontent.c -
 *    Routines pertaining to map node names, name-id mapping, and map block serialization
 */

#include "mcconvert.h"
#include "mapcontent.h"

const char *node_names[] = {
	"air",
	"default:stone",
	"default:dirt_with_grass",
	"default:dirt",
	"default:cobble",
	"default:wood",
	"default:sapling",
	"default:cloud",	//actually bedrock, but this is indestructable as well
	"default:water_flowing",
	"default:water_source",
	
	"default:lava_flowing",
	"default:lava_source",
	"default:sand",
	"default:gravel",
	"default:stone_with_mese",
	"default:stone_with_iron",
	"default:stone_with_coal",
	"default:tree",
	"default:leaves",
	"default:nyancat_rainbow", //sponge...
	
	"default:glass",
	"wool:red",       //red
	"wool:orange",    //orange
	"wool:yellow",    //yellow
	"wool:green",     //lightgreen
	"wool:green",     //green
	"wool:darkgreen", //aquagreen
	"wool:cyan",      //cyan
	"wool:blue",      //lightblue
	"wool:blue",      //blue
	"wool:violet",    //purple
	"wool:violet",    //lightpurple
	"wool:pink",      //pink
	"wool:magenta",   //darkpink
	"wool:dark_grey", //darkgrey
	"wool:grey",      //lightgrey
	"wool:white",     //white
	"", //yellow flower
	"", //red flower
	"", //mushroom1
	
	"", //mushroom2
	"default:mese",
	"default:steelblock",
	"default:cobble", //full step
	"stairs:slab_cobble", //half step
	"default:brick",
	"", //tnt
	"default:bookshelf",
	"default:mossycobble",
	"default:obsidian"
};


///////////////////////////////////////////////////////////////////////////////


sqlite3_int64 MapBlockPosToInteger(const v3s16 pos) {
	return (sqlite3_int64)pos.Z * 16777216 +
		   (sqlite3_int64)pos.Y * 4096 +
		   (sqlite3_int64)pos.X;
}


LPVECTOR MapBlockCreateMappingTableAndFixNodes(MapNode *nodes, size_t nnodes) {
	u8 global_to_relative_id_map[ARRAYLEN(node_names)];
	LPVECTOR names_seen = NULL;
	int i;
	u8 id, global_id, relative_id, num_ids = 0;
	
	memset(global_to_relative_id_map, 0xFF, sizeof(global_to_relative_id_map));

	for (i = 0; i != nnodes; i++) {
		global_id = nodes[i].param0;
		if (global_id >= ARRAYLEN(node_names)) {
			fprintf(stderr, "ERROR: Invalid node ID 0x%04x\n", (u16)global_id);
			return NULL;
		}
		
		relative_id = global_to_relative_id_map[global_id];
		if (relative_id != 0xFF) {
			id = relative_id;
		} else {
			if (*node_names[global_id]) {
				id = num_ids;
				num_ids++;
				
				global_to_relative_id_map[global_id] = id;
				VectorAdd(&names_seen, node_names[global_id]);
			} else {
				id = 0; //just make it anything, whatever the first node was
			}
		}
		nodes[i].param0 = id;
	}
	
	return names_seen;
}


size_t MapBlockSerialize(MapBlock *block, u8 *outbuf) {
	u8 *os = outbuf;
	u8 *compressed_data;
	size_t compressed_len;
	LPVECTOR names_seen;
	
	// MapBlock serialization version
	InsertU8(os, 25);
	
	// Flag byte
	u8 flags = 0;
	InsertU8(os, flags);
	
	// Bulk node data
	u32 nodecount = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;

	u8 content_width = 2;
	u8 params_width  = 2;
	InsertU8(os, content_width);
	InsertU8(os, params_width);
	
	names_seen = MapBlockCreateMappingTableAndFixNodes(block->data, nodecount);
	if (!names_seen) {
		fprintf(stderr, "crap\n");
		return 0;
	}
	
	MapNodeSerializeBulk(block->data, nodecount, &compressed_data, &compressed_len);
	memcpy(os, compressed_data, compressed_len);
	os += compressed_len;
	free(compressed_data);
	
	// Node metadata
	u8 buf[4];
	WriteU8(buf, 0); //version 1 for real data, 0 for "go away"
	ZLibCompress(buf, 1, &compressed_data, &compressed_len);
	
	memcpy(os, compressed_data, compressed_len);
	os += compressed_len;
	free(compressed_data);

	// Static objects
	InsertU8(os, 0);
	InsertU16(os, 0);

	// Timestamp
	InsertU32(os, 0xFFFFFFFF);

	// Write block-specific node definition id mapping
	InsertU8(os, 0);
	InsertU16(os, names_seen->numelem); //num ids
	int i;
	for (i = 0; i != names_seen->numelem; i++) {
		size_t len;
		
		InsertU16(os, i);
		len = strlen(names_seen->elem[i]);
		InsertU16(os, len);
		memcpy(os, names_seen->elem[i], len);
		os += len;
	}
	free(names_seen);

	InsertU8(os, 2+4+4);
	InsertU16(os, 0); //number of timers, none

	return os - outbuf;
}


void MapNodeSerializeBulk(const MapNode *nodes, u32 nodecount,
						u8 **outbuf, size_t *outlen) {
	unsigned int i;
	size_t datalen = nodecount * sizeof(MapNode);
	u8 *databuf = malloc(datalen);
	u8 *d = databuf;

	// Serialize content
	for (i = 0; i != nodecount; i++) {
		WriteU16(d, nodes[i].param0);
		d += sizeof(nodes[i].param0);
	}

	// Serialize param1
	for (i = 0; i != nodecount; i++) {
		WriteU8(d, nodes[i].param1);
		d += sizeof(nodes[i].param1);
	}

	// Serialize param2
	for (i = 0; i != nodecount; i++) {
		WriteU8(d, nodes[i].param2);
		d += sizeof(nodes[i].param2);
	}

	ZLibCompress(databuf, datalen, outbuf, outlen);
	
	free(databuf);
}
