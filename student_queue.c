#include				 <stdio.h>
#include                 <stdlib.h>
#include                 <memory.h>

#include				"student_queue.h"

/*********************************************************************
	This is the file implement queue used by hdduong
	Sep/08/13:		Created
*********************************************************************/


ProcessControlBlock *CreateProcessControlBlock()
{
	ProcessControlBlock *pcb; 
	pcb  = (ProcessControlBlock *)malloc(sizeof(ProcessControlBlock));
	if (pcb == NULL)
	{
			printf("@CreateProcessControlBlock: We didn't complete the malloc.\n");
			return NULL;
	}
	pcb->nextPCB = NULL;
	pcb->context = NULL;
	return pcb;
}



BOOL IsTimerQueueEmpty(ProcessControlBlock *head, ProcessControlBlock *tail)
{
	if ( (head == NULL) && (tail == NULL) ) 
	{
		return TRUE;
	}
	return FALSE;
}


void AddToTimerQueue(ProcessControlBlock **head, ProcessControlBlock **tail, ProcessControlBlock *pcb)
{
	if (IsTimerQueueEmpty(*head,*tail)) 
	{
		*head = *tail = pcb;
		(*head)->nextPCB = NULL;
		return;
	}
	else 
	{
		(*tail)->nextPCB = pcb;
		*tail = pcb;
	}
}



void RemoveFromTimerQueue(ProcessControlBlock **head, ProcessControlBlock **tail, ProcessControlBlock *removePCB)
{
	if (IsTimerQueueEmpty(*head,*tail)) 
	{
		return;
	}
	else if ( (*head == *tail) && (*head == removePCB)	) {				//head and tail not NULL. Only one element 
		*head = *tail = NULL;
		FreeFromTimerQueue(removePCB);
		return;
	}
	else { 
		// fix there
		ProcessControlBlock *tmp = *head;
		ProcessControlBlock *prev = NULL;

		while (tmp != NULL) {
			if (tmp != removePCB) {
				prev = tmp;
				tmp = tmp->nextPCB;
			}
			else {                                         // found removePCB
				prev->nextPCB = tmp->nextPCB;
				FreeFromTimerQueue(removePCB);
			}
		}
		return;
	}
	// should not never get there
	printf("@RemoveFromTimerQueue: Something wrong. \n");
	return;
}

INT16 SizeTimerQueue(ProcessControlBlock *head, ProcessControlBlock *tail)
{
	int count = 0;
	if (IsTimerQueueEmpty(head,tail))
	{
		return 0;
	}
	else 
	{
		count++;
		while (head != tail)
		{
			count++;
			head = head->nextPCB;
		}
	}
	return count;
}


void PrintTimerQueue(ProcessControlBlock *head,ProcessControlBlock *tail)
{
	if (IsTimerQueueEmpty(head,tail))
	{
		//printf("@PrintPCBQueue: NULL \n");
		return;
	}
	else
	{
		//printf("@PrintPCBQueue: %d ",head->processId);
		while (head != tail)
		{
			head = head->nextPCB;
			printf("%d ",head->processId);
		}
		printf("\n");
	}

}

void FreeFromTimerQueue(ProcessControlBlock *pcb)
{
	if (pcb != NULL)
		free(pcb);
}