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
INT32						global_process_id = 0;													// keep track of process_id  
INT32						number_of_processes_created = 0;										// keep track of number processes created. It might need lock
																									// = global_process_id but easier to read	
INT32						lock_switch_thread_result;												// lock for ready queue
INT32						printer_lock_result;													// used for locking printing code

INT32						generate_interrupt_immediately  = 1 ;									// use for generate interrupt when Timer passed the wake-up time

BOOL						wait_interrupt_finish;
INT32						interrupt_level = 0;

ProcessControlBlock			*TimerQueueHead;														// TimerQueue 
ProcessControlBlock			*ReadyQueueHead;														// ReadyQueue

ProcessControlBlock			*PCB_Transfer_Timer_To_Ready;											// used to transfer PCB in interrupt
ProcessControlBlock			*PCB_Transfer_Ready_To_Timer;											// used to transfer PCB in start_timer
ProcessControlBlock			*PCB_Current_Running_Process;											// keep point to currrent running process, set when SwitchContext, Dispatcher
																									// used as ProcessControlBlock			*PCB_Transfer_Ready_To_Timer;											// used to transfer PCB in start_timer
ProcessControlBlock			*PCB_Terminating_Processs;												// keep current context then destroy in next context

ProcessControlBlock			*PCB_Table[MAX_PROCESSES];												// Move PCB Table to array.  //ProcessControlBlock			*ListHead;				// PCB table
																									// With List, I have to allocate seperate memeory for Table. With array, TimerQueue and ReadQueue can point to same as array

void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 process_id, INT32 *error);
INT32	check_legal_process(char* process_name, INT32 initial_priority);							// check Legal process before create process
void	start_timer(long * sleepTime);
void	terminate_proccess_id(INT32 process_id, INT32 *process_error_return);						// terminate specified a process with process_id
void	make_ready_to_run(ProcessControlBlock **ReadyQueueHead, ProcessControlBlock *pcb);			// Insert into readque
void	dispatcher();
void	please_charge_hardware_time() {};															// increase time function, used in infinite loop
void	process_printer(char* action, INT32 target_process_id, INT32 running_process_id, // process_id or target = -1
						INT32 waiting_mode,INT32 ready_mode, INT32 suspended_mode, INT32 swapped_mode,// means does not print out in state printer
						INT32 terminated_process_id, INT32 new_process_id);

void LockTimer (INT32 *timer_lock_result);
void UnLockTimer (INT32 *timer_lock_result);
void LockPrinter (INT32 *printer_lock_result);
void UnLockPrinter(INT32 *printer_lock_result);

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

	CALL( LockTimer(&lock_switch_thread_result) );
	interrupt_level++;
    // Get cause of interrupt
    CALL( MEM_READ(Z502InterruptDevice, &device_id ) );

    // Set this device as target of our query
    CALL ( MEM_WRITE(Z502InterruptDevice, &device_id ) );

	// Now read the status of this device
    CALL( MEM_READ(Z502InterruptStatus, &status ) );

	if ( device_id == TIMER_INTERRUPT && status == ERR_SUCCESS ) {
		MEM_READ(Z502ClockStatus, &Time);													// read current time and compare to front of Timer queue

		tmp = TimerQueueHead;
		while ( (tmp!= NULL) && (tmp->wakeup_time <= Time) ) {
			//debug
			//printf (" ...ready ... process_id: %d, wake-up: %d, Time: %d \n", tmp->process_id, tmp->wakeup_time, Time);
			//debug

			CALL( LockTimer(&lock_switch_thread_result) );
			PCB_Transfer_Timer_To_Ready = DeQueue(&TimerQueueHead);							// take off front Timer queue

			CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_Timer_To_Ready) );		// put in the end of Ready Queue

			CALL( LockPrinter(&printer_lock_result) );
			process_printer("Ready",-1,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1);
			CALL( UnLockPrinter(&printer_lock_result) );

			CALL( UnLockTimer(&lock_switch_thread_result) );
			
			tmp = TimerQueueHead;															// transfer until front Timer Queue > current time

		}

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
	}
	interrupt_level--;
	CALL( UnLockTimer(&lock_switch_thread_result) );
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
    short               call_type;
    static INT16        do_print = 100;
    INT32               Time;
	short               i;
	INT32				create_priority;			 // read argument 2 - prioirty of CREATE_PROCESS
	long				SleepTime;					 // using in SYSNUM_SLEEP
	//ProcessControlBlock *newPCB;					 // create new process
	INT32				terminate_arg;				 // terminate argument of TERMINATE_PROCESS
	char				*process_name_arg = NULL;	 // process name argument of GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_error_arg;			 // process error check for GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_id_arg;				 // process_id returns in GET_PROCESS_ID, CREATE_PROCESS
	void				*starting_address;			 // starting address of rountine in CREATE_PROCESS
	INT32				generate_interrupt = 1;
	BOOL				loop_wait_fill_ReadyQueue = TRUE;

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
        case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
            MEM_READ( Z502ClockStatus, &Time );
			*(INT32 *)SystemCallData->Argument[0] = Time;
            break;
       
		case SYSNUM_SLEEP:
			SleepTime = (INT32) SystemCallData->Argument[0];
			start_timer(&SleepTime);
			break;

		// terminate system call
		case SYSNUM_TERMINATE_PROCESS:
			
			terminate_arg =  (INT32) SystemCallData->Argument[0];		//get process_id

			if (terminate_arg == -1) {									//terminate self means terminate current running process
				CALL( terminate_proccess_id(PCB_Current_Running_Process->process_id,&process_error_arg) );		//this will not free PCB_Current_Running_Process. Free in the queue only
				
				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;

				CALL( dispatcher() );													//try to switch to next process after current process terminated	
				
			}

			else if (terminate_arg == -2) {								//terminate self + children
				// global_process_id
				CALL( DeleteQueue(TimerQueueHead) );							// release all heap memory before quit
				CALL( DeleteQueue(ReadyQueueHead) );
				CALL( Z502Halt() );
			}
			else  {																	// terminate a process with process_id
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
	INT32				test_case_prioirty = 1;
	

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

	//----------------------------------------------------------//
	//				Run test manuallly			    			//
	//----------------------------------------------------------//

	//CALL(os_create_process("test0",(void*) test0,test_case_prioirty, &created_process_id, &created_process_error));
	//Z502MakeContext( &next_context, (void*) test0, USER_MODE );	
	//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );

	//CALL( os_create_process("test1a",(void*) test1a,test_case_prioirty, &created_process_id, &created_process_error) );
	//Z502MakeContext( &next_context, (void*) test1a, USER_MODE );	
	//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );

	//CALL ( os_create_process("test1b",(void*) test1b,test_case_prioirty, &created_process_id, &created_process_error) );
	//CALL (set_pcb_current_running_process("test1b",(void*)test1b, test_case_prioirty,&created_process_id));					//set current running process. Used in GET_PROCESS_ID
	//Z502MakeContext( &next_context, (void*) test1b, USER_MODE );	
	//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );
	
	CALL ( os_create_process("test1c",(void*) test1c,test_case_prioirty, &created_process_id, &created_process_error) );
	
}                                               // End of osInit


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id, INT32 *error)
{
	 //void						*next_context;
	 INT32						legal_process;
	 ProcessControlBlock		*newPCB;
	 
	 LockPrinter(&printer_lock_result);
	 if (PCB_Current_Running_Process != NULL) {
		 process_printer("Create",-1,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1) ;
	 }
	 else {
		 process_printer("Create",-1,-1,1,1,-1,-1,-1,-1) ;
	 }
	 
	 UnLockPrinter(&printer_lock_result);

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

	Z502MakeContext( &newPCB->context, (void*) starting_address, USER_MODE );	

	
	CALL(make_ready_to_run(&ReadyQueueHead,newPCB) );												// whenever new process created, put into ready queue

}




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


void start_timer(long *sleepTime) 
{
	INT32		Time;														// get current time assigned to 
	INT32		Timer_Status;
	INT32		*new_update_time = NULL;									// for Relative time from new head after put into Ready queue
	BOOL		loop_wait_ReadyQueue_filled = TRUE;
	ProcessControlBlock *prev_Head = TimerQueueHead;						// to get timer of HeadTimerQueue. Needed for updatding Timer when changes in header

	MEM_READ (Z502ClockStatus, &Time);								        // get current time
	
	//----------------------------------------------------------------------//
	//	PCB_Current_Running_Process <-- ReadyQueue Head from dispatcher     //
	//----------------------------------------------------------------------//
	PCB_Current_Running_Process->wakeup_time = Time + (INT32) (*sleepTime);					// Set absolute time to PCB
	PCB_Current_Running_Process->state	= PROCESS_STATE_SLEEP;								// Current PCB still points to ReadyQueue Head


	CALL( LockTimer(&lock_switch_thread_result) );
	PCB_Transfer_Ready_To_Timer = DeQueue(&ReadyQueueHead);														// Remove from ReadyQueue so no need to point

	CALL( AddToTimerQueue(&TimerQueueHead, PCB_Transfer_Ready_To_Timer) );					// then Add to Timer queue
	
	CALL( LockPrinter(&printer_lock_result) );
	if (PCB_Current_Running_Process != NULL) {
		 process_printer("Waiting",-1,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1) ;
	}
	else  {
		 process_printer("Waiting",-1,-1,1,1,-1,-1,-1,-1) ;
	}
	CALL( UnLockPrinter(&printer_lock_result) );
	
	CALL( UnLockTimer(&lock_switch_thread_result) );
	

	
	if (prev_Head == NULL) {                                            // no process in timerQueue
			
		// debug
		// printf("... Waiting... NULL TimerQueue... Process sleep: %d \n",PCB_Current_Running_Process->process_id);
		// printf("... Waiting... NULL TimerQueue... new timer: %d, current time: %d , wake up time %d \n",*new_update_time,Time,PCB_Current_Running_Process->wakeup_time);
		//debug
			
		MEM_WRITE( Z502TimerStart, sleepTime );							// go ahead and set timer
	}
	else if ( (prev_Head->wakeup_time > TimerQueueHead->wakeup_time) ||  // compare wakeup time of new inserted with 1st in timer queue
		( ! IsExistsProcessIDQueue(TimerQueueHead,prev_Head->process_id)) ) {    // in case prev_head goes ReadyQueue        
			
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
		
	CALL( dispatcher() );

}

void make_ready_to_run(ProcessControlBlock **head_of_ready, ProcessControlBlock *pcb) 
{
	CALL( AddToReadyQueue(&ReadyQueueHead,pcb) );

	if ( strcmp(pcb->process_name, "test1c") == 0 )
		CALL( dispatcher() );

}

void terminate_proccess_id(INT32 process_id, INT32* error_return) 
{
	INT32	Time;
	INT32	*new_start_timer;

	CALL( LockPrinter(&printer_lock_result) );
	CALL( process_printer("Kill",-1,-1,1,1,-1,-1,-1,-1) );
	CALL( UnLockPrinter(&printer_lock_result) );

	if ( !IsExistsProcessIDArray(PCB_Table,  process_id, number_of_processes_created) ) {							//invalid process_id
		*error_return = PROCESS_TERMINATE_INVALID_PROCESS_ID;
		return;
	}
	else {																					

		if (IsExistsProcessIDQueue(ReadyQueueHead,process_id) ) {
			CALL( LockTimer(&lock_switch_thread_result) );
			CALL(RemoveProcessFromQueue(&ReadyQueueHead,process_id) );
			CALL( UnLockTimer(&lock_switch_thread_result) );
		}
		if (IsExistsProcessIDQueue(TimerQueueHead,process_id) ) {
			CALL( LockTimer(&lock_switch_thread_result) );
			CALL( RemoveProcessFromQueue(&TimerQueueHead,process_id) );						//remove from timer queue
			CALL( UnLockTimer(&lock_switch_thread_result) );

			MEM_READ (Z502ClockStatus, &Time);	
			new_start_timer = (INT32*) malloc(sizeof(INT32));
			*new_start_timer = TimerQueueHead->wakeup_time - Time;
			if (*new_start_timer > 0) {													   // remove from TimerQueue need update timer
				MEM_WRITE( Z502TimerStart, new_start_timer );
			}
			else {																		   // while removing old head maybe time already pass	
				MEM_WRITE( Z502TimerStart, &generate_interrupt_immediately );							   // generate new interrupt	
			}
			free(new_start_timer);
			
		}

		CALL( LockTimer(&lock_switch_thread_result) );										// remove from PCB table
		CALL( RemoveFromArray(PCB_Table,process_id,number_of_processes_created) );
		PCB_Current_Running_Process = ReadyQueueHead;
		CALL( UnLockTimer(&lock_switch_thread_result) );
		*error_return = ERR_SUCCESS;														//success

		return;
		
	}
}


void dispatcher() {
	
	while (IsQueueEmpty(ReadyQueueHead)) {
		CALL(Z502Idle());
	}

	CALL( LockTimer(&lock_switch_thread_result) );

	CALL( LockPrinter(&printer_lock_result) );

	if (PCB_Current_Running_Process != NULL) {
		 process_printer("Run",-1,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1) ;
	}
	else { 
		 process_printer("Run",-1,-1,1,1,-1,-1,-1,-1) ;
	}
	CALL( UnLockPrinter(&printer_lock_result) );

	PCB_Current_Running_Process = ReadyQueueHead; 

	
	PCB_Current_Running_Process->state = PROCESS_STATE_RUNNING;											// make running process
	if ( (PCB_Current_Running_Process != NULL) && (ReadyQueueHead != NULL) && 
		(PCB_Current_Running_Process->process_id == ReadyQueueHead->process_id) )		{				// run when same process_id. Otherwise already run

			CALL( UnLockTimer(&lock_switch_thread_result) );
			if(interrupt_level > 0) { return; };														// interrupt is running
			Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &PCB_Current_Running_Process->context );		//switch context always by Current_Running_Process
	}

	
}

void	process_printer(char* action, INT32 target_process_id, INT32 running_process_id, // process_id or target = -1
						INT32 waiting_mode,INT32 ready_mode, INT32 suspended_mode, INT32 swapped_mode,// means does not print out in state printer
						INT32 terminated_process_id, INT32 new_process_id) 
{
	INT32					Time;
	ProcessControlBlock		*readyQueue = ReadyQueueHead;			// for ready mode
	ProcessControlBlock		*timerQueue = TimerQueueHead;			// for waiting mode

	MEM_READ(Z502ClockStatus, &Time);									
	CALL(SP_setup( SP_TIME_MODE, Time) );

	CALL(SP_setup_action(SP_ACTION_MODE, action) );					//CREATE RUN READY WAITING
	
	// ------------------
	// Print one value
	// ------------------
	if (target_process_id != -1)
		CALL(SP_setup( SP_TARGET_MODE, target_process_id) );

	if (running_process_id != -1)
		CALL(SP_setup( SP_RUNNING_MODE, running_process_id) );

	if (terminated_process_id != -1)
		CALL(SP_setup( SP_TERMINATED_MODE, terminated_process_id) );

	if (new_process_id != -1)
		CALL(SP_setup( SP_NEW_MODE, new_process_id) );

	// --------------------
	// Print queue, table
	// --------------------
	
	if (waiting_mode != -1) {
		if (!IsQueueEmpty(TimerQueueHead)) {							// print only queue is not blank
			while (timerQueue != NULL) {
				CALL( SP_setup( SP_WAITING_MODE,timerQueue->process_id) ) ;
				timerQueue = timerQueue->nextPCB;
			}
		}
	}
	
	if ( ready_mode != -1) {
		if (!IsQueueEmpty(ReadyQueueHead)) {							// print only queue is not blank
			while (readyQueue != NULL) {
				CALL( SP_setup( SP_READY_MODE,readyQueue->process_id) );
				readyQueue = readyQueue->nextPCB;
			}
		}
	}

	if (swapped_mode != -1) {}
	
	if (suspended_mode != -1) {}

	CALL( printf("\n-------------------------scheduler_printer Output------------------------\n") );
    CALL( SP_print_header() );
    CALL( SP_print_line() );
    CALL( printf("-------------------------------------------------------------------------\n\n") );


}

void LockTimer (INT32 *timer_lock_result){	
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_TIMER_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, timer_lock_result );
}

void UnLockTimer (INT32 *timer_lock_result){
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PROCESS_TIMER_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, timer_lock_result );
}

void LockPrinter (INT32 *printer_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRINTER_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, printer_lock_result );
}

void UnLockPrinter(INT32 *printer_lock_result) {
	READ_MODIFY( MEMORY_INTERLOCK_BASE + PRINTER_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, printer_lock_result );
}