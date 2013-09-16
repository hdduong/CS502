/*********************************************************************
	This incude file is using by hdduong for Z502
	Sep/08/13:		Created

*********************************************************************/
#include "global.h"

typedef struct StructProcessControlBlock
{
	INT32				process_id;
	char				*process_name;
	INT32				priority;
	void				*context;								//pointer to context for the process
	struct				StructProcessControlBlock				* nextPCB; 
} ProcessControlBlock;


ProcessControlBlock		*CreateProcessControlBlock();
ProcessControlBlock		*CreateProcessControlBlockWithData(char *process_name, void *starting_address, INT32 priority , INT32 process_id);
BOOL					IsTimerQueueEmpty(ProcessControlBlock *head, ProcessControlBlock *tail);
void					AddToTimerQueue(ProcessControlBlock **head, ProcessControlBlock **tail, ProcessControlBlock *pcb);
void					RemoveFromTimerQueue(ProcessControlBlock **head, ProcessControlBlock **tail, ProcessControlBlock *removePCB);
INT16					SizeTimerQueue(ProcessControlBlock *head, ProcessControlBlock *tail);
void					PrintTimerQueue(ProcessControlBlock *head,ProcessControlBlock *tail);
void					FreePCB(ProcessControlBlock *pcb);

///* Linked List */
ProcessControlBlock		*InsertLinkedListPID(ProcessControlBlock *head, ProcessControlBlock *pcb);
BOOL					IsExistsProcessNameLinkedList(ProcessControlBlock *head, char *process_name);
BOOL					IsExistsProcessIDLinkedList(ProcessControlBlock * head, INT32 process_id);
ProcessControlBlock		*RemoveFromLinkedList(ProcessControlBlock *head, INT32 process_id);
ProcessControlBlock		*RemoveLinkedList(ProcessControlBlock *head);