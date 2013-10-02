#ifndef STUDENT_GLOBAL_H_
#define STUDENT_GLOBAL_H_

        /* Definition of return codes for PROCESS                          */

#define         PROCESS_CREATED_SUCCESS                 0L
#define			PROCESS_LEGAL							0L
#define         PROCESS_CREATED_ERROR_SAME_NAME         1L
#define			PROCESS_CREATED_ILLEGAL_PRIORITY        2L
#define			PROCESS_CREATED_NOT_ENOUGH_MEMORY       3L
#define			PROCESS_CREATED_MAX_EXCEED				4L

#define			PROCESS_ID_EXIST						0L
#define			PROCESS_ID_NOT_EXIST					1L
#define			PROCESS_ID_NOT_VALID					-1L


#define			PROCESS_TERMINATE_INVALID_PROCESS_ID    1L

#define			PROCESS_STATE_SLEEP						1L		/* Waiting for resource */
#define			PROCESS_STATE_RUNNING					2L
#define			PROCESS_STATE_CREATE					3L
#define			PROCESS_STATE_READY						3L		/* Ready to run */


#endif