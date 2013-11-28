/**************************************************************************************************
 * Filename:       SimpleDBTxt.c
 * Description:    This is a specific implemntation of a text-based db system, based on the SimpleDB module.
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

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "hal_types.h"
#include "SimpleDBTxt.h"

uint32_t sdbtGetRecordSize(void * record)
{
	return strlen(record);
}

bool sdbtCheckDeleted(void * record)
{
	return (((char *) record)[0] == SDBT_DELETED_LINE_CHARACTER);
}

bool sdbtCheckIgnored(void * record)
{
	return ((((char *) record)[0] == SDBT_BAD_FORMAT_CHARACTER)
			|| (((char *) record)[0] == SDBT_PENDING_COMMENT_FORMAT_CHARACTER));
}

void sdbtMarkDeleted(void * record)
{
	((char *) record)[0] = ';';
}

uint32_t sdbtGetRecordCount(db_descriptor * db)
{
	uint32_t recordCnt = 0;
	char * rec;
	uint32_t context;

	rec = SDB_GET_FIRST_RECORD(db, &context);

	while (rec != NULL)
	{
		recordCnt++;
		rec = SDB_GET_NEXT_RECORD(db, &context);
	}

	return recordCnt;
}

bool sdbtErrorComment(db_descriptor * db, char * record)
{
	parsingResult_t parsingResult =
	{ SDB_TXT_PARSER_RESULT_OK, 0 };
	char * pBuf = record + 1;
	char comment[MAX_SUPPORTED_RECORD_SIZE];
	uint8_t errorCode;
	uint16_t errorOffset;
	bool rc;
	char tempstr[3] =
	{ SDBT_BAD_FORMAT_CHARACTER, '\n', '\0' };

	if (record[0] == SDBT_PENDING_COMMENT_FORMAT_CHARACTER)
	{
		record[0] = SDBT_BAD_FORMAT_CHARACTER;
		sprintf(comment, "%c-------\n", SDBT_BAD_FORMAT_CHARACTER);
		rc = sdb_add_record(db, comment) && sdb_add_record(db, record);

		if (rc == TRUE)
		{
//			printf("here&\n");
			sdb_txt_parser_get_numeric_field(&pBuf, &errorCode, 1, FALSE,
					&parsingResult);
			if (errorCode > SDB_TXT_PARSER_RESULT_MAX)
			{
//printf("here0\n");
				parsingResult.code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
				parsingResult.errorLocation = pBuf - 2;
			}
			else
			{
//printf("here1 %d \n", parsingResult.code);
				sdb_txt_parser_get_numeric_field(&pBuf, (uint8_t *) &errorOffset, 2,
						FALSE, &parsingResult);
				if (errorOffset >= strlen(record))
				{
					parsingResult.code = SDB_TXT_PARSER_RESULT_VALUE_OUT_OF_RANGE;
					parsingResult.errorLocation = pBuf - 2;
				}
			}
//			printf("here2\n");
			if (parsingResult.code == SDB_TXT_PARSER_RESULT_OK)
			{
//				printf("here3\n");
//				printf("%s\n%d %d\n", record, errorOffset - 7 + 1, errorCode);
				sprintf(comment, "%cERROR:%*s %s\n", SDBT_BAD_FORMAT_CHARACTER,
						errorOffset - 7 + 1, "^", parsingErrorStrings[errorCode]);
			}
			else
			{
//				printf("here4\n");
//				printf("%s\n%d %d\n", record, parsingResult.errorLocation - record - 1 + 1, parsingResult.code);
				sprintf(comment, "%c%*s %s (%s)\n", SDBT_BAD_FORMAT_CHARACTER,
						parsingResult.errorLocation - record - 1 + 1, "^",
						"BAD_ERROR_DESCRIPTION_HEADER",
						parsingErrorStrings[parsingResult.code]);
			}
//			printf("here5\n");

			rc = sdb_add_record(db, comment) && sdb_add_record(db, tempstr);
		}
	}
	else
	{
		rc = sdb_add_record(db, record);
	}

	return rc;
}

void sdbtMarkError(db_descriptor * db, char * record,
		parsingResult_t * parsingResult)
{
	//mark record as 'bad format'
	record[0] = SDBT_PENDING_COMMENT_FORMAT_CHARACTER;
	record[1] = '0' + parsingResult->code;
	record[2] = ',';
	sprintf(record + 3, "%3d", (int) (parsingResult->errorLocation - record));
	record[6] = ','; //must overwrite the '\0' from the above sprintf
	sdb_modify_last_accessed_record(db, record);
}

