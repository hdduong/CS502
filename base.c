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

#include			 "student_queue.h"


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

ProcessControlBlock *TimerQueueHead, *TimerQueueTail; // TimerQueue 
ProcessControlBlock *PCB_Current_Process;


/**************************** <hdduong ********************************/
void	os_make_process(char* testCase);
void	start_timer(long * sleepTime);

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

	printf(" .... Start interrupt_handler! ...\n");
	printf(" .... Queue Size: %d ... \n",SizeTimerQueue(TimerQueueHead,TimerQueueTail));


    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );

    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );

	// Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

	if ( device_id == TIMER_INTERRUPT && status == ERR_SUCCESS ) {
		RemoveFromTimerQueue(&TimerQueueHead, &TimerQueueTail,PCB_Current_Process);
		printf(" .... Queue Size: %d ... \n",SizeTimerQueue(TimerQueueHead,TimerQueueTail));
	}
    
	// Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );

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

	long				SleepTime;

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
       
		// System sleep test 1a
		case SYSNUM_SLEEP:
			SleepTime = SystemCallData->Argument[0];
			//SleepTime = *(INT32 *)SystemCallData->Argument[0]; /* hdduong-ErrorWhy */
			start_timer(&SleepTime);
			break;

		// terminate system call
		case SYSNUM_TERMINATE_PROCESS:
            CALL(Z502Halt());
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
        test0 runs on a process recognized by the operating system.    */
	/*
    Z502MakeContext( &next_context, (void *)test0, USER_MODE );
    Z502SwitchContext( SWITCH_CONTEXT_KILL_MODE, &next_context );
	*/

	/* Start of test1a */
	/*
    Z502MakeContext( &next_context, (void *)test1a, USER_MODE );
    Z502SwitchContext( SWITCH_CONTEXT_KILL_MODE, &next_context );
	*/
	//os_make_process("test0");
	os_make_process("test1a");
}                                               // End of osInit


void os_make_process(char *testCase) 
{
	 void *next_context;
	 if (strcmp(testCase,"test0") == 0) {
		Z502MakeContext( &next_context, (void *)test0, USER_MODE );
		Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );
	 }

	 else if (strcmp(testCase,"test1a") == 0) {
		 PCB_Current_Process = CreateProcessControlBlock(); // create the PCB
		 PCB_Current_Process->context = (void*)test1a; // fill in data for PCB

		 Z502MakeContext( &next_context, (void*)test1a, USER_MODE ); //ask the hardware for a context
		 Z502SwitchContext( SWITCH_CONTEXT_SAVE_MODE, &next_context );
	 }
}


void start_timer(long * sleepTime) 
{
	//printf( " .... Start start_timer! ....\n");
	//pcb_Current_Process->context = (void*)start_timer;

	AddToTimerQueue(&TimerQueueHead,&TimerQueueTail,PCB_Current_Process);
	CALL(MEM_WRITE( Z502TimerStart, &*sleepTime));
	//printf( " .... Start Z502Idle! ....\n");
	Z502Idle();
	//printf( " .... End Z502Idle! ....\n");
	//printf( " .... End start_timer! ....\n");
}
