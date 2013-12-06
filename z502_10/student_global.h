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
#define			DISK_QUEUE_LOCK							8L
#define			DISK_ACCESS_LOCK						9L
#define			FRAME_TABLE_LOCK						10L
#define			SHADOW_TABLE_LOCK						11L


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


#define			OUT_OF_UPPER_BOUND_LOGICAL_ADDRESS		1023
#define			OUT_OF_LOWER_BOUND_LOGICAL_ADDRESS		0

#define			DISK_IO_BUFF_SIZE						16
#define			PG_TABLE_LENGTH							1024
#define			FRAME_LENGTH							64

#define			FRAME_AVAILABLE							0x8000
#define			FRAME_USED								0x4000

#define			BUFFER_DISK_IO_SIZE						16

#define         DISK_INTERRUPT_DISK3					(short)7
#define         DISK_INTERRUPT_DISK4					(short)8
#define         DISK_INTERRUPT_DISK5					(short)9
#define         DISK_INTERRUPT_DISK6					(short)10
#define         DISK_INTERRUPT_DISK7					(short)11
#define         DISK_INTERRUPT_DISK8					(short)12
#define         DISK_INTERRUPT_DISK9					(short)13
#define         DISK_INTERRUPT_DISK10					(short)14
#define         DISK_INTERRUPT_DISK11					(short)15
#define         DISK_INTERRUPT_DISK12					(short)16

#define			NOT_WRITE_YET							1L
#define			NOT_READ_YET							2L
#define			ALREADY_WRITE							3L
#define			ALREADY_READ							4L

#define			PAGE_FIFO_ALGO							20L
#define			PAGE_LRU_ALGO							30L

#define			MAX_NUMBER_OF_SECTORS					1600


#endif