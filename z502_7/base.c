/************************************************************************

        This code forms the base of the operating system you will
        build.  It has only the barest rudiments of what you will
        eventually construct; yet it contains the interfaces that
        allow test.c and z502.c to be successfully built together.

        Revision History:
        1.0 August 1990
        1.1 December 1990: Portability attempted.
        1.3 July     1992: More Portability enhancements.
                           Add call to sample_code.
        1.4 December 1992: Limit (temporarily) printout in
                           interrupt handler.  More portability.
        2.0 January  2000: A number of small changes.
        2.1 May      2001: Bug fixes and clear STAT_VECTOR
        2.2 July     2002: Make code appropriate for undergrads.
                           Default program start is in test0.
        3.0 August   2004: Modified to support memory mapped IO
        3.1 August   2004: hardware interrupt runs on separate thread
        3.11 August  2004: Support for OS level locking
		4.0  July    2013: Major portions rewritten to support multiple threads
************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include              <stdlib.h>

#include			 "student_queue.h"
#include			 "student_global.h"

// These loacations are global and define information about the page table
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;

extern void          *TO_VECTOR [];

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ",
                            "get_pid  ", "create   ", "term_proc",
                            "suspend  ", "resume   ", "ch_prior ",
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };


/**************************************************************** <hdduong **********************************************************************************/
extern						INT16								Z502_MODE;
INT32						global_process_id = 0;													// keep track of process_id  
INT32						number_of_processes_created = 0;										// keep track of number processes created. It might need lock
																									// = global_process_id but easier to read	
INT32						ready_queue_result;														// lock for ready queue
INT32						timer_queue_result;														// lock for timer queue
INT32						suspend_list_result;													// lock for suspend list
INT32						printer_lock_result;													// used for locking printing code
INT32						priority_lock_result;													// used for locking priority changed code
INT32						message_lock_result;													// used for locking global message queue
INT32						message_suspend_list_result;											// used for locking suspend list send/receive message

INT32						generate_interrupt_immediately  = 1 ;									// use for generate interrupt when Timer passed the wake-up time

BOOL						already_run_main_test = FALSE;
BOOL						use_priority_queue = FALSE;												// use priority queue or not

Message						*Message_Table[MAX_MESSAGES];											// use to store messages exchange between processes
INT32						global_msg_id = 0;														// keep track of number of message in the system
INT32						number_of_message_created;												// hom many messages is stored in each process and whole OS

ProcessControlBlock			*TimerQueueHead;														// TimerQueue 
ProcessControlBlock			*ReadyQueueHead;														// ReadyQueue
ProcessControlBlock			*SuspendListHead;														// SuspendList
ProcessControlBlock			*MessageSuspendListHead;												// MessageSuspendList

ProcessControlBlock			*PCB_Transfer_Timer_To_Ready;											// used to transfer PCB in interrupt
ProcessControlBlock			*PCB_Transfer_Ready_To_Timer;											// used to transfer PCB in start_timer
ProcessControlBlock			*PCB_Transfer_Ready_To_Suspend;											// used to transfer PCB to suspend
ProcessControlBlock			*PCB_Transfer_Timer_To_Suspend;											// used to transfer PCB to suspend
ProcessControlBlock			*PCB_Transfer_Suspend_To_Ready;											// used to transfer PCB in resume
ProcessControlBlock			*PCB_Change_Priority;													// used for change priority in ReadyQueue

ProcessControlBlock			*PCB_Transfer_Timer_To_MsgSuspend;										// used in suspend/resume becuase of send and receive messsage
ProcessControlBlock			*PCB_Transfer_Ready_To_MsgSuspend;										// used in suspend/resume becuase of send and receive messsage
ProcessControlBlock			*PCB_Transfer_MsgSuspend_To_Timer;										// used in suspend/resume becuase of send and receive messsage
ProcessControlBlock			*PCB_Transfer_MsgSuspend_To_Ready;										// used in suspend/resume becuase of send and receive messsage
ProcessControlBlock			*PCB_Transfer_MsgSuspend_To_Suspend;		


ProcessControlBlock			*PCB_Current_Running_Process = NULL;									// keep point to currrent running process, set when SwitchContext, Dispatcher
																									
ProcessControlBlock			*PCB_Terminating_Processs;												// keep current context then destroy in next context

ProcessControlBlock			*PCB_Table[MAX_PROCESSES];												// Move PCB Table to array.  //ProcessControlBlock			*ListHead;				// PCB table
																									// With List, I have to allocate seperate memeory for Table. With array, TimerQueue and ReadQueue can point to same as array

void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 process_id, INT32 *error);
INT32	check_legal_process(char* process_name, INT32 initial_priority);							// check Legal process before create process
INT32	check_legal_process_suspend(INT32 process_id);												// check legal process from ReadyQueue before suspend
void	start_timer(long * sleepTime);
void	terminate_proccess_id(INT32 process_id, INT32 *process_error_return);						// terminate specified a process with process_id
void	suspend_process_id(INT32  process_id, INT32 *process_error_return);							// suspend specified a process with process_id. Allow suspend self
void	change_process_priority(INT32 process_id, INT32 new_priority, INT32 *process_error_return);	// change priority of a process with process_id. Allow change self
void	resume_process_id(INT32  process_id, INT32 *process_error_return);							// resume specified a process with process_id
void	make_ready_to_run(ProcessControlBlock **ReadyQueueHead, ProcessControlBlock *pcb);			// Insert into readque
void	dispatcher();
void	please_charge_hardware_time() {};															// increase time function, used in infinite loop
void	process_printer(char* action, INT32 target_process_id,										// -1 means does not print out in state printer
						INT32 terminated_process_id, INT32 new_process_id, INT32 where_to_print);

void	message_suspend_process_id(INT32  process_id);												// suspend specified a process with process_id. Allow suspend self
void	message_resume_process_id(INT32  process_id);												// resume specified a process with process_id. Use to suspend message 

void	send_message(INT32 target_pid, char *message, INT32 send_length, INT32 *process_error_return);
void	receive_message(INT32 source_pid, char *message, INT32 receive_length, INT32 *send_length, INT32 *send_pid, INT32 *process_error_return);

void	LockTimer (INT32 *timer_lock_result);
void	UnLockTimer (INT32 *timer_lock_result);
void	LockReady (INT32 *ready_lock_result);
void	UnLockReady (INT32 *ready_lock_result);
void	LockPrinter (INT32 *printer_lock_result);
void	UnLockPrinter(INT32 *printer_lock_result);
void	LockSuspend (INT32 *suspend_list_result);
void	UnLockSuspend (INT32 *suspend_list_result);
void	LockPriority (INT32 *priority_lock_result);
void	UnLockPriority (INT32 *priority_lock_result);
void	LockMessageQueue (INT32 *message_lock_result);
void	UnLockMessageQueue (INT32 *message_lock_result);
void	LockSuspendListMessage (INT32 *message_suspend_list_result);
void	UnLockSuspendListMessage (INT32 *message_suspend_list_result);


/**************************************************************** hdduong> **********************************************************************************/



/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/
void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;
	INT32			   Time;
	INT32			   *new_update_time;										// for Relative time from new head after put into Ready queue
	ProcessControlBlock	*tmp = NULL;											// check front of time queue head

	
	// Get cause of interrupt
    CALL( MEM_READ(Z502InterruptDevice, &device_id ) );

    // Set this device as target of our query
    CALL ( MEM_WRITE(Z502InterruptDevice, &device_id ) );

	// Now read the status of this device
    CALL( MEM_READ(Z502InterruptStatus, &status ) );

	if ( device_id == TIMER_INTERRUPT && status == ERR_SUCCESS ) {
		

		MEM_READ(Z502ClockStatus, &Time);													// read current time and compare to front of Timer queue
		
		CALL(LockTimer(&timer_queue_result) );
		
		CALL(process_printer("Ready",-1,-1,-1,INTERRUPT_BEFORE) );

		tmp = TimerQueueHead;
		while ( (tmp!= NULL) && (tmp->wakeup_time <= Time) ) {
			//debug
			//printf (" ...ready ... process_id: %d, wake-up: %d, Time: %d \n", tmp->process_id, tmp->wakeup_time, Time);
			//debug

			PCB_Transfer_Timer_To_Ready = DeQueue(&TimerQueueHead);							// take off front Timer queue

			CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_Timer_To_Ready) );		// put in the end of Ready Queue

			tmp = TimerQueueHead;															// transfer until front Timer Queue > current time

		}
		
		CALL( process_printer("Ready",-1,-1,-1,INTERRUPT_AFTER) );
		//--------------------------------------------------------//
		//		  update start timer based on new head			  //	
		//--------------------------------------------------------//
		if (tmp != NULL) {
			new_update_time = (INT32*) malloc(sizeof(INT32));
			MEM_READ(Z502ClockStatus, &Time);			
			*new_update_time = tmp->wakeup_time - Time;							// there: tmp== NULL or tmp->wakeup_time > Time
			
			if (*new_update_time > 0) {										// if time is not pass
				MEM_WRITE(Z502TimerStart, new_update_time);						// set timer based on new timer queue head
				//debug
				//printf("... Ready... new_update_time: %d \n",*new_update_time);
				//debug
			}
			else {															// time already pass
				MEM_WRITE(Z502TimerStart, &generate_interrupt_immediately);
				//debug
				//printf("... Ready... Time Pass... IMMEDIATELY GEN: \n");
				//debug
			}
			free(new_update_time);
		}

		CALL( UnLockTimer(&timer_queue_result) );									// should have lock here otherwise something come in before update start_timer
	}
	
	// Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
	
}                                       /* End of interrupt_handler */
/************************************************************************
    FAULT_HANDLER
        The beginning of the OS502.  Used to receive hardware faults.
************************************************************************/

void    fault_handler( void )
    {
    INT32       device_id;
    INT32       status;
    INT32       Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );


	switch (device_id)
	{
	case PRIVILEGED_INSTRUCTION:
		printf("\nExiting test1k Because of PRIVILEGED_INSTRUCTION...\n\n");
		CALL(Z502Halt());
		break;
	case INVALID_PHYSICAL_MEMORY:
		break;
	case INVALID_MEMORY:
		break;
	case CPU_ERROR:
		break;
	default:
		break;
	}
    printf( "Fault_handler: Found vector type %d with value %d\n",
                        device_id, status );
    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of fault_handler */

/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
        The variable do_print is designed to print out the data for the
        incoming calls, but does so only for the first ten calls.  This
        allows the user to see what's happening, but doesn't overwhelm
        with the amount of data.
************************************************************************/
void    svc (SYSTEM_CALL_DATA *SystemCallData ) {
    short               call_type;					 /* those babies are from Prof's code */
    static INT16        do_print = 100;				 /*									  */
    INT32               Time;						 /*									  */
	short               i;							 /*			Leave as they are		  */	
	
	INT32				create_priority;			 // read argument 2 - prioirty of CREATE_PROCESS
	long				SleepTime;					 // using in SYSNUM_SLEEP
	INT32				terminate_arg;				 // terminate argument process_id of TERMINATE_PROCESS
	char				*process_name_arg = NULL;	 // process name argument of GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_error_arg;			 // process error check for GET_PROCESS_ID, CREATE_PROCESS, SUSPEND_PROCESS
	INT32				process_id_arg;				 // process_id returns in GET_PROCESS_ID, CREATE_PROCESS
	void				*starting_address;			 // starting address of rountine in CREATE_PROCESS
	INT32				suspend_arg;				 // suspend argument process_id of SUSPEND_PROCESS
	INT32				resume_arg;					 // resume argument process_id of RESUME_PROCESS
	INT32				priority_process_id_arg;	 //	change priority of a process_id in CHANGE_PRIORITY
	INT32				priority_new_arg;			 //	new priority in CHANGE_PRIORITY

	INT32				send_target_pid_arg;						// target process_id in SEND_MESSAGE
	char				msg_buffer_arg[LEGAL_MESSAGE_LENGTH];		// message buffer in SEND_MESSAGE, RECEIVE_MESSAGE
	INT32				message_send_length_arg;					// message send length of SEND_MESSAGE, RECEIVE_MESSAGE
		
	INT32				receive_source_pid_arg;						// RECEIVE_MESSAGE
	INT32				message_receive_length_arg;					// RECEIVE_MESSAGE
	INT32				message_sender_pid_arg;						// RECEIVE_MESSAGE



    call_type = (short)SystemCallData->SystemCallNumber;
    if ( do_print > 0 ) {
        // same code as before
		 printf( "SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++ ){
        	 printf( "Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
             (unsigned long )SystemCallData->Argument[i],
             (unsigned long )SystemCallData->Argument[i]);
        }
    do_print--;
    }

    switch (call_type) {
        
		// Get time service call
        case SYSNUM_GET_TIME_OF_DAY:																			// This value is found in syscalls.h
            MEM_READ( Z502ClockStatus, &Time );
			*(INT32 *)SystemCallData->Argument[0] = Time;
            break;
       
		case SYSNUM_SLEEP:
			SleepTime = (INT32) SystemCallData->Argument[0];
			start_timer(&SleepTime);
			break;

		// terminate system call
		case SYSNUM_TERMINATE_PROCESS:
			
			terminate_arg =  (INT32) SystemCallData->Argument[0];												//get process_id

			if (terminate_arg == -1) {																			//terminate self means terminate current running process
				CALL( terminate_proccess_id(PCB_Current_Running_Process->process_id,&process_error_arg) );		//this will not free PCB_Current_Running_Process. Free in the queue only
				
				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;

				PCB_Current_Running_Process = NULL;																//termination condition for test1e

				CALL( dispatcher() );																			//try to switch to next process after current process terminated	
				
			}

			else if (terminate_arg == -2) {																		//terminate self + children
				
				CALL( DeleteQueue(TimerQueueHead) );															// release all heap memory before quit
				CALL( DeleteQueue(ReadyQueueHead) );
				// ----Need to delete the array also -- //
				CALL( Z502Halt() );
			}
			else  {																								// terminate a process with process_id
				//SleepTime = 0;
				CALL( terminate_proccess_id(terminate_arg, &process_error_arg) );	
				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;
			}
            break;
        
		// create new process
		case SYSNUM_CREATE_PROCESS:
			process_name_arg = strdup((char *)SystemCallData->Argument[0]);								// copy data and malloc
			starting_address = (void*) SystemCallData->Argument[1];										// get address of rountine from argument
			create_priority = (INT32)SystemCallData->Argument[2];										// get priority of process through argument 	

			CALL(os_create_process(process_name_arg,starting_address, create_priority, &process_id_arg, &process_error_arg));  // create process and include checking
			
			*(INT32 *)SystemCallData->Argument[3] = process_id_arg;										// return created process id if success
			*(INT32 *)SystemCallData->Argument[4] = process_error_arg;									// error or success return here
			
			free(process_name_arg);																		// free after malloc
			break;
		
		//get process id
		case SYSNUM_GET_PROCESS_ID:
			process_name_arg = (char *)SystemCallData->Argument[0];
			if (strcmp(process_name_arg,"") == 0) {														// get self process
				*(INT32 *)SystemCallData->Argument[1] = PCB_Current_Running_Process->process_id;		//get data from Current_Process_PCB 
				*(INT32 *)SystemCallData->Argument[2] = PROCESS_ID_EXIST;
			}
			else 
			{
				process_id_arg = GetProcessID(PCB_Table, process_name_arg, number_of_processes_created);
				if (process_id_arg == PROCESS_ID_NOT_VALID) {											// no process exits
					*(INT32 *)SystemCallData->Argument[2] = PROCESS_ID_NOT_EXIST;
				}
				else {
					*(INT32 *)SystemCallData->Argument[1] = process_id_arg;
					*(INT32 *)SystemCallData->Argument[2] = PROCESS_ID_EXIST;
				}

			}
			break;
		
		// suspend process
		case SYSNUM_SUSPEND_PROCESS:
			suspend_arg =  (INT32) SystemCallData->Argument[0];												//get process_id	
			
			if (suspend_arg == -1) {																		// suspend self
				suspend_process_id(PCB_Current_Running_Process->process_id,&process_error_arg);

				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;
				
			}
			else {                                                                                          // suspend with process_id provided
				suspend_process_id(suspend_arg,&process_error_arg);
				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;
			}
			//if (process_error_arg == PROCESS_SUSPEND_LEGAL)												// only allows another process to run when suspend self
			//	CALL(dispatcher() );																		// which done in suspend_process_id
			
			break;
		
		// resume process
		case SYSNUM_RESUME_PROCESS:
			resume_arg =  (INT32) SystemCallData->Argument[0];												//get process_id	
			
			resume_process_id(resume_arg, &process_error_arg);
		    *(INT32 *)SystemCallData->Argument[1] = process_error_arg;                                      // resume with process_id provided

			break;
		// change priority
		case SYSNUM_CHANGE_PRIORITY:																		//The result of a change priority takes effect immediately
			priority_process_id_arg = (INT32) SystemCallData->Argument[0];
			priority_new_arg = (INT32) SystemCallData->Argument[1];
			if (priority_process_id_arg != -1)
				change_process_priority(priority_process_id_arg,priority_new_arg, &process_error_arg);
			else 
				change_process_priority(PCB_Current_Running_Process->process_id,priority_new_arg, &process_error_arg);

			 *(INT32 *)SystemCallData->Argument[2] = process_error_arg;

			break;
		// change priority
		case SYSNUM_SEND_MESSAGE:	
			send_target_pid_arg = (INT32) SystemCallData->Argument[0];
			
			strcpy(msg_buffer_arg, (char*) SystemCallData->Argument[1]);

			message_send_length_arg = (INT32) SystemCallData->Argument[2];

			send_message(send_target_pid_arg,msg_buffer_arg,message_send_length_arg,&process_error_arg);
			*(INT32 *)SystemCallData->Argument[3] = process_error_arg;
			
			break;

		case SYSNUM_RECEIVE_MESSAGE:
			receive_source_pid_arg = (INT32) SystemCallData->Argument[0];
			
			strcpy(msg_buffer_arg, (char*) SystemCallData->Argument[1]);

			message_receive_length_arg = (INT32) SystemCallData->Argument[2];

			receive_message(receive_source_pid_arg,msg_buffer_arg,message_receive_length_arg,
				&message_receive_length_arg,&message_sender_pid_arg, &process_error_arg);
			strcpy((char*) SystemCallData->Argument[1], msg_buffer_arg);
			*(INT32 *)SystemCallData->Argument[3] = message_receive_length_arg;
			*(INT32 *)SystemCallData->Argument[4] = message_sender_pid_arg;
			*(INT32 *)SystemCallData->Argument[5] = process_error_arg;

		break;
		
		default:  
            printf( "ERROR!  call_type not recognized!\n" ); 
            printf( "Call_type is - %i\n", call_type);
    }                                           // End of switch
}                                               // End of svc 




/************************************************************************
    osInit
        This is the first routine called after the simulation begins.  This
        is equivalent to boot code.  All the initial OS components can be
        defined and initialized here.
************************************************************************/

void    osInit( int argc, char *argv[]  ) {
    void                *next_context;
    INT32               i;
	INT32				created_process_error;
	INT32				created_process_id;
	INT32				test_case_prioirty = 10;
	

    /* Demonstrates how calling arguments are passed thru to here       */

    printf( "Program called with %d arguments:", argc );
    for ( i = 0; i < argc; i++ )
        printf( " %s", argv[i] );
    printf( "\n" );
    printf( "Calling with argument 'sample' executes the sample program.\n" );

    /*          Setup so handlers will come to code in base.c           */

    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;

    /*  Determine if the switch was set, and if so go to demo routine.  */

    if (( argc > 1 ) && ( strcmp( argv[1], "sample" ) == 0 ) ) {
        Z502MakeContext( &next_context, (void *)sample_code, KERNEL_MODE );
        Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );
    }                   /* This routine should never return!!           */

    /*  This should be done by a "os_make_process" routine, so that
        test0 runs on a process recognized by the operating system.  

    Z502MakeContext( &next_context, (void *)test0, USER_MODE );
    Z502SwitchContext( SWITCH_CONTEXT_KILL_MODE, &next_context );		*/
	

	//----------------------------------------------------------//
	//		Choose to run test from command line				//
	//----------------------------------------------------------//
	if (( argc > 1 ) && ( strcmp( argv[1], "test0" ) == 0 ) ) {
		CALL( os_create_process("test0",(void*) test1a,test_case_prioirty, &created_process_id, &created_process_error) );
    } 
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1a" ) == 0 ) ) {
		CALL( os_create_process("test1a",(void*) test1a,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1b" ) == 0 ) ) {
		CALL( os_create_process("test1b",(void*) test1b,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1c" ) == 0 ) ) {
		CALL( os_create_process("test1c",(void*) test1c,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1d" ) == 0 ) ) {
		CALL( os_create_process("test1d",(void*) test1d,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1e" ) == 0 ) ) {
		CALL( os_create_process("test1e",(void*) test1e,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1f" ) == 0 ) ) {
		CALL( os_create_process("test1f",(void*) test1f,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1g" ) == 0 ) ) {
		CALL( os_create_process("test1g",(void*) test1g,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1h" ) == 0 ) ) {
		CALL( os_create_process("test1h",(void*) test1h,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1i" ) == 0 ) ) {
		CALL( os_create_process("test1i",(void*) test1i,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1j" ) == 0 ) ) {
		CALL( os_create_process("test1j",(void*) test1j,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1k" ) == 0 ) ) {
		CALL( os_create_process("test1k",(void*) test1k,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	else if (( argc > 1 ) && ( strcmp( argv[1], "test1l" ) == 0 ) ) {
		CALL( os_create_process("test1l",(void*) test1l,test_case_prioirty, &created_process_id, &created_process_error) );
	}
	//----------------------------------------------------------//
	//				Run test manuallly			    			//
	//----------------------------------------------------------//

	//CALL ( os_create_process("test0",(void*) test0,test_case_prioirty, &created_process_id, &created_process_error) );
	
	//CALL ( os_create_process("test1a",(void*) test1a,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1b",(void*) test1b,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1c",(void*) test1c,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1d",(void*) test1d,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1e",(void*) test1e,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1f",(void*) test1f,test_case_prioirty, &created_process_id, &created_process_error) );
	
	//CALL ( os_create_process("test1g",(void*) test1g,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1h",(void*) test1h,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1i",(void*) test1i,test_case_prioirty, &created_process_id, &created_process_error) );

	CALL ( os_create_process("test1j",(void*) test1j,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1k",(void*) test1k,test_case_prioirty, &created_process_id, &created_process_error) );

	//CALL ( os_create_process("test1l",(void*) test1l,test_case_prioirty, &created_process_id, &created_process_error) );
}                                               // End of osInit


/*******************************************************************************************************************************
os_create_process

This function invoked by CREATE_PROCESS system call. 
First of all, this function check process going to be created is a legalprocess or not. 
If a legal process then new Process Control Block (PCB) created. Then it makes context for new process. Finally put the PCB to 
ReadyQueue by invoking make_ready_to_run. 
If not a legal process then return error through error variable. 
********************************************************************************************************************************/


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id, INT32 *error)
{
	 INT32						legal_process;
	 ProcessControlBlock		*newPCB;
	 
	
	 CALL( process_printer("Create",-1,-1,-1,CREATE_BEFORE) );
	 
	 if ( (legal_process = check_legal_process(process_name,priority)) != PROCESS_LEGAL) {    // if not legal write error then return
		 *error = legal_process;
		 return;
	 }

	 global_process_id++;																	 // if legal processs
	 newPCB = CreateProcessControlBlockWithData( (char*) process_name,						 // create new pcb
												 (void*) starting_address,
												 priority,
												 global_process_id);
	
	if (newPCB == NULL) {																	// run out of resources
		*error = PROCESS_CREATED_NOT_ENOUGH_MEMORY;
		return;
	}
	PCB_Table[number_of_processes_created] = newPCB;										// new process  created, put immediately in PCB_Table

	number_of_processes_created++;
	
	newPCB->wakeup_time = 0;															  // add more field for newPCB if successfully created
	newPCB->state = PROCESS_STATE_CREATE;

	*process_id = global_process_id;														// prepare for return values
	*error = PROCESS_CREATED_SUCCESS;

	CALL( Z502MakeContext( &newPCB->context, (void*) starting_address, USER_MODE ) );	

	CALL( process_printer("Create",-1,-1,newPCB->process_id,CREATE_AFTER) );

	CALL(make_ready_to_run(&ReadyQueueHead,newPCB) );												// whenever new process created, put into ready queue

}




/*******************************************************************************************************************************
check_legal_process

This function uses to validate a process is legal or not. 
A legal process when all following conditions are valid:
	* Priority 
	* Process Name
	* Number of processes created.
********************************************************************************************************************************/

INT32	check_legal_process(char* process_name, INT32 initial_priority) 
{
	if(	initial_priority <= 0	) {															// ILLEGAL Priority
		return PROCESS_CREATED_ILLEGAL_PRIORITY;
	}
	else if ( IsExistsProcessNameArray(PCB_Table, process_name,number_of_processes_created) ) {						//already process_name
		return PROCESS_CREATED_ERROR_SAME_NAME;
	}
	else if ( number_of_processes_created >= MAX_PROCESSES) {
		return PROCESS_CREATED_MAX_EXCEED;
	}
	return PROCESS_LEGAL;
}




/*******************************************************************************************************************************
start_timer

This function invoked by system call SLEEP. First of all, this function will caculate Absolute Time of current running process.
The input of SetTimer = (Absolute Time - Current Time) is the time expected in the furture a interrupt occours.
Then this function will push current running process into TimerQueue. The insertion might cause change in order of PCB in 
TimerQueue. There are some cases needed to considered before SetTimer:
(I) If new PCB is inserted in front of TimerQueue then update Timer. This need to be check with two cases:
	(1) If current time does not pass the Absolute Time of new PCB, then SetTimer = newHeaderTime - oldHeaderTime;
	(2) If current time is already passed, then SetTimer = 1 to generate interrupt immediately. 
(II) If new PCB is inserted in the middle or the end of the TimerQueue so do not need to update SetTimer.
Finally, after all above processing finishes, dispatcher is invoked to let another process in ReadyQueu to run.
Notice: In PCB, wakeup_time is the absolute Time expects interrupt occurs in furture.
********************************************************************************************************************************/

void start_timer(long *sleepTime) 
{
	INT32		Time;														// get current time assigned to 
	INT32		*new_update_time = NULL;									// for Relative time from new head after put into Ready queue
	BOOL		loop_wait_ReadyQueue_filled = TRUE;
	ProcessControlBlock *prev_Head;											// to get timer of HeadTimerQueue. Needed for updatding Timer when changes in header

	MEM_READ (Z502ClockStatus, &Time);								        // get current time
	
	//----------------------------------------------------------------------//
	//	PCB_Current_Running_Process <-- ReadyQueue Head from dispatcher     //
	//----------------------------------------------------------------------//
	PCB_Current_Running_Process->wakeup_time = Time + (INT32) (*sleepTime);						// Set absolute time to PCB
	PCB_Current_Running_Process->state	= PROCESS_STATE_SLEEP;									// Current PCB still points to ReadyQueue Head

	PCB_Transfer_Ready_To_Timer = PCB_Current_Running_Process;													
	
	CALL( LockTimer(&timer_queue_result) );
	
	CALL( process_printer("Waiting",-1,-1,-1,START_TIMER_BEFORE) );
	prev_Head = TimerQueueHead;
	
	CALL( AddToTimerQueue(&TimerQueueHead, PCB_Transfer_Ready_To_Timer) );						// then Add to Timer queue
	
	CALL( process_printer("Waiting",-1,-1,-1,START_TIMER_AFTER) );
	
	if (prev_Head == NULL) {                                            // no process in timerQueue for main test
			
		// debug
		// printf("... Waiting... NULL TimerQueue... Process sleep: %d \n",PCB_Current_Running_Process->process_id);
		// printf("... Waiting... NULL TimerQueue... new timer: %d, current time: %d , wake up time %d \n",*new_update_time,Time,PCB_Current_Running_Process->wakeup_time);
		// debug
			
		MEM_WRITE( Z502TimerStart, sleepTime );							// go ahead and set timer
	}
	else if ( (prev_Head->wakeup_time > TimerQueueHead->wakeup_time) ) {
			
		new_update_time = (INT32*) malloc(sizeof(INT32));
		MEM_READ (Z502ClockStatus, &Time);
		*new_update_time = TimerQueueHead->wakeup_time - Time;	

		if (*new_update_time > 0) {										// if time is not pass
			MEM_WRITE(Z502TimerStart, new_update_time);						// set timer based on new timer queue head
		}
		else {															// time already pass
			MEM_WRITE(Z502TimerStart, &generate_interrupt_immediately);
		}
		free(new_update_time);
	}
	
	CALL( UnLockTimer(&timer_queue_result) );	
	CALL( dispatcher() );

}



/***************************************************************************************************
make_ready_to_run

This function decides a PCB is added into ReadyQueue ordered by Priority or Not. 
And this is the place lets main test processes to run immediately by calling dispatcher. 
Other children processes are added to ReadyQueue only.
****************************************************************************************************/

void make_ready_to_run(ProcessControlBlock **head_of_ready, ProcessControlBlock *pcb) 
{
	
	CALL( LockReady(&ready_queue_result) );
	if (!use_priority_queue) {
		CALL( AddToReadyQueueNotPriority(&ReadyQueueHead,pcb) );
	} 
	else {
		CALL( AddToReadyQueue(&ReadyQueueHead,pcb) );
	}
	CALL( UnLockReady(&ready_queue_result) );

	if (already_run_main_test == FALSE) {
		already_run_main_test= TRUE;
		
		if ( strcmp(pcb->process_name, "test0") == 0 ) {
			use_priority_queue = FALSE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1a") == 0 ) {
			use_priority_queue = FALSE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1b") == 0 ) {
			use_priority_queue = FALSE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1c") == 0 ) {
			use_priority_queue = FALSE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1d") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1e") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1f") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1g") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1h") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1i") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1j") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1k") == 0 ) {
			use_priority_queue = FALSE;
			CALL( dispatcher() );
		}
		else if ( strcmp(pcb->process_name, "test1l") == 0 ) {
			use_priority_queue = TRUE;
			CALL( dispatcher() );
		}
	}

}



/**********************************************************************************************************************
terminate_proccess_id

This function is invoked by TERMINATE_PROCESS system call. 
First it checks there is a legal process passed into the function.
Then it checks each TimerQueue or ReadyQueue. If the process needed to be terminated is in 
(I) ReadyQueue: then just take out from the ReadyQueue.
(II) TimerQueue: take out from the TimerQueue with comparision between time at front of TimerQueue and current time. 
There are two cases to be considered:
	(1) If current time does not pass wake up time of PCB at front of TimerQueue then update SetTimer
		by different between those time. This update happens even head PCB of TimerQueue is changed or not.
		If change order happens at header TimerQueue then update SetTimer is needed. But even header does not change, 
		I still update time in SetTimer because interrupt time is still correct but I can reduce the code. 
	(2) If current time already passed, then generate interrupt immediately.
Finally, set Terminate state in PCB_Table so that this process never be scheduled again.
Notice: I think I should check with SuspendList but this function works with all the tests so I deciede not to code that.
*************************************************************************************************************************/

void terminate_proccess_id(INT32 process_id, INT32* error_return) 
{
	INT32	Time;
	INT32	*new_start_timer;

	
	CALL( process_printer("Kill",-1,process_id,-1,TERMINATE_BEFORE) );
	

	if ( !IsExistsProcessIDArray(PCB_Table,  process_id, number_of_processes_created) ) {							//invalid process_id
		*error_return = PROCESS_TERMINATE_INVALID_PROCESS_ID;
		return;
	}
	else {																					

		if (IsExistsProcessIDQueue(ReadyQueueHead,process_id) ) {
			CALL(LockReady(&ready_queue_result) );
			CALL(RemoveProcessFromQueue(&ReadyQueueHead,process_id) );
			CALL(UnLockReady(&ready_queue_result) );
		}
		if (IsExistsProcessIDQueue(TimerQueueHead,process_id) ) {
			CALL(LockTimer(&timer_queue_result) );
			CALL( RemoveProcessFromQueue(&TimerQueueHead,process_id) );						//remove from timer queue
			

			MEM_READ (Z502ClockStatus, &Time);	
			new_start_timer = (INT32*) malloc(sizeof(INT32));
			*new_start_timer = TimerQueueHead->wakeup_time - Time;
			if (*new_start_timer > 0) {													   // remove from TimerQueue need update timer
				MEM_WRITE( Z502TimerStart, new_start_timer );
			}
			else {																		   // while removing old head maybe time already pass	
				MEM_WRITE( Z502TimerStart, &generate_interrupt_immediately );			   // generate new interrupt	
			}
			free(new_start_timer);
			
			CALL(UnLockTimer (&timer_queue_result) );
		}

		CALL( process_printer("Kill",-1,process_id,-1,TERMINATE_AFTER) );
											
		CALL( RemoveFromArray(PCB_Table,process_id,number_of_processes_created) );			// remove from PCB table
		
		*error_return = ERR_SUCCESS;														//success

		return;
		
	}
}



/********************************************************************************************************************
dispatcher

The main function is let a process from front of ReadyQueue to run by calling SwitchContext. 
Howerver, before leting a process to run, it need to check if any PCB is queued in ReadyQueue. There are some cases:
(1) It waits for a process go into ReadyQueue if there is some processes in TimerQueue.
(2)	It stops simulation if current running process is NULL and there is nothing in TimerQueue and ReadQueue. 
	This means all processes are all in Suspend states or Killed. 
	There is no processes to run in furture so stop simulation.
**********************************************************************************************************************/

void dispatcher() {
	
	INT32			   Time;
	INT32			   *new_update_time;										// for Relative time from new head after put into Ready queue
	ProcessControlBlock	*tmp = NULL;											

	while (IsQueueEmpty(ReadyQueueHead)) {
		//debug
		//MEM_READ(Z502ClockStatus, &Time);	
		//debug

		//-------------------------------------------------//
		//	test1e: all in suspend list. Nothing wake up   //
		//-------------------------------------------------//
		if ( (IsQueueEmpty(TimerQueueHead)) &&
			(! IsListEmpty (SuspendListHead)) && 
			(PCB_Current_Running_Process == NULL) ) {
				CALL( printf("Test1e: Self-Suspend Allowed. I Myself test1e Suspends Myself. No One wakes me up then the Program Halts... \n") );
				CALL(Z502Halt());
		}
		//-----------------------------------------------//
		//			bug fix: test0, test1a				 //
		//-----------------------------------------------//
		if ( (IsQueueEmpty(TimerQueueHead) ) &&
			(IsQueueEmpty(ReadyQueueHead) ) &&
			(PCB_Current_Running_Process == NULL) ) {
				CALL(Z502Halt());
		}
		CALL();
	}

	CALL( LockReady(&ready_queue_result) );										// do not put Lock before while because Interrupt handler might wants to lock while loop 
																				// then wait forever				
	 
	CALL ( process_printer("Run",-1,-1,-1,DISPATCH_BEFORE)  );
	
	
	PCB_Current_Running_Process = DeQueue(&ReadyQueueHead);												// take out from ReadyQueue
	PCB_Current_Running_Process->state = PROCESS_STATE_RUNNING;											// make running process

	CALL ( process_printer("Run",-1,-1,-1,DISPATCH_AFTER) );
	
	CALL( UnLockReady(&ready_queue_result) );
	
	Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &PCB_Current_Running_Process->context );		//switch context always by Current_Running_Process

}



/********************************************************************************************************************
process_printer

Print all the states of processes using State_Printer. 
Input parameter = -1 means does not print out in state printer.
**********************************************************************************************************************/
void	process_printer(char* action, INT32 target_process_id,				
						INT32 terminated_process_id, INT32 new_process_id, INT32 where_to_print) 
{
	INT32					Time;
	ProcessControlBlock		*readyQueue;			// for ready mode
	ProcessControlBlock		*timerQueue;			// for waiting mode
	ProcessControlBlock		*suspendList;			// for suspend mode

	CALL( LockPrinter(&printer_lock_result) );
	MEM_READ(Z502ClockStatus, &Time);									
	CALL(SP_setup( SP_TIME_MODE, Time) );

	CALL(SP_setup_action(SP_ACTION_MODE, action) );					//CREATE RUN READY WAITING
	
	// ------------------
	// Print one value
	// ------------------
	if (target_process_id != -1)
		CALL(SP_setup( SP_TARGET_MODE, target_process_id) );

	if ( (PCB_Current_Running_Process!= NULL) && 
		(!IsKilledProcess(PCB_Table, PCB_Current_Running_Process->process_id, number_of_processes_created) )) // check whether currrent process killed then do not print killed one as run
		CALL(SP_setup( SP_RUNNING_MODE, PCB_Current_Running_Process->process_id) );

	if (terminated_process_id != -1)
		CALL(SP_setup( SP_TERMINATED_MODE, terminated_process_id) );

	if (new_process_id != -1)
		CALL(SP_setup( SP_NEW_MODE, new_process_id) );

	// --------------------
	// Print queue, table
	// --------------------
	
	if (!IsQueueEmpty(TimerQueueHead)) {							// print only queue is not blank
		
		timerQueue = TimerQueueHead;
		
		while (timerQueue != NULL) {
			CALL( SP_setup( SP_WAITING_MODE,timerQueue->process_id) ) ;
			timerQueue = timerQueue->nextPCB;
		}
	}
	
	if (!IsQueueEmpty(ReadyQueueHead)) {							// print only queue is not blank

		readyQueue = ReadyQueueHead;
		
		while (readyQueue != NULL) {
			CALL( SP_setup( SP_READY_MODE,readyQueue->process_id) );
			readyQueue = readyQueue->nextPCB;
		}
	}
	
	
	if (!IsListEmpty(SuspendListHead)) {							// print only queue is not blank

		suspendList = SuspendListHead;

		while (suspendList != NULL) {
			CALL( SP_setup( SP_SUSPENDED_MODE,suspendList->process_id) );
			suspendList = suspendList->nextPCB;
		}
	}
	
	switch (where_to_print)
	{
	case CREATE_BEFORE:
		CALL( printf("\n--------------------------Before Creating Process------------------------\n") );
		break;

	case CREATE_AFTER:
		CALL( printf("\n--------------------------After Creating Process-------------------------\n") );
		break;

	case START_TIMER_AFTER:
		CALL( printf("\n--------------------------After Start Timer------------------------------\n") );
		break;

	case START_TIMER_BEFORE:
		CALL( printf("\n--------------------------Before Start Timer-----------------------------\n") );
		break;

	case DISPATCH_BEFORE:
		CALL( printf("\n--------------------------Before Dispatch--------------------------------\n") );
		break;

	case DISPATCH_AFTER:
		CALL( printf("\n--------------------------After Dispatch---------------------------------\n") );
		break;

	case INTERRUPT_AFTER:
		CALL( printf("\n--------------------------After Interrupt---------------------------------\n") );
		break;

	case INTERRUPT_BEFORE:
		CALL( printf("\n--------------------------Before Interrupt--------------------------------\n") );
		break;

	case TERMINATE_BEFORE:
		CALL( printf("\n--------------------------Before Terminate--------------------------------\n") );
		break;

	case TERMINATE_AFTER:
		CALL( printf("\n--------------------------After Terminate---------------------------------\n") );
		break;

	case RESUME_BEFORE:
		CALL( printf("\n--------------------------Before Resume-----------------------------------\n") );
		break;

	case RESUME_AFTER:
		CALL( printf("\n--------------------------After Resume------------------------------------\n") );
		break;

	case SUSPEND_BEFORE:
		CALL( printf("\n--------------------------Before Suspend----------------------------------\n") );
		break;

	case SUSPEND_AFTER:
		CALL( printf("\n--------------------------After Suspend-----------------------------------\n") );
		break;

	case PRIORITY_BEFORE:
		CALL( printf("\n--------------------------Before Change Priority--------------------------\n") );
		break;

	case PRIORITY_AFTER:
		CALL( printf("\n--------------------------After Change Priority---------------------------\n") );
		break;


	default:
		break;
	}

    CALL( SP_print_header() );
    CALL( SP_print_line() );
    CALL( printf("--------------------------------------------------------------------------\n\n") );

	CALL( UnLockPrinter(&printer_lock_result) );
}




void LockTimer (INT32 *timer_lock_result){	
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_TIMER_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, timer_lock_result );
}

void UnLockTimer (INT32 *timer_lock_result){
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_TIMER_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, timer_lock_result );
}

void LockReady (INT32 *ready_lock_result){	
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_READY_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, ready_lock_result );
}

void UnLockReady (INT32 *ready_lock_result){
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_READY_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, ready_lock_result );
}

void LockSuspend (INT32 *suspend_lock_result){	
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_READY_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, suspend_lock_result );
}

void UnLockSuspend (INT32 *suspend_lock_result){
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_READY_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, suspend_lock_result );
}

void LockPrinter (INT32 *printer_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRINTER_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, printer_lock_result );
}

void UnLockPrinter(INT32 *printer_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRINTER_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, printer_lock_result );
}

void LockPriority (INT32 *priority_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRIORITY_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, priority_lock_result );
}

void UnLockPriority(INT32 *priority_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRIORITY_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, priority_lock_result );
}

void	LockSuspendListMessage (INT32 *message_suspend_list_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + MSG_SUSPEND_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, message_suspend_list_result );
}
void	UnLockSuspendListMessage (INT32 *message_suspend_list_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + MSG_SUSPEND_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, message_suspend_list_result );
}



/*******************************************************************************************************************************
check_legal_process_suspend

This function uses to validate a process is legal for suspending or not. 
A legal process when all following conditions are valid:
	* Process ID
	* Suspend already suspend process
********************************************************************************************************************************/

INT32	check_legal_process_suspend(INT32 process_id) 
{
	if (!IsExistsProcessIDArray(PCB_Table, process_id, number_of_processes_created) )					// if this suspend process_id is not even in table
		return PROCESS_SUSPEND_INVALID_PROCESS_ID;

	if ( IsExistsProcessIDList(SuspendListHead, process_id) ) {
		return PROCESS_SUSPEND_INVALID_SUSPENDED;
	}
	
	if ( ( !IsExistsProcessIDQueue(ReadyQueueHead, process_id) ) &&										// if in table but cannot find process in ReadyQueue to Suspend
		 ( !IsExistsProcessIDQueue(TimerQueueHead, process_id) ) && 									// updated Oct 5 cannot find in TimerQueue also
		 ( !IsExistsProcessIDQueue(MessageSuspendListHead, process_id) ) ) {							// updated Oct 10 cannot find in MessageSuspendListHead also
		if (PCB_Current_Running_Process == NULL)																	
			return PROCESS_SUSPEND_INVALID_PROCESS_ID;
		else {																							// not in ReadyQueue but maybe is a Current running PCB
			if (PCB_Current_Running_Process->process_id != process_id) 
				return PROCESS_SUSPEND_INVALID_PROCESS_ID;												// should mantain Current PCB point to ReadyQueue
			else																						// suspend self
				return PROCESS_SUSPEND_LEGAL;															// suspend current running
		}
	}
	else if ( ( !IsExistsProcessIDQueue(ReadyQueueHead, process_id) ) ||								// if exists in either ReadyQueue or TimerQueue then fine
		 ( !IsExistsProcessIDQueue(TimerQueueHead, process_id) ) || 
		 ( !IsExistsProcessIDQueue(MessageSuspendListHead, process_id) ) ) {									
		return PROCESS_SUSPEND_LEGAL;
	}

	return PROCESS_SUSPEND_INVALID_PROCESS_ID;

}




/*************************************************************************************************************************************
suspend_process_id

This function invoked by SUSPEND_PROCESS system call. It allows suspend self. 
If a process to be suspended is current running process then just push it into Suspend List then let other processes in ReadyQueue to run
If a process to be suspended is in TimerQueue,ReadyQueue or Suspend List:
	First, Pull out this process out of Queue or List.
	If pull out process from TimerQueue just need to update appropirate SetTimer.
	Then call dispatcher to let other processes in ReadyQueue to run.
*************************************************************************************************************************************/

void suspend_process_id(INT32  process_id, INT32 *process_error_return)
{
	INT32						legal_suspend_process; 
	BOOL						in_ready_queue;
	BOOL						in_timer_queue;
	INT32						*new_update_time;
	INT32						Time;
	ProcessControlBlock			*prev_timer_head;
	BOOL						in_msg_suspend_queue;
																			// if from send_receive then always ok		
	if ( (legal_suspend_process = check_legal_process_suspend(process_id)) != PROCESS_SUSPEND_LEGAL) {	// not a legal process  
		*process_error_return  =  legal_suspend_process;
		return;
	}
	

	CALL(process_printer("Suspend",process_id,-1,-1,SUSPEND_BEFORE) );

	if (PCB_Current_Running_Process->process_id == process_id) {                                       // suspend self
		CALL(LockSuspend(&suspend_list_result));
		PCB_Current_Running_Process->state = PROCESS_STATE_SUSPEND;
		AddToSuspendList(&SuspendListHead, PCB_Current_Running_Process);							  // there is nothing left to run	
		CALL(UnLockSuspend(&suspend_list_result));

		CALL(process_printer("Suspend",process_id,-1,-1,SUSPEND_AFTER) );
		
		CALL( dispatcher());																		  // let another process run				
		
	}
	else {																							   // suspend another process
		CALL( LockTimer(&timer_queue_result) );
		in_timer_queue = IsExistsProcessIDQueue(TimerQueueHead,process_id);
		if (in_timer_queue) {
			prev_timer_head = TimerQueueHead;														// to record Timer interrupt

			PCB_Transfer_Timer_To_Suspend = PullProcessFromQueue(&TimerQueueHead,process_id); 
			PCB_Transfer_Timer_To_Suspend->state = PROCESS_STATE_SUSPEND;

			if (prev_timer_head != TimerQueueHead) {												// update TimerQueue when chaning in header
				if (TimerQueueHead != NULL) {														// just take out the header	
					new_update_time = (INT32*) malloc(sizeof(INT32));
					MEM_READ(Z502ClockStatus, &Time);			
					*new_update_time = TimerQueueHead->wakeup_time - Time;	
					if (*new_update_time > 0) {														// if time is not pass
						MEM_WRITE(Z502TimerStart, new_update_time);									// set timer based on new timer queue head
						//debug
						//printf("... Suspend... new_update_time: %d \n",*new_update_time);
						//debug
					}
					else {																			// time already pass
						MEM_WRITE(Z502TimerStart, &generate_interrupt_immediately);
						//debug
						//printf("... Suspend... Time Pass... IMMEDIATELY GEN: \n");
						//debug
					}
	
					free(new_update_time);
				}
				else {}																				// NULL then pull the only one. Leave Timer as is it because interrupt will not process anything in Interrupt_handler
			}
		}
		CALL( UnLockTimer(&timer_queue_result) );

		CALL(LockReady(&ready_queue_result));
		in_ready_queue = IsExistsProcessIDQueue(ReadyQueueHead,process_id);
		if ( in_ready_queue ) {	
			PCB_Transfer_Ready_To_Suspend = PullProcessFromQueue(&ReadyQueueHead,process_id); 
			PCB_Transfer_Ready_To_Suspend->state = PROCESS_STATE_SUSPEND;

			CALL(LockSuspend(&suspend_list_result));
			AddToSuspendList(&SuspendListHead, PCB_Transfer_Ready_To_Suspend);
			CALL(UnLockSuspend(&suspend_list_result))
		}
		CALL(UnLockReady(&ready_queue_result));

		//----------------------------------------------------------------------//
		//					also check msg suspend list							//
		//----------------------------------------------------------------------//
		
		CALL(LockSuspendListMessage(&message_suspend_list_result));
		in_msg_suspend_queue = IsExistsProcessIDList(MessageSuspendListHead,process_id);
		if ( in_msg_suspend_queue ) {	
			PCB_Transfer_MsgSuspend_To_Suspend = PullFromSuspendList(&MessageSuspendListHead,process_id); 
			PCB_Transfer_MsgSuspend_To_Suspend->state = PROCESS_STATE_SUSPEND;

			CALL(LockSuspend(&suspend_list_result));
			AddToSuspendList(&SuspendListHead, PCB_Transfer_MsgSuspend_To_Suspend);
			CALL(UnLockSuspend(&suspend_list_result))
		}
		CALL(LockSuspendListMessage(&message_suspend_list_result));

		CALL(process_printer("Suspend",process_id,-1,-1,SUSPEND_AFTER) );
		
	}

	*process_error_return = PROCESS_SUSPEND_LEGAL;

}



/*******************************************************************************************************************************
check_legal_process_resume

This function uses to validate a process is legal for resuming or not. 
A legal process when all following conditions are valid:
	* Process ID
	* Resume a process already suspend.
	* Not resume an already resumed process.
********************************************************************************************************************************/

INT32	check_legal_process_resume(INT32 process_id) 
{
	if (!IsExistsProcessIDArray(PCB_Table, process_id, number_of_processes_created) )					// if this suspend process_id is not even in table
		return PROCESS_RESUME_INVALID_PROCESS_ID;

	if ( IsExistsProcessIDList(SuspendListHead, process_id) ) {											// in suspendList then valid resume
		return PROCESS_RESUME_LEGAL;
	}
	
	if ( IsExistsProcessIDQueue(ReadyQueueHead, process_id) ){
		return PROCESS_RESUME_INVALID_RESUMED;															// invalid resume when in ReadyQueue because already Resume
	}
	
	if ( IsExistsProcessIDQueue(TimerQueueHead, process_id) ){ 
		return PROCESS_RESUME_INVALID_RESUMED;
	}

	if (process_id = PCB_Current_Running_Process->process_id) {
		return PROCESS_RESUME_INVALID_RESUMED;
	}

	return PROCESS_RESUME_LEGAL;
}


/*************************************************************************************************************************************
resume_process_id

This function invoked by RESUME_PROCESS system call.
The process to be resumed taken out from Suspend List and put in ReadyQueue by calling make_ready_to_run.
*************************************************************************************************************************************/

void resume_process_id(INT32  process_id, INT32 *process_error_return)
{
	INT32					legal_resume_process; 
	ProcessControlBlock		*tmp;	

	if ( (legal_resume_process = check_legal_process_resume(process_id)) != PROCESS_RESUME_LEGAL) {		// not a legal process  
		*process_error_return  =  legal_resume_process;
		return;
	}

	
	tmp = ReadyQueueHead;
	CALL(LockSuspend(&suspend_list_result));
	
	PCB_Transfer_Suspend_To_Ready=  PullFromSuspendList(&SuspendListHead,process_id) ;	
	PCB_Transfer_Suspend_To_Ready->state = PROCESS_STATE_READY;
	
	CALL(UnLockSuspend(&suspend_list_result));
	
	CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_Suspend_To_Ready) );
	
	*process_error_return =  PROCESS_RESUME_LEGAL;
}




/*******************************************************************************************************************************
check_legal_change_process_priority

This function uses to validate a process is legal for changing priority or not. 
Need to be consider:
	* Valid process_id.
	* Priority in rage.
********************************************************************************************************************************/

INT32	check_legal_change_process_priority(INT32 process_id, INT32 new_priority)  {
	
	if (!IsExistsProcessIDArray(PCB_Table, process_id, number_of_processes_created) )					// if this priority process_id is not even in table
		return PRIORITY_ILLEGAL_PROCESS_ID;

	if (new_priority > MAX_PRIORITY_ALLOWED) {														    // if this process exists but priority is illegal				
		return PRIORITY_ILLEGAL_PRIORITY;
	}

	return PRIORITY_LEGAL;
}



/*******************************************************************************************************************************
change_process_priority

This function invoked by CHANGE_PRIORITY system call. It allows a process to  change its own priority.
If current running process or a process in TimerQueue wants to change its priority just update itself.
If a process from ReadyQueue then needs to pull it out from ReadyQueue, update its Priority and push back to ReadyQueue.
********************************************************************************************************************************/

void	change_process_priority(INT32 process_id, INT32 new_priority, INT32 *process_error_return) {
	
	INT32					legal_priority_process; 
	ProcessControlBlock		*tmp;	 
	BOOL					in_timer_queue;
	BOOL					in_ready_queue;

	if ( (legal_priority_process = check_legal_change_process_priority(process_id, new_priority) ) != PRIORITY_LEGAL)  {		// not a legal process  
		*process_error_return  =  legal_priority_process;
		return;
	}

	CALL(process_printer("Priority", process_id,-1,-1,PRIORITY_BEFORE) );

	if (PCB_Current_Running_Process->process_id == process_id) {
		CALL( LockPriority(&priority_lock_result) );
		PCB_Current_Running_Process->priority = new_priority;
		CALL( UnLockPriority(&priority_lock_result) );
		*process_error_return = PRIORITY_LEGAL;
		return;
	}
	else {																							// change priority of process in ReadyQueue, TimerQueue or maybe SuspendList
		CALL( LockTimer(&timer_queue_result) );
		
		in_timer_queue = IsExistsProcessIDQueue(TimerQueueHead,process_id);
		
		if (in_timer_queue) {
			CALL( UpdateProcessPriorityQueue(&TimerQueueHead,process_id,new_priority) );
		}
		
		CALL( UnLockTimer(&timer_queue_result) );

		CALL( LockReady(&ready_queue_result) );
		
		in_ready_queue = IsExistsProcessIDQueue(ReadyQueueHead,process_id);
		
		if (in_ready_queue) {
			PCB_Change_Priority = PullProcessFromQueue(&ReadyQueueHead, process_id);
			PCB_Change_Priority->priority = new_priority;
			CALL( make_ready_to_run(&ReadyQueueHead,PCB_Change_Priority) );
		}
		
		CALL( UnLockReady(&ready_queue_result) );
		CALL(process_printer("Priority", process_id,-1,-1,PRIORITY_AFTER) );
		*process_error_return = PRIORITY_LEGAL;
	}

}



/*****************************************************************************************************
message_send_legal

This function uses to validate a message going to be sent is legal or not. 
Need to be consider:
	* Length of buffer bigger than allowed.
	* A legal process_id to be sent message to.
	* Not over number of messages can be created in the system.
*******************************************************************************************************/

INT32 message_send_legal(INT32 target_process_id, INT32 send_length) 
{
	INT32		ret_value = -1;

	if  (target_process_id == -1)  {									// a success broadcast message
		if (send_length <= LEGAL_MESSAGE_LENGTH)
			ret_value = PROCESS_SEND_SUCCESS;
		else 
			return PROCESS_SEND_ILLEGAL_MSG_LENGTH;
	}
	if (ret_value != PROCESS_SEND_SUCCESS) {
		if (!IsExistsProcessIDArray(PCB_Table, target_process_id, number_of_processes_created) )					// send to illegal process
			return PROCESS_SEND_ILLEGAL_PID;

		if (send_length > LEGAL_MESSAGE_LENGTH) {																	// illegal message length
			return PROCESS_SEND_ILLEGAL_MSG_LENGTH;
		}
	}

	if (target_process_id == -1) {																					// limited broadcast message
		if ( (number_of_message_created + 1) > MAX_MESSAGES_IN_QUEUE) {
			return PROCESS_SEND_CREATE_OVER_MAX;
		}
	}
	else {
		if ( (number_of_message_created + 1) > MAX_MESSAGES) {
			return PROCESS_SEND_CREATE_OVER_MAX;
		}
	}

	return PROCESS_SEND_SUCCESS;

}



/********************************************************************************************************************************
send_message

This function is invoked by SEND_MESSAGE system call. 
When a legal message going to sent, allocate memory for a message. Then push  a message into Global Message Queue.  
A process sent a message then this message is kept in its SentBoxQueue. Some cases:
(1) If a messsage is a broadcast message (target_pid == -1) then wakes up any message resides in Message Suspend List. This
	allows message Ready to receive a message. 
(2) If a process sent a message is sent to its self then just need to add message to its SentBoxQueue.
(3) If send directly to specified process then make sure to send the lastest message in Global Message Queue. This happens
	sometimes a process is suspended before sending a message. When it resumes, it might receives newer messages then it has 
	to update its message to echo the lastest one. If a receiving process in Suspend Message List then sending process resumes
	receiving process, that allows receiving process runs and ready to receive message.
*********************************************************************************************************************************/

void send_message(INT32 target_pid, char *message, INT32 send_length, INT32 *process_error_return) {

	INT32						legal_send_message;
	Message						*msg;
	INT32						msg_length;
	INT32						error_ret;
	INT32						index;
	ProcessControlBlock			*tmp;

	if ( (legal_send_message = message_send_legal(target_pid, send_length) ) != PROCESS_SEND_SUCCESS)  {		// not a legal send  
		*process_error_return  =  legal_send_message;
		return;
	}

	//--------------------------//
	//	 message preparation    //
	//--------------------------//			
	

	global_msg_id++;	

																												//message_id starts at 1
	msg_length = strlen(message);

	if (target_pid == - 1) {		
																												// broadcast
		msg = CreateMessage(global_msg_id,target_pid,
			PCB_Current_Running_Process->process_id, send_length, msg_length,message,TRUE);

		Message_Table[number_of_message_created] = msg;															// Message_Table starts at 0
		number_of_message_created++;

		AddToSentBox(PCB_Table,PCB_Current_Running_Process->process_id,msg,number_of_processes_created);

		// wake up every processes that in suspend to Ready to receive messages
		while (!IsListEmpty(MessageSuspendListHead))
		{
			tmp = MessageSuspendListHead;
			//CALL(LockSuspend(&suspend_list_result));
	
			PCB_Transfer_MsgSuspend_To_Ready=  PullFromSuspendList(&MessageSuspendListHead,tmp->process_id) ;	
			PCB_Transfer_MsgSuspend_To_Ready->state = PROCESS_STATE_READY;
	
			//CALL(UnLockSuspend(&suspend_list_result));
	
			CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_MsgSuspend_To_Ready) );

		}

	}

	else if (target_pid == PCB_Current_Running_Process->process_id) {											// send self
		msg = CreateMessage(global_msg_id,target_pid,
			PCB_Current_Running_Process->process_id,send_length,msg_length,message, FALSE);

		Message_Table[number_of_message_created] = msg;															// Message_Table starts at 0
		number_of_message_created++;

		AddToSentBox(PCB_Table,target_pid,msg,number_of_processes_created);										// add to outbox of sent current process

	}

	else if (target_pid != PCB_Current_Running_Process->process_id) {											// send to another
		
		if ( (index = IsNewSendMsgInArray(Message_Table,target_pid,PCB_Current_Running_Process->process_id,
			PCB_Current_Running_Process->inboxQueue,number_of_message_created) ) == -1 )  {
			msg = CreateMessage(global_msg_id,target_pid,
				PCB_Current_Running_Process->process_id,send_length,msg_length,message, FALSE);
		}
		else {
			msg = CreateMessage(global_msg_id,target_pid,
				PCB_Current_Running_Process->process_id,send_length,strlen(Message_Table[index]->msg_buffer),Message_Table[index]->msg_buffer, FALSE);
		}

		Message_Table[number_of_message_created] = msg;															// Message_Table starts at 0
		number_of_message_created++;

		AddToSentBox(PCB_Table,PCB_Current_Running_Process->process_id,msg,number_of_processes_created);	

		if ( IsExistsProcessIDList(MessageSuspendListHead,target_pid) ) {												//------------//
			
			//RESUME_PROCESS(target_pid, &error_ret);															    // if target process is in Suspend then resume it first then Suspend self
			message_resume_process_id(target_pid);
			
			//SUSPEND_PROCESS(-1,&error_ret);																	// whenever I call this, return error at *(INT32 *)SystemCallData->Argument[1] = process_error_arg of Suspend Process
			message_suspend_process_id(PCB_Current_Running_Process->process_id);
		}

	}
	
																					// record sucessfully legal messages
	*process_error_return = PROCESS_SEND_SUCCESS;
}



/*****************************************************************************************************
message_receive_legal

This function uses to validate a message going to be received is legal or not. 
Need to be consider:
	* Length of buffer bigger than allowed.
	* Going to receive from invalid process_id.
*******************************************************************************************************/

INT32 message_receive_legal(INT32 source_pid, char* msg, INT32 receive_length) 
{

	INT32	index;
	INT32	ret_value = -1;

	if  (source_pid == -1)  {									// a success broadcast message
		if (receive_length <= LEGAL_MESSAGE_LENGTH)
			ret_value = PROCESS_RECEIVE_SUCCESS;
		else 
			return PROCESS_RECEIVE_ILLEGAL_MSG_LENGTH;
	}

	if (ret_value != PROCESS_RECEIVE_SUCCESS) {
		if (!IsExistsProcessIDArray(PCB_Table, source_pid, number_of_processes_created) )					// send to illegal process
			return PROCESS_RECEIVE_ILLEGAL_PID;

		if (receive_length > LEGAL_MESSAGE_LENGTH) {														// illegal message length
			return PROCESS_RECEIVE_ILLEGAL_MSG_LENGTH;
		}
	}

	//---------------------------------------//
	//   find in Message GLobal sent to me   //
	//---------------------------------------//
	if ( ( index = IsMyMessageInArray(Message_Table,
		PCB_Current_Running_Process->process_id, PCB_Current_Running_Process->inboxQueue, number_of_message_created) ) > -1) {						// someone sends me a message
			if (strlen(Message_Table[index]->msg_buffer) > receive_length )									// but if message length is not okie	
				return PROCESS_RECEIVE_ILLEGAL_MSG_LENGTH;													// I cannot receive it
			else {																							// else put it in my inbox if not inbox yet
				/*if (!IsExistsMessageIDQueue(PCB_Current_Running_Process->inboxQueue,
					Message_Table[index]->msg_id) ) {*/
						/*AddToInbox(PCB_Table,Message_Table[index]->target_id,Message_Table[index],number_of_processes_created); */
						
						strcpy(msg,Message_Table[index]->msg_buffer);
						
						ret_value = PROCESS_RECEIVE_SUCCESS;
				//}
			}
	}
	
	if (index == -1)
		return PROCESS_RECEIVE_SUCCESS;

	return ret_value;

}


/****************************************************************************************************************************************************
receive_message

This function is invoked by RECEIVE_MESSAGE system call. Some cases to be considered:
(I)   If a receiving message is a broadcast message then check this message already received or not. 
	  (1) If already received then no new message for this process. It suspends itself to let other processes to receive broadcast messsage.
	  (2) If receiving message is a new one, then add to process's inbox 
(II)  If a process receving message receive from itself then this case is as smaller case of (III)
(III) If a receving message is sent from specifed process (A) then current running process (B) suspends itself to make sure source process (A) 
	  can run and completes its send. The sending process (A) now can run, send a message to (B) with making a call to resume receiving process (A). 
	  Finally, (A) resumes, add message to its inbox and then updates necessary output parameters.
******************************************************************************************************************************************************/

void receive_message(INT32 source_pid, char *message, INT32 receive_length, INT32 *send_length, INT32 *send_pid, INT32 *process_error_return) {

	INT32				legal_receive_message;
	INT32				index;
	INT32				receive_send_length;
	INT32				actual_send_length;
	INT32				actual_send_pid;
	INT32				error_ret;
	ProcessControlBlock *tmp;

	if ( (legal_receive_message = message_receive_legal(source_pid,message, receive_length) ) != PROCESS_RECEIVE_SUCCESS)  {		// not a legal send  
		*process_error_return  =  legal_receive_message;
		return;
	}

	if (source_pid == -1) {																			// receive from anyone
		if ( ( index = IsMyMessageInArray(Message_Table,
		PCB_Current_Running_Process->process_id,PCB_Current_Running_Process->inboxQueue, number_of_message_created) ) == -1) {				// no one sends me a NEW message
			//SUSPEND_PROCESS(-1,&error_ret);														// suspend myself and I wait										
			if (IsQueueEmpty(ReadyQueueHead) ) {
				if (IsListEmpty(SuspendListHead ) ) {												// Pull out from MessageSuspendList only when SuspenList is empty
					while (!IsListEmpty(MessageSuspendListHead))									// otherwise fight each other in ready
					{
						tmp = MessageSuspendListHead;
						//CALL(LockSuspend(&suspend_list_result));
	
						PCB_Transfer_MsgSuspend_To_Ready=  PullFromSuspendList(&MessageSuspendListHead,tmp->process_id) ;	
						PCB_Transfer_MsgSuspend_To_Ready->state = PROCESS_STATE_READY;
	
						//CALL(UnLockSuspend(&suspend_list_result));
	
						CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_MsgSuspend_To_Ready) );

					}
				}
			}
			message_suspend_process_id(PCB_Current_Running_Process->process_id);									// NOT receive anything so suspend for waiting 
		}																											//suspend self already include dispatcher
																							
			
	

		//--------------------------------------//																							
		// return where suspend. Index cleared  //
		//--------------------------------------//

			index = IsMyMessageInArray(Message_Table,
			PCB_Current_Running_Process->process_id,PCB_Current_Running_Process->inboxQueue,number_of_message_created);

			if (!IsExistsMessageIDQueue(PCB_Current_Running_Process->inboxQueue,
					Message_Table[index]->msg_id) ) {
				
				CALL( AddToInbox(PCB_Table,PCB_Current_Running_Process->process_id, Message_Table[index],number_of_processes_created) );
			}
			//AddToInbox(PCB_Table,Message_Table[index]->target_id,Message_Table[index],number_of_processes_created);

			actual_send_length = strlen(Message_Table[index]->msg_buffer);
			actual_send_pid = Message_Table[index]->source_id;
			strcpy(message,Message_Table[index]->msg_buffer);
			receive_send_length =  Message_Table[index]->send_length;
			

	}
	else if (source_pid == PCB_Current_Running_Process->process_id) {

	}
	else if (source_pid != PCB_Current_Running_Process->process_id) {
		message_suspend_process_id(PCB_Current_Running_Process->process_id);									// NOT receive anything so suspend for waiting 

		//--------------------------------------//																							
		// return where suspend. Index cleared  //
		//--------------------------------------//

			index = IsMyMessageInArray(Message_Table,
			PCB_Current_Running_Process->process_id,PCB_Current_Running_Process->inboxQueue,number_of_message_created);

			if (!IsExistsMessageIDQueue(PCB_Current_Running_Process->inboxQueue,
					Message_Table[index]->msg_id) ) {
				
				CALL( AddToInbox(PCB_Table,PCB_Current_Running_Process->process_id, Message_Table[index],number_of_processes_created) );
			}
			//AddToInbox(PCB_Table,Message_Table[index]->target_id,Message_Table[index],number_of_processes_created);
						
			actual_send_length = strlen(Message_Table[index]->msg_buffer);
			actual_send_pid = Message_Table[index]->source_id;
			strcpy(message,Message_Table[index]->msg_buffer);
			receive_send_length =  Message_Table[index]->send_length;


	}
	*process_error_return = PROCESS_RECEIVE_SUCCESS;
	*send_pid = actual_send_pid;
	*send_length = receive_send_length;
	
	

}

/*****************************************************************************************************
message_suspend_process_id

The functionality of this process is simalar to suspend_process_id, except without output parameters. 
This function is called in both send_message and receiving message to make sure a process sends a 
right message or receives the message it expected.
The processe is suspended to be push in Message Suspend List. This List is different from SuspendList,
which is used in SUSPEND_PROCESS and RECEIVE_PROCESS.
*******************************************************************************************************/

void message_suspend_process_id(INT32  process_id)
{
	INT32						legal_suspend_process; 
	BOOL						in_ready_queue;
	BOOL						in_timer_queue;
	INT32						*new_update_time;
	INT32						Time;
	ProcessControlBlock			*prev_timer_head;

																			// if from send_receive then always ok		
	//CALL(process_printer("MsgSus",process_id,-1,-1,SUSPEND_BEFORE) );

	if (PCB_Current_Running_Process->process_id == process_id) {                                       // suspend self
		//CALL(LockSuspend(&suspend_list_result));
		PCB_Current_Running_Process->state = PROCESS_STATE_SUSPEND_MESSAGE;
		AddToMsgSuspendList(&MessageSuspendListHead, PCB_Current_Running_Process);					  // there is nothing left to run	
		//CALL(UnLockSuspend(&suspend_list_result));

		//CALL(process_printer("MsgSus",process_id,-1,-1,SUSPEND_AFTER) );
		
		CALL( dispatcher());																		  // let another process run				
		
	}
	else {																							   // suspend another process
		CALL( LockTimer(&timer_queue_result) );
		in_timer_queue = IsExistsProcessIDQueue(TimerQueueHead,process_id);
		if (in_timer_queue) {
			prev_timer_head = TimerQueueHead;														// to record Timer interrupt

			PCB_Transfer_Timer_To_MsgSuspend = PullProcessFromQueue(&TimerQueueHead,process_id); 
			PCB_Transfer_Timer_To_MsgSuspend->state = PROCESS_STATE_SUSPEND;

			if (prev_timer_head != TimerQueueHead) {												// update TimerQueue when chaning in header
				if (TimerQueueHead != NULL) {														// just take out the header	
					new_update_time = (INT32*) malloc(sizeof(INT32));
					MEM_READ(Z502ClockStatus, &Time);			
					*new_update_time = TimerQueueHead->wakeup_time - Time;	
					if (*new_update_time > 0) {														// if time is not pass
						MEM_WRITE(Z502TimerStart, new_update_time);									// set timer based on new timer queue head
						//debug
						//printf("... Suspend... new_update_time: %d \n",*new_update_time);
						//debug
					}
					else {																			// time already pass
						MEM_WRITE(Z502TimerStart, &generate_interrupt_immediately);
						//debug
						//printf("... Suspend... Time Pass... IMMEDIATELY GEN: \n");
						//debug
					}
	
					free(new_update_time);
				}
				else {}																				// NULL then pull the only one. Leave Timer as is it because interrupt will not process anything in Interrupt_handler
			}
		}
		CALL( UnLockTimer(&timer_queue_result) );

		CALL(LockReady(&ready_queue_result));
		in_ready_queue = IsExistsProcessIDQueue(ReadyQueueHead,process_id);
		if ( in_ready_queue ) {	
			PCB_Transfer_Ready_To_MsgSuspend = PullProcessFromQueue(&ReadyQueueHead,process_id); 
			PCB_Transfer_Ready_To_MsgSuspend->state = PROCESS_STATE_SUSPEND;

			CALL(LockSuspendListMessage(&message_suspend_list_result));
			AddToMsgSuspendList(&MessageSuspendListHead, PCB_Transfer_Ready_To_MsgSuspend);
			CALL(UnLockSuspendListMessage(&message_suspend_list_result))
		}
		CALL(UnLockReady(&ready_queue_result));

		//CALL(process_printer("MsgSus",process_id,-1,-1,SUSPEND_AFTER) );
		
	}

}

/*************************************************************************************************************************************
message_resume_process_id

The process to be resumed taken out from Suspend List and put in ReadyQueue by calling make_ready_to_run.
*************************************************************************************************************************************/

void message_resume_process_id(INT32  process_id)
{
	INT32					legal_resume_process; 
	ProcessControlBlock		*tmp;	

	
	tmp = ReadyQueueHead;
	//CALL(LockSuspend(&suspend_list_result));
	
	PCB_Transfer_MsgSuspend_To_Ready=  PullFromSuspendList(&MessageSuspendListHead,process_id) ;	
	PCB_Transfer_MsgSuspend_To_Ready->state = PROCESS_STATE_READY;
	
	//CALL(UnLockSuspend(&suspend_list_result));
	
	CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_MsgSuspend_To_Ready) );
	
}
