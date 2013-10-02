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
#define			PROCESS_STATE_TERMINATE					4L		/* Terminated process*/

#define			MAX_PROCESSES					20L				/* maximum processes: test1b */
																/* maximum processes: ProcessControlBlockTable */

#define         DO_LOCK                     1L
#define         DO_UNLOCK                   0L
#define         SUSPEND_UNTIL_LOCKED        TRUE
#define         DO_NOT_SUSPEND              FALSE

#define			PROCESS_READY_LOCK			0
#define			PROCESS_TIMER_LOCK			1
#define			PROCESS_PCB_TABLE_LOCK		2
#define			PRINTER_LOCK				3

#endif