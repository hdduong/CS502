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


/**************************** <hdduong ********************************/
INT32						global_process_id = 0;													// keep track of process_id
INT32						flag_switch_and_destroy = FALSE;										// destroy a context when terminate
ProcessControlBlock			*TimerQueueHead;														// TimerQueue 
ProcessControlBlock			*ReadyQueueHead;														// ReadyQueue
ProcessControlBlock			*PCB_Transfer_Ready_To_Timer;											// used to transfer PCB in start_timer
ProcessControlBlock			*PCB_Transfer_Timer_To_Ready;											// used to transfer PCB in interrupt
ProcessControlBlock			*PCB_Current_Running_Process;											// keep point to currrent running process, set when SwitchContext, Dispatcher
ProcessControlBlock			*PCB_Terminating_Processs;												// keep current context then destroy in next context
ProcessControlBlock			*ListHead;																// PCB table


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 process_id, INT32 *error);
INT32	check_legal_process(char* process_name, INT32 initial_priority);							// check Legal process before create process
void	start_timer(long * sleepTime);
void	terminate_proccess_id(INT32 process_id, INT32 *process_error_return);						// terminate specified a process with process_id
void	make_ready_to_run(ProcessControlBlock **ReadyQueueHead, ProcessControlBlock *pcb);			// Insert into readque
void	set_pcb_current_running_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id);		//set current pcb process for switch context
void	dispatcher(INT32 *destroy_context);
void	please_charge_hardware_time() {};															// increase time function, used in infinite loop
void	process_printer(char* action, INT32 target_process_id, INT32 running_process_id, // process_id or target = -1
						INT32 waiting_mode,INT32 ready_mode, INT32 suspended_mode, INT32 swapped_mode,// means does not print out in state printer
						INT32 terminated_process_id, INT32 new_process_id);
/************************** hdduong> **********************************/



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
	ProcessControlBlock	*tmp = TimerQueueHead;								// check front of time queue head

	//printf(" .... Start interrupt_handler TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Start interrupt_handler Timer Queue! ..."); CALL( PrintQueue(TimerQueueHead) );
	//printf(" .... Start interrupt_handler ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Start interrupt_handler Ready Queue! ..."); CALL( PrintQueue(ReadyQueueHead) );
	 process_printer("S_Ready",-1,-1,1,1,-1,-1,-1,-1);

    // Get cause of interrupt
    CALL( MEM_READ(Z502InterruptDevice, &device_id ) );

    // Set this device as target of our query
    CALL ( MEM_WRITE(Z502InterruptDevice, &device_id ) );

	// Now read the status of this device
    CALL( MEM_READ(Z502InterruptStatus, &status ) );

	if ( device_id == TIMER_INTERRUPT && status == ERR_SUCCESS ) {
		MEM_READ(Z502ClockStatus, &Time);									// read current time and compare to front of Timer queue
		//printf("... interrupt_handler current_time....%d \n",Time);

		while ( (tmp!= NULL) && (tmp->wakeup_time <= Time) ) {
			PCB_Transfer_Timer_To_Ready = DeQueue(&TimerQueueHead);						// take off front Timer queue
			
			/*if (PCB_Current_Running_Process != NULL) {
				process_printer("Ready",PCB_Transfer_Timer_To_Ready->process_id,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1);
			}*/

			CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer_Timer_To_Ready) );		// put in the end of Ready Queue
			
			tmp = TimerQueueHead;											// transfer until front Timer Queue > current time

		}

		//update start timer based on new head
		if (tmp != NULL) {
			new_update_time = (INT32*) malloc(sizeof(INT32));
			MEM_READ(Z502ClockStatus, &Time);			
			*new_update_time = tmp->wakeup_time - Time;							// there: tmp== NULL or tmp->wakeup_time > Time
			MEM_WRITE(Z502TimerStart, new_update_time);						// set timer based on new timer queue head
			//printf("...interrupt_handler set timer hardware %d....\n", *new_update_time);
			free(new_update_time);
		}
	}

	// Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
	
	 process_printer("E_Ready",-1,-1,1,1,-1,-1,-1,-1);
	//printf(" .... Ending interrupt_handler TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Ending interrupt_handler Timer Queue! ..."); CALL( PrintQueue(TimerQueueHead) );
	//printf(" .... Ending interrupt_handler ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Ending interrupt_handler Ready Queue! ..."); CALL( PrintQueue(ReadyQueueHead) );

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
	ProcessControlBlock *newPCB;					 // create new process
	INT32				terminate_arg;				 // terminate argument of TERMINATE_PROCESS
	char				*process_name_arg = NULL;	 // process name argument of GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_error_arg;			// process error check for GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_id_arg;				 // process_id returns in GET_PROCESS_ID, CREATE_PROCESS
	void				*starting_address;			 // starting address of rountine in CREATE_PROCESS
	INT32				generate_interrupt = 1;
	BOOL				loop_wait_fill_ReadyQueue = TRUE;

    call_type = (short)SystemCallData->SystemCallNumber;
    if ( do_print > 0 ) {
        // same code as before
		 printf( "SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++ ){
        	 //Value = (long)*SystemCallData->Argument[i];
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
			//CALL(Z502Halt());
			terminate_arg =  (INT32) SystemCallData->Argument[0];		//get process_id

			if (terminate_arg == -1) {									//terminate self means terminate current running process
				terminate_proccess_id(PCB_Current_Running_Process->process_id,&process_error_arg);		//this will not free PCB_Current_Running_Process. Free in the queue only
				PCB_Terminating_Processs = CreateProcessControlBlockWithData( PCB_Current_Running_Process->process_name,
																			PCB_Current_Running_Process->context, 
																			PCB_Current_Running_Process->priority,
																			PCB_Current_Running_Process->process_id);					
				*(INT32 *)SystemCallData->Argument[1] = process_error_arg;
				/*printf("... Terminating PID: %d...\n",PCB_Current_Running_Process->process_id);
				printf(" .... Terminate TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Terminate Timer Queue! ..."); CALL( PrintQueue(TimerQueueHead) );
				printf(" .... Terminate ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Terminate Ready Queue! ..."); CALL( PrintQueue(ReadyQueueHead) );*/
				process_printer("Termint",-1,-1,1,1,-1,-1,-1,-1);
				flag_switch_and_destroy = TRUE;

				if ( IsQueueEmpty(ReadyQueueHead) ) {
					MEM_WRITE(Z502TimerStart, &generate_interrupt);	
					MEM_READ( Z502ClockStatus, &Time );
					//printf(" .... Terminate Set Clock Size: %d  at %d ... ",generate_interrupt,Time);
					flag_switch_and_destroy = TRUE;
					//Z502Idle();

				}

				dispatcher(&flag_switch_and_destroy);							//try to switch if 

				while(loop_wait_fill_ReadyQueue) {								// loop until something filled in ready queue
						if (!IsQueueEmpty(ReadyQueueHead)) {                    // fix the error with Ready queue is empty because of removing
							loop_wait_fill_ReadyQueue = FALSE;                  // but cannot switch to another context
							dispatcher(&flag_switch_and_destroy);				// switch to other contexts
						}														// loop_wait_fill_ReadyQueue will be changed to FALSE in dispatcher if TRUE;
						CALL(please_charge_hardware_time);						// recharge the time	
				}
				
				
			}
			else if (terminate_arg == -2) {								//terminate self + children
				// global_process_id
				DeleteQueue(TimerQueueHead);							// release all heap memory before quit
				DeleteQueue(ReadyQueueHead);
				Z502Halt();
			}
			else  {																	// terminate a process with process_id
				//SleepTime = 0;
				terminate_proccess_id(terminate_arg, &process_error_arg);	
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
				process_id_arg = GetProcessID(ListHead,process_name_arg);
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
		CALL (set_pcb_current_running_process("test1c",(void*)test1c, test_case_prioirty,&created_process_id));					//set current running process. Used in GET_PROCESS_ID
		//Z502MakeContext( &next_context, (void*) test1c, USER_MODE );	
		//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );
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
	CALL (set_pcb_current_running_process("test1c",(void*)test1c, test_case_prioirty,&created_process_id));					//set current running process. Used in GET_PROCESS_ID
	//Z502MakeContext( &next_context, (void*) test1c, USER_MODE );	
	//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );

}                                               // End of osInit


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id, INT32 *error)
{
	 void						*next_context;
	 INT32						legal_process;
	 ProcessControlBlock		*newPCB, *newPCBList;
	 
	 // for debugging
	 char*						process_name_print;
	 process_name_print = (char*) malloc(strlen(process_name)  + 1);
	 strcpy(process_name_print,process_name);
	 process_name_print[strlen(process_name)] = '\0';
	 //printf("... Start os_create_process %s...", process_name_print); 
	 //printf(" .... TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Timer Queue! ...\n"); CALL( PrintQueue(TimerQueueHead) );
	 //printf(" .... ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Ready Queue! ...\n"); CALL( PrintQueue(ReadyQueueHead) );
	 process_printer("S_Create",-1,-1,1,1,-1,-1,-1,-1);

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
	  
	newPCB->wakeup_time = 0;															  // add more field for newPCB if successfully created
	newPCB->state = PROCESS_STATE_CREATE;

	*process_id = global_process_id;														// prepare for return values
	*error = PROCESS_CREATED_SUCCESS;
	

	make_ready_to_run(&ReadyQueueHead,newPCB);												// whenever new process created, put into ready queue
	
	Z502MakeContext( &newPCB->context, (void*) starting_address, USER_MODE );	


	// another instance of newPCB otherwise memory mix
	newPCBList = CreateProcessControlBlockWithData( (char*) process_name,						 // create new pcb
											(void*) starting_address,
											priority,
											global_process_id);

	newPCBList->process_id = newPCB->process_id;
	ListHead = InsertLinkedListPID(ListHead,newPCBList);										// insert new pcb to pcb table

	if (strcmp(process_name,"test1c") == 0) {                                                   // after newPCBList to make sure test case in PCB table
		dispatcher(&flag_switch_and_destroy);
	}


	//printf("... ending os create process %s...", process_name_print);	
	free(process_name_print);
	process_printer("E_Create",-1,-1,1,1,-1,-1,-1,-1);
	//printf(" .... TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Timer Queue! ..\n"); CALL( PrintQueue(TimerQueueHead) );
	//printf(" .... ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Ready Queue! ...\n");CALL( PrintQueue(ReadyQueueHead) );

	//if (PCB_Current_Running_Process != NULL) {													// at least one process created
	//	process_printer("Create",newPCB->process_id,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,newPCB->process_id);
	//}
}




INT32	check_legal_process(char* process_name, INT32 initial_priority) 
{
	INT32	MAX_EXCEED_PROCESSES = 20;

	if(	initial_priority <= 0	) {															// ILLEGAL Priority
		return PROCESS_CREATED_ILLEGAL_PRIORITY;
	}
	else if ( IsExistsProcessNameLinkedList(ListHead, process_name) ) {						//already process_name
		return PROCESS_CREATED_ERROR_SAME_NAME;
	}
	else if ( SizeLinkedListPID(ListHead) >= MAX_EXCEED_PROCESSES) {
		return PROCESS_CREATED_MAX_EXCEED;
	}
	return PROCESS_LEGAL;
}

// start_timer transfers from ReadyQueue to TimerQueue
void start_timer(long *sleepTime) 
{
	INT32		Time;														// get current time assigned to 
	INT32		Timer_Status;
	INT32		*new_update_time = NULL;										// for Relative time from new head after put into Ready queue
	BOOL		loop_wait_ReadyQueue_filled = TRUE;
	ProcessControlBlock *prev_Head = TimerQueueHead;						// to get timer of HeadTimerQueue

	process_printer("S_Sleep",-1,-1,1,1,-1,-1,-1,-1);
	 //printf(" .... Start start_timer TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Timer Queue! ..."); CALL( PrintQueue(TimerQueueHead) );
	 //printf(" .... Start start_timer ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Ready Queue! ..."); CALL( PrintQueue(ReadyQueueHead) );

	MEM_READ (Z502ClockStatus, &Time);								        // get current time
	MEM_READ (Z502TimerStatus, &Timer_Status);
	
	while(loop_wait_ReadyQueue_filled) {                                    // loop wait here for wating something to do      
		if (!IsQueueEmpty(ReadyQueueHead)) {                                // z502Idle() here throw error otherwise
			loop_wait_ReadyQueue_filled = FALSE;
		}
		//CALL(please_charge_hardware_time);									// recharge the time	
	}
	
	//if (PCB_Current_Running_Process != NULL) {
	//	process_printer("Wait",PCB_Current_Running_Process->process_id,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1);
	//}

	PCB_Transfer_Ready_To_Timer = DeQueue(&ReadyQueueHead)  ;								// Take from ready queue
	PCB_Transfer_Ready_To_Timer->wakeup_time = Time + (INT32) (*sleepTime);					// Set absolute time to PCB
	PCB_Transfer_Ready_To_Timer->state	= PROCESS_STATE_SLEEP;

	//printf("... start_timer current time:%d...\n",Time);
	//printf("... start_timer process: %s; sleep at %d then wakeup:%d...\n", PCB_Transfer_Ready_To_Timer->process_name, Time,PCB_Transfer_Ready_To_Timer->wakeup_time);

	CALL(AddToTimerQueue(&TimerQueueHead, PCB_Transfer_Ready_To_Timer) );					// then Add to Timer queue

	if (Timer_Status == DEVICE_FREE) {                                      // nothing set the timer
		MEM_WRITE( Z502TimerStart, sleepTime );								// go ahead and set timer
		//printf("...start_timer set timer hardware %d....\n", *sleepTime);
	}
	else if  (Timer_Status == DEVICE_IN_USE) {								// a process in front of set Timer before
		if (prev_Head->wakeup_time > PCB_Transfer_Ready_To_Timer->wakeup_time) {           // comopare time. Another way is compare pointer address

			new_update_time = (INT32*) malloc(sizeof(INT32));
			MEM_READ (Z502ClockStatus, &Time);
			*new_update_time = PCB_Transfer_Ready_To_Timer->wakeup_time - Time;				
			MEM_WRITE(Z502TimerStart, new_update_time);						// set timer based on new timer queue head
			//printf("...start_timer set timer hardware %d....\n", *new_update_time);
			free(new_update_time);
		}
		else {}																// do not hav eto change anything
	}
	

	//printf(" .... End start_timer TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... End start_timer Timer Queue! ...\n"); CALL(PrintQueue(TimerQueueHead) );
	//printf(" .... End start_timer ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... End start_timer Ready Queue! ...\n"); CALL( PrintQueue(ReadyQueueHead) );	
	process_printer("E_Sleep",-1,-1,1,1,-1,-1,-1,-1);
	//Z502Idle();															// run good from tes0- test1b
	dispatcher(&flag_switch_and_destroy);
	// transfer to dispatcher from test1c
}

void make_ready_to_run(ProcessControlBlock **head_of_ready, ProcessControlBlock *pcb) 
{
	AddToReadyQueue(&ReadyQueueHead,pcb);
}

void terminate_proccess_id(INT32 process_id, INT32* error_return) 
{
	INT32	Time;
	INT32	*new_start_timer;

	if ( !IsExistsProcessIDLinkedList(ListHead,  process_id) ) {							//invalid process_id
		*error_return = PROCESS_TERMINATE_INVALID_PROCESS_ID;
		return;
	}
	else {																					// remove from List
		ListHead = RemoveFromLinkedList(ListHead,process_id);
		*error_return = ERR_SUCCESS;														//success
		
		if (IsExistsProcessIDQueue(ReadyQueueHead,process_id) ) {
			CALL(RemoveProcessFromQueue(&ReadyQueueHead,process_id) );
		}
		if (IsExistsProcessIDQueue(TimerQueueHead,process_id) ) {
			CALL( RemoveProcessFromQueue(&TimerQueueHead,process_id) );						//remove from timer queue
			
			MEM_READ (Z502ClockStatus, &Time);	
			new_start_timer = (INT32*) malloc(sizeof(INT32));
			*new_start_timer = TimerQueueHead->wakeup_time - Time;
			if (*new_start_timer >= 0) {													// this new_start_timer always > 0, otherwise moved to Ready queue
				MEM_WRITE( Z502TimerStart, new_start_timer );
			}
			free(new_start_timer);
			
		}

		return;
		
	}
}

void set_pcb_current_running_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id) {
	PCB_Current_Running_Process = CreateProcessControlBlockWithData(process_name, starting_address,priority, *process_id) ;

}

void dispatcher(INT32 *destroy_context) {
	// not dequeue. Just take head then run
	// becasue when sleep, we take off from Ready --> Timer

	//printf(" .... Start dispatcher TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... Start dispatcher Timer Queue! ..."); CALL( PrintQueue(TimerQueueHead) );
	//printf(" .... Start dispatcher ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... Start dispatcher Ready Queue! ...\n"); CALL( PrintQueue(ReadyQueueHead) );	

	process_printer("S_Dispth",-1,-1,1,1,-1,-1,-1,-1);

	if (!IsQueueEmpty(ReadyQueueHead)) {
		if (PCB_Current_Running_Process != NULL) {									 // remove the previous data first 
			FreePCB(PCB_Current_Running_Process);									 // remember whenever FreePCB
			PCB_Current_Running_Process = NULL;										 // set to NULL
		}
		
			set_pcb_current_running_process(ReadyQueueHead->process_name,			//set current running process. Used in GET_PROCESS_ID
													ReadyQueueHead->context,
													ReadyQueueHead->priority,
													&ReadyQueueHead->process_id ) ;		
						
			if (*destroy_context == FALSE) {
				//if (PCB_Current_Running_Process != NULL) {													// at least one process created
				//	process_printer("Dispatch",PCB_Current_Running_Process->process_id,PCB_Current_Running_Process->process_id,1,1,-1,-1,-1,-1);
				//}
				Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &PCB_Current_Running_Process->context );		//switch context always by Current_Running_Process
			}
			else if (*destroy_context == TRUE)  {
				*destroy_context = FALSE;
				Z502SwitchContext(  SWITCH_CONTEXT_KILL_MODE, &PCB_Current_Running_Process->context );		//switch context always by Current_Running_Process
			}
			
		//	break;																	// just run one
		}
		else  {
			Z502Idle();
		}
	//}

	//printf(" .... End dispatcher TimerQueue Size: %d ... ",SizeQueue(TimerQueueHead)); printf(" .... End dispatcher Timer Queue! ...\n"); CALL( PrintQueue(TimerQueueHead) );
	//printf(" .... End dispatcher ReadyQueue Size: %d ... ",SizeQueue(ReadyQueueHead)); printf(" .... End dispatcher Ready Queue! ...\n"); CALL( PrintQueue(ReadyQueueHead) );	

	process_printer("E_Dispth",-1,-1,1,1,-1,-1,-1,-1);
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
	
	// ---------------
	// Print one value
	// ---------------
	if (target_process_id != -1)
		CALL(SP_setup( SP_TARGET_MODE, target_process_id) );

	if (running_process_id != -1)
		CALL(SP_setup( SP_RUNNING_MODE, running_process_id) );

	if (terminated_process_id != -1)
		CALL(SP_setup( SP_TERMINATED_MODE, terminated_process_id) );

	if (new_process_id != -1)
		CALL(SP_setup( SP_NEW_MODE, new_process_id) );

	// ------------------
	// Print queue, table
	// ------------------
	
	if (waiting_mode != -1) {
		if (!IsQueueEmpty(TimerQueueHead)) {							// print only queue is not blank
			while (timerQueue != NULL) {
				CALL( SP_setup( SP_WAITING_MODE,timerQueue->process_id) );
				timerQueue = timerQueue->nextPCB;
			}
		}
	}
	
	if ( ready_mode != -1) {
		if (!IsQueueEmpty(ReadyQueueHead)) {							// print only queue is not blank
			while (readyQueue != NULL) {
				CALL ( SP_setup( SP_READY_MODE,readyQueue->process_id) );
				readyQueue = readyQueue->nextPCB;
			}
		}
	}

	if (swapped_mode != -1) {}
	
	if (suspended_mode != -1) {}

	printf("\n-------------------------scheduler_printer Output------------------------\n");
    CALL( SP_print_header() );
    CALL( SP_print_line() );
    printf("-------------------------------------------------------------------------\n\n");


}