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
 * db.c - 
 *    Routines related to database manipulation 
 */

#include "mcconvert.h"
#include "mapcontent.h"
#include "db.h"
 
sqlite3 *m_database = NULL;
sqlite3_stmt *m_database_read  = NULL;
sqlite3_stmt *m_database_write = NULL;
sqlite3_stmt *m_database_list  = NULL;


///////////////////////////////////////////////////////////////////////////////


void DBCreate() {
	int e = sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `blocks` ("
			"`pos` INT NOT NULL PRIMARY KEY,"
			"`data` BLOB"
		");", NULL, NULL, NULL);
	if (e == SQLITE_ABORT)
		fprintf(stderr, "Could not create database structure\n");
	else
		printf("Database structure was created\n");
}


int DBVerify() {
	if (!m_database) {
		int needs_create;
		int d;

		needs_create = 1; //!stat(OUTPUT_FILENAME, NULL);

		d = sqlite3_open_v2(OUTPUT_FILENAME, &m_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if (d != SQLITE_OK) {
			fprintf(stderr, "WARNING: Database failed to open: %s\n", sqlite3_errmsg(m_database));
			return 0;
		}

		if (needs_create)
			DBCreate();

		d = sqlite3_prepare(m_database, "SELECT `data` FROM `blocks` WHERE `pos`=? LIMIT 1", -1, &m_database_read, NULL);
		if (d != SQLITE_OK) {
			fprintf(stderr, "WARNING: Database read statment failed to prepare: %s\n", sqlite3_errmsg(m_database));
			return 0;
		}

		d = sqlite3_prepare(m_database, "REPLACE INTO `blocks` VALUES(?, ?)", -1, &m_database_write, NULL);
		if (d != SQLITE_OK) {
			fprintf(stderr, "WARNING: Database write statment failed to prepare: %s\n", sqlite3_errmsg(m_database));
			return 0;
		}

		d = sqlite3_prepare(m_database, "SELECT `pos` FROM `blocks`", -1, &m_database_list, NULL);
		if (d != SQLITE_OK) {
			fprintf(stderr, "WARNING: Database list statment failed to prepare: %s\n", sqlite3_errmsg(m_database));
			return 0;
		}
		
		printf("Database opened\n");
	}
	
	return 1;
}


int DBSaveMapBlock(MapBlock *block) {
	DBVerify();

	// To make things easy, just guess the needed size of the buffer.
	// This seems like a generous enough amount.
	u8 *output = malloc(0x20000);
	size_t outlen = MapBlockSerialize(block, output);
	if (outlen > 0x20000) {
		fprintf(stderr, "UH OH, serialized output was too big\n");
		exit(0);
	}

	int success = 0;
	if (sqlite3_exec(m_database, "BEGIN;", NULL, NULL, NULL) != SQLITE_OK)
		fprintf(stderr, "WARNING: begin save failed, saving might be slow.\n");

	if (sqlite3_bind_int64(m_database_write, 1, MapBlockPosToInteger(block->pos)) != SQLITE_OK)
		fprintf(stderr, "WARNING: Block position failed to bind: %s\n", sqlite3_errmsg(m_database));
		
	if (sqlite3_bind_blob(m_database_write, 2, output, outlen, NULL) != SQLITE_OK)
		fprintf(stderr, "WARNING: Block data failed to bind: %s\n", sqlite3_errmsg(m_database));
		
	int written = sqlite3_step(m_database_write);
	if (written != SQLITE_DONE)
		fprintf(stderr, "ERROR: Block failed to save (%d, %d, %d) %s\n",
				block->pos.X, block->pos.Y, block->pos.Z, sqlite3_errmsg(m_database));
	
	success = 1;
	
	sqlite3_reset(m_database_write);
	if (sqlite3_exec(m_database, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK)
		fprintf(stderr, "WARNING: end save failed, map might not have saved.\n");

	free(output);

	return success;
}
