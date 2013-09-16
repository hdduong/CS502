#include				 <stdio.h>
#include                 <stdlib.h>
#include                 <memory.h>
#include				 <string.h>

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

	
ProcessControlBlock *CreateProcessControlBlockWithData(char *process_name, void *starting_address, INT32 priority, INT32 process_id)
{
	ProcessControlBlock *pcb;

	pcb = (ProcessControlBlock*) malloc(sizeof(ProcessControlBlock));
	if (pcb == NULL) return NULL;

	pcb->process_name = strdup(process_name);
	if (pcb->process_name == NULL) return NULL;

	pcb->context = starting_address;
	pcb->priority = priority;
	pcb->process_id = process_id;
	pcb->nextPCB = NULL;

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



void RemoveFromTimerQueue(ProcessControlBlock **head, ProcessControlBlock **tail, ProcessControlBlock *removepcb)
{
	if (IsTimerQueueEmpty(*head,*tail)) 
	{
		return;
	}
	else if ( (*head == *tail) && (*head == removepcb)	) {				//head and tail not null. only one element 
		*head = *tail = NULL;
		FreePCB(removepcb);
		return;
	}
	else { 
		// fix there
		ProcessControlBlock *tmp = *head;
		ProcessControlBlock *prev = NULL;

		while (tmp != NULL) {
			if (tmp != removepcb) {
				prev = tmp;
				tmp = tmp->nextPCB;
			}
			else {                                         // found removepcb
				prev->nextPCB = tmp->nextPCB;
				FreePCB(removepcb);
			}
		}
		return;
	}
	// should not never get there
	printf("@removefromtimerqueue: something wrong. \n");
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
		//printf("@printpcbqueue: null \n");
		return;
	}
	else
	{
		//printf("@printpcbqueue: %d ",head->processid);
		while (head != tail)
		{
			head = head->nextPCB;
			printf("%d ",head->process_id);
		}
		printf("\n");
	}

}

void FreePCB(ProcessControlBlock *pcb)
{
	if (pcb != NULL) {
		free(pcb->process_name);
		free(pcb);
	}
}


//
////-----------------------------------------------------//
//
//// Insert by increase of PID
ProcessControlBlock		*InsertLinkedListPID(ProcessControlBlock *head, ProcessControlBlock *pcb) 
{
	ProcessControlBlock *tmp = NULL;
	ProcessControlBlock *prev = NULL;

	if (head == NULL)
		return pcb;
	
	if (pcb->process_id < head->process_id) {
		pcb->nextPCB = head;
		return pcb;
	}

	tmp = head;
	while ( (tmp != NULL) && (tmp->process_id < pcb->process_id) ) {
		prev = tmp;
		tmp = tmp->nextPCB;
	}

	prev->nextPCB = pcb;
	pcb->nextPCB = tmp;

	return head;
}

BOOL	IsExistsProcessNameLinkedList(ProcessControlBlock *head, char* process_name)
{
	ProcessControlBlock *tmp = head;
	if (head == NULL)
		return FALSE;

	while (tmp!= NULL) {
		if (strcmp(tmp->process_name, process_name) == 0)
			return TRUE;

		tmp = tmp->nextPCB;
	}

	return FALSE;
}



BOOL	IsExistsProcessIDLinkedList(ProcessControlBlock *head, INT32 process_id)
{
	ProcessControlBlock *tmp = head;
	if (head == NULL)
		return FALSE;

	while (tmp!= NULL) {
		if (tmp->process_id == process_id)
			return TRUE;

		tmp = tmp->nextPCB;
	}

	return FALSE;
}


// assume that process_id exists
ProcessControlBlock *RemoveFromLinkedList(ProcessControlBlock *head, INT32 process_id)
{
	ProcessControlBlock *tmp = head;
	ProcessControlBlock *prev = NULL;

	if  (head == NULL)
		return;
	
	if (head->process_id == process_id) {                            //remove at head
		FreePCB(head);
		return NULL;
	}
	
	while ( (tmp != NULL) &&   (tmp->process_id != process_id) )  {
				prev = tmp;
				tmp = tmp->nextPCB;
	}                                  
	
	if (tmp != NULL) {
		prev->nextPCB = tmp->nextPCB;
		FreePCB(tmp);
	}
	return;
	
}


ProcessControlBlock * RemoveLinkedList(ProcessControlBlock *head) 
{
	ProcessControlBlock *tmp = head;
	while(head!=NULL) {
		head = head->nextPCB;
		FreePCB(tmp);
	}

	return NULL;
}