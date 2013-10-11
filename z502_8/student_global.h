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
#define			PROCESS_STATE_SUSPEND					5L		/* Suspend process*/
#define			PROCESS_STATE_SUSPEND_MESSAGE			6L		/* Manually suspend because of SEND RECEIVE message */

#define			MAX_PROCESSES							10L				/* maximum processes: test1b. Set to 10 otherwise z502.c displays error */
																		/* maximum processes: ProcessControlBlockTable */

#define         DO_LOCK									1L
#define         DO_UNLOCK								0L
#define         SUSPEND_UNTIL_LOCKED					TRUE
#define         DO_NOT_SUSPEND							FALSE

#define			PROCESS_READY_LOCK						1L
#define			PROCESS_TIMER_LOCK						2L
#define			PROCESS_PCB_TABLE_LOCK					3L
#define			PRINTER_LOCK							4L
#define			SUSPEND_LOCK							5L
#define			PRIORITY_LOCK							6L
#define			MSG_SUSPEND_LOCK						7L


#define			PROCESS_SUSPEND_INVALID_PROCESS_ID		1L
#define			PROCESS_RESUME_INVALID_PROCESS_ID		2L
#define			PROCESS_SUSPEND_LEGAL					0L
#define			PROCESS_RESUME_LEGAL					0L
#define			PROCESS_SUSPEND_INVALID_SUSPENDED		3L
#define			PROCESS_RESUME_INVALID_RESUMED			4L


#define			START_TIMER_BEFORE						10L
#define			START_TIMER_AFTER						11L
#define			TERMINATE_BEFORE						12L
#define			TERMINATE_AFTER							13L
#define			CREATE_BEFORE							14L
#define			CREATE_AFTER							15L
#define			INTERRUPT_BEFORE						16L
#define			INTERRUPT_AFTER							17L
#define			SUSPEND_BEFORE							18L
#define			SUSPEND_AFTER							19L
#define			RESUME_BEFORE							20L
#define			RESUME_AFTER							21L
#define			DISPATCH_BEFORE							22L
#define			DISPATCH_AFTER							23L
#define			PRIORITY_BEFORE							24L
#define			PRIORITY_AFTER							25L


#define			MAX_PRIORITY_ALLOWED					500L

#define			PRIORITY_LEGAL							0L
#define			PRIORITY_ILLEGAL_PROCESS_ID				1L
#define			PRIORITY_ILLEGAL_PRIORITY				2L


#define			LEGAL_MESSAGE_LENGTH					(INT32)64

#define			PROCESS_SEND_SUCCESS					0L
#define			PROCESS_SEND_ILLEGAL_PID				1L
#define			PROCESS_SEND_ILLEGAL_MSG_LENGTH			2L
#define			PROCESS_SEND_CREATE_OVER_MAX			3L

#define			PROCESS_RECEIVE_SUCCESS					0L
#define			PROCESS_RECEIVE_ILLEGAL_PID				1L
#define			PROCESS_RECEIVE_ILLEGAL_MSG_LENGTH		2L

#define			MAX_MESSAGES							25L
#define			MAX_MESSAGES_IN_QUEUE					15L

#define			MESSAGE_STATE_FREE						1L
#define			MESSAGE_STATE_RECEIVED					2L
#endif