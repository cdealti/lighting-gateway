/**************************************************************************************************
 * Filename:       interface_scenelist.c
 * Description:    Socket Remote Procedure Call Interface - sample device application.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "interface_scenelist.h"
#include "hal_types.h"
#include "SimpleDBTxt.h"

static db_descriptor * db;

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
	char* name;
	uint16 groupId;
} scene_key_NA_GID;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */ 
static char * sceneListComposeRecord(sceneRecord_t *group, char * record);
static sceneRecord_t * sceneListParseRecord(char * record);
static int sceneListCheckKeyId(char * record, uint8_t * key);
static int sceneListCheckKeyNameGid(char * record, scene_key_NA_GID * key);
static sceneRecord_t * sceneListGetSceneByNameGid(char *sceneNameStr, uint16_t groupId);
static uint8_t sceneListGetUnusedSceneId(void);

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

void sceneListInitDatabase(char * dbFilename)
{
	db = sdb_init_db(dbFilename, sdbtGetRecordSize, sdbtCheckDeleted,
			sdbtCheckIgnored, sdbtMarkDeleted,
			(consolidation_processing_f) sdbtErrorComment, SDB_TYPE_TEXT, 0);
	sdb_consolidate_db(&db);
}

static char * sceneListComposeRecord(sceneRecord_t *scene, char * record)
{
	sceneMembersRecord_t *sceneMembers;

	sprintf(record, "        0x%04X , 0x%02X , \"%s\"", //leave a space at the beginning to mark this record as deleted if needed later, or as bad format (can happen if edited manually). Another space to write the reason of bad format.
			scene->groupId, scene->sceneId, scene->name ? scene->name : "");

	sceneMembers = scene->members;

	while (sceneMembers != NULL)
	{
		sprintf(record + strlen(record), " , 0x%04X , 0x%02X",
				sceneMembers->nwkAddr, sceneMembers->endpoint);
		sceneMembers = sceneMembers->next;
	}

	sprintf(record + strlen(record), "\n");

	return record;
}

#define MAX_SUPPORTED_SCENE_NAME_LENGTH 32
#define MAX_SUPPORTED_SCENE_MEMBERS 20

static sceneRecord_t * sceneListParseRecord(char * record)
{
	char * pBuf = record + 1; //+1 is to ignore the 'for deletion' mark that may just be added to this record.
	static sceneRecord_t scene;
	static char sceneName[MAX_SUPPORTED_SCENE_NAME_LENGTH + 1];
	static sceneMembersRecord_t member[MAX_SUPPORTED_SCENE_MEMBERS];
	sceneMembersRecord_t ** nextMemberPtr;
	parsingResult_t parsingResult =
	{ SDB_TXT_PARSER_RESULT_OK, 0 };
	int i;

	if (record == NULL)
	{
		return NULL;
	}

	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *) &scene.groupId, 2, FALSE,
			&parsingResult);
	sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *) &scene.sceneId, 1, FALSE,
			&parsingResult);
	sdb_txt_parser_get_quoted_string(&pBuf, sceneName,
			MAX_SUPPORTED_SCENE_NAME_LENGTH, &parsingResult);
	nextMemberPtr = &scene.members;
	for (i = 0;
			(parsingResult.code == SDB_TXT_PARSER_RESULT_OK)
					&& (i < MAX_SUPPORTED_SCENE_MEMBERS); i++)
	{
		*nextMemberPtr = &(member[i]);
		sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *) &(member[i].nwkAddr), 2,
				FALSE, &parsingResult);
		sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *) &(member[i].endpoint),
				1, FALSE, &parsingResult);
		nextMemberPtr = &(member[i].next);
	}
	*nextMemberPtr = NULL;

	if ((parsingResult.code != SDB_TXT_PARSER_RESULT_OK)
			&& (parsingResult.code != SDB_TXT_PARSER_RESULT_REACHED_END_OF_RECORD))
	{
		sdbtMarkError(db, record, &parsingResult);
		return NULL;
	}

	if (strlen(sceneName) > 0)
	{
		scene.name = sceneName;
	}
	else
	{
		scene.name = NULL;
	}

	return &scene;
}

static int sceneListCheckKeyId(char * record, uint8_t * key)
{
	sceneRecord_t * scene;
	int result = SDB_CHECK_KEY_NOT_EQUAL;

	scene = sceneListParseRecord(record);
	if (scene == NULL)
	{
		return SDB_CHECK_KEY_ERROR;
	}

	if (scene->sceneId == *key)
	{
		result = SDB_CHECK_KEY_EQUAL;
	}

	return result;
}

static int sceneListCheckKeyNameGid(char * record, scene_key_NA_GID * key)
{
	sceneRecord_t * scene;
	int result = SDB_CHECK_KEY_NOT_EQUAL;

	scene = sceneListParseRecord(record);
	if (scene == NULL)
	{
		return SDB_CHECK_KEY_ERROR;
	}

	if ( (strcmp(scene->name, key->name) == 0) && (scene->groupId == key->groupId) )
	{
		result = SDB_CHECK_KEY_EQUAL;
	}

	return result;
}

static sceneRecord_t * sceneListGetSceneByNameGid(char *sceneNameStr, uint16_t groupId)
{
	char * rec;
	scene_key_NA_GID key;

	key.groupId = groupId;
	key.name = sceneNameStr;

	rec = SDB_GET_UNIQUE_RECORD(db, &key, (check_key_f)sceneListCheckKeyNameGid);
	if (rec == NULL)
	{
		return NULL;
	}

	return sceneListParseRecord(rec);
}

static uint8_t sceneListGetUnusedSceneId(void)
{
	static uint8_t lastUsedSceneId = 0;

	lastUsedSceneId++;

	while (SDB_GET_UNIQUE_RECORD(db, &lastUsedSceneId, (check_key_f)sceneListCheckKeyId)
			!= NULL)
	{
		lastUsedSceneId++;
	}

	return lastUsedSceneId;
}

/*********************************************************************
 * @fn      sceneListAddScene
 *
 * @brief   add a scene to the scene list.
 *
 * @return  sceneId
 */
uint8_t sceneListAddScene( char *sceneNameStr, uint16_t groupId )
{
	sceneRecord_t *exsistingScene;
	sceneRecord_t newScene;
	char rec[MAX_SUPPORTED_RECORD_SIZE];

	exsistingScene = sceneListGetSceneByNameGid(sceneNameStr, groupId);

	if (exsistingScene != NULL)
	{
		return exsistingScene->sceneId;
	}

	newScene.groupId = groupId;
	newScene.sceneId = sceneListGetUnusedSceneId();
	newScene.name = sceneNameStr;
	newScene.members = NULL;

	sceneListComposeRecord(&newScene, rec);

	sdb_add_record(db, rec);

	//printf("SceneListAddScene--\n");

	return newScene.sceneId;
}

/*********************************************************************
 * @fn      sceneListAddScene
 *
 * @brief   add a scene to the scene list.
 *
 * @return  sceneId
 */
uint8_t sceneListGetSceneId( char *sceneNameStr, uint16_t groupId )
{
  uint8_t sceneId = 0;
  sceneRecord_t *scene;

  scene = sceneListGetSceneByNameGid(sceneNameStr, groupId);
  
  if( scene == NULL)
  {     
    sceneId = -1;
  }
  else
  {
    sceneId = scene->sceneId;
  }
  
  //printf("sceneListGetSceneId--\n");
  
  return sceneId;
}

/*********************************************************************
 * @fn      sceneListGetNextScene
 *
 * @brief   Return the next scene in the list.
 *
 * @param   context	Pointer to the current scene record
 *
 * @return  sceneRecord_t, return next scene record in the DB
 */
sceneRecord_t* sceneListGetNextScene(uint32_t *context)
{
	char * rec;
	sceneRecord_t *scene;

	do
	{
		rec = SDB_GET_NEXT_RECORD(db,context);

		if (rec == NULL)
		{
			return NULL;
		}

		scene = sceneListParseRecord(rec);
	}
	while (scene == NULL); //in case of a bad-format record - skip it and read the next one

	return scene;
}
