/*********************************************************************
	This incude file is using by hdduong for Z502
	Sep/08/13:		Created

*********************************************************************/
#include "global.h"

typedef struct StructProcessControlBlock
{
	long				processId;
    long                reg1, reg2, reg3, reg4, reg5;
    long                reg6, reg7, reg8, reg9;
	struct				StructProcessControlBlock				* nextPCB; 
	void				*context; //pointer to context for the process
	UINT16				*page_table_ptr;     // Location of the page table
	INT16				page_table_len;    // Length of the page table
} ProcessControlBlock;


ProcessControlBlock		*CreateProcessControlBlock();
BOOL					IsTimerQueueEmpty(ProcessControlBlock *, ProcessControlBlock *);
void					AddToTimerQueue(ProcessControlBlock **, ProcessControlBlock **, ProcessControlBlock *);
void					RemoveFromTimerQueue(ProcessControlBlock **, ProcessControlBlock **, ProcessControlBlock *);
INT16					SizeTimerQueue(ProcessControlBlock *, ProcessControlBlock *);
void					PrintTimerQueue(ProcessControlBlock *,ProcessControlBlock *);
void					FreeFromTimerQueue(ProcessControlBlock *);

