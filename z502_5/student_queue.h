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
	INT32				state;									// state of the process
	INT32				wakeup_time;							// time to wake up
	struct				StructProcessControlBlock				* nextPCB; 
} ProcessControlBlock;


ProcessControlBlock		*CreateProcessControlBlockWithData(char *process_name, void *starting_address, INT32 priority , INT32 process_id);
BOOL					IsQueueEmpty(ProcessControlBlock *head);
void					AddToTimerQueue(ProcessControlBlock **head, ProcessControlBlock *pcb);
void					AddToReadyQueue(ProcessControlBlock **head, ProcessControlBlock *pcb);
ProcessControlBlock		*DeQueue(ProcessControlBlock **head);
INT32					SizeQueue(ProcessControlBlock *head);
void					PrintQueue(ProcessControlBlock *head);
void					FreePCB(ProcessControlBlock *pcb);
void					DeleteQueue(ProcessControlBlock *head);
BOOL					IsExistsProcessIDQueue(ProcessControlBlock *head, INT32 process_id);
void					RemoveProcessFromQueue(ProcessControlBlock **head, INT32 process_id); 

///* Linked List */
ProcessControlBlock		*InsertLinkedListPID(ProcessControlBlock *head, ProcessControlBlock *pcb);
BOOL					IsExistsProcessNameArray(ProcessControlBlock *head[], char *process_name, INT32 number_of_processes);
BOOL					IsExistsProcessIDArray(ProcessControlBlock *head[], INT32 process_id,INT32 number_of_processes);
void					RemoveFromArray(ProcessControlBlock *head[], INT32 process_id, INT32 number_of_processes);
void					RemoveLinkedList(ProcessControlBlock *head);
INT32					GetProcessID(ProcessControlBlock *Head[], char* process_name, INT32 num_processes);
INT32					CountActiveProcesses(ProcessControlBlock *head[], INT32 num_processes);