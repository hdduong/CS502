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
ProcessControlBlock			*TimerQueueHead;														// TimerQueue 
ProcessControlBlock			*ReadyQueueHead;														// ReadyQueue
ProcessControlBlock			*PCB_Transfer;															// point to current running PCB
ProcessControlBlock			*ListHead;																// PCB table


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 process_id, INT32 *error);
INT32	check_legal_process(char* process_name, INT32 initial_priority);							// check Legal process before create process
void	start_timer(long * sleepTime);
void	make_ready_to_run(ProcessControlBlock **ReadyQueueHead, ProcessControlBlock *pcb);			// Insert into readque

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
	ProcessControlBlock	*tmp = TimerQueueHead;								// check front of time queue head

	printf(" .... Start interrupt_handler! ...\n");
	printf(" .... TimerQueue Size: %d ... \n",SizeQueue(TimerQueueHead));
	printf(" .... ReadyQueue Size: %d ... \n",SizeQueue(ReadyQueueHead));


    // Get cause of interrupt
    CALL( MEM_READ(Z502InterruptDevice, &device_id ) );

    // Set this device as target of our query
    CALL ( MEM_WRITE(Z502InterruptDevice, &device_id ) );

	// Now read the status of this device
    CALL( MEM_READ(Z502InterruptStatus, &status ) );

	if ( device_id == TIMER_INTERRUPT && status == ERR_SUCCESS ) {
		MEM_READ(Z502ClockStatus, &Time);									// read current time and compare to front of Timer queue
							
		while ( (tmp!= NULL) && (tmp->wakeup_time <= Time) ) {
			PCB_Transfer = DeQueue(&TimerQueueHead);						// take off front Timer queue
			CALL( make_ready_to_run(&ReadyQueueHead, PCB_Transfer) );		// put in the end of Ready Queue
			tmp = TimerQueueHead;											// transfer until front Timer Queue > current time
		}
	}

	// Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
	
	printf(" .... TimerQueue Size: %d ... \n",SizeQueue(TimerQueueHead));
	printf(" .... ReadyQueue Size: %d ... \n",SizeQueue(ReadyQueueHead));
	printf( " ... End interrupt_handler! ...\n");

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
    static INT16        do_print = 10;
    INT32               Time;
	short               i;
	INT32				create_priority;			 // read argument 2 - prioirty of CREATE_PROCESS
	long				SleepTime;					 // using in SYSNUM_SLEEP
	ProcessControlBlock *newPCB;					 // create new process
	INT32				terminate_arg;				 // terminate argument of TERMINATE_PROCESS
	char				*process_name_arg = NULL;	 // process name argument of GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_error_arg;			// process error check for GET_PROCESS_ID, CREATE_PROCESS
	INT32				process_id_arg;				 // process_id returns in GET_PROCESS_ID, CREATE_PROCESS
	//INT32				MAX_EXCEED_PROCESSES = 1000; // Maximum created processes
	void				*starting_address;			 // starting address of rountine in CREATE_PROCESS


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
            CALL(MEM_READ( Z502ClockStatus, &Time ));
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

			if (terminate_arg == -1) {									//terminate self
				//Time
				global_process_id--;
				DeleteQueue(TimerQueueHead);							// release all heap memory before quit
				DeleteQueue(ReadyQueueHead);
				CALL(Z502Halt());
				
			}
			else if (terminate_arg == -2) {								//terminate self + children
				// global_process_id
				DeleteQueue(TimerQueueHead);							// release all heap memory before quit
				DeleteQueue(ReadyQueueHead);
				CALL(Z502Halt());
			}
			else  {														// terminate a process
				// test1b remove from LIST
				if ( !IsExistsProcessIDLinkedList(ListHead,  terminate_arg) ) {    //invalid process_id
					*(INT32 *)SystemCallData->Argument[1] = PROCESS_TERMINATE_INVALID_PROCESS_ID;
				}
				else {													 // remove from List
					ListHead = RemoveFromLinkedList(ListHead,terminate_arg);
					*(INT32 *)SystemCallData->Argument[1] = ERR_SUCCESS;  //success
					global_process_id--;
					return;
				}
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
				//*(INT32 *)SystemCallData->Argument[1] = PCB_Current_Process->process_id;				//get data from Current_Process_PCB 
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
		CALL( os_create_process("test1a",(void*) test1b,test_case_prioirty, &created_process_id, &created_process_error) );
	}


	//----------------------------------------------------------//
	//				Run test manuallly			    			//
	//----------------------------------------------------------//

	//CALL(os_create_process("test0",(void*) test0,test_case_prioirty, &created_process_id, &created_process_error));
	//Z502MakeContext( &next_context, (void*) test0, USER_MODE );	
	//Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );

	CALL( os_create_process("test1a",(void*) test1a,test_case_prioirty, &created_process_id, &created_process_error) );
	Z502MakeContext( &next_context, (void*) test1a, USER_MODE );	
	Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );

	/*CALL ( os_create_process("test1b",(void*) test1b,test_case_prioirty, &created_process_id, &created_process_error) );
	Z502MakeContext( &next_context, (void*) test1b, USER_MODE );	
	Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );*/

}                                               // End of osInit


void	os_create_process(char* process_name, void	*starting_address, INT32 priority, INT32 *process_id, INT32 *error)
{
	 void						*next_context;
	 INT32						legal_process;
	 ProcessControlBlock		*newPCB;
	 

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
	ListHead = InsertLinkedListPID(ListHead,newPCB);										// insert new pcb to pcb table

	make_ready_to_run(&ReadyQueueHead,newPCB);												// whenever new process created, put into ready queue
	
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

void start_timer(long *sleepTime) 
{
	INT32		Time;														// get current time assigned to 
	INT32		Timer_Status;
	ProcessControlBlock *prev_Head = TimerQueueHead;						// to get timer of HeadTimerQueue

	MEM_READ (Z502ClockStatus, &Time);								        // get current time
	MEM_READ (Z502TimerStatus, &Timer_Status);
	
	PCB_Transfer = DeQueue(&ReadyQueueHead)  ;								// Take from ready queue
	PCB_Transfer->wakeup_time = Time + (INT32) *sleepTime;					// Set absolute time to PCB
	PCB_Transfer->state	= PROCESS_STATE_SLEEP;

	CALL(AddToTimerQueue(&TimerQueueHead, PCB_Transfer) );					// then Add to Timer queue

	if (Timer_Status == DEVICE_FREE) {                                      // nothing set the timer
		MEM_WRITE( Z502TimerStart, &*sleepTime );							// go ahead and set timer
	}
	else if  (Timer_Status == DEVICE_IN_USE) {								// a process in front of set Timer before
		if (prev_Head->wakeup_time > PCB_Transfer->wakeup_time) {           // comopare time. Another way is compare pointer address
			MEM_WRITE( Z502TimerStart, &*sleepTime );
		}
		else {}																// do not hav eto change anything
	}
	
		
	Z502Idle();
	
}

void make_ready_to_run(ProcessControlBlock **head_of_ready, ProcessControlBlock *pcb) {
	pcb->state = PROCESS_STATE_READY;
	AddToReadyQueue(&ReadyQueueHead,pcb);
}