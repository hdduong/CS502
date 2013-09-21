#include				 <stdio.h>
#include                 <stdlib.h>
#include                 <memory.h>
#include				 <string.h>

#include				"student_queue.h"
#include				"student_global.h"


/*********************************************************************
	This is the file implement queue used by hdduong
	Sep/08/13:		Created
	Sep/16/13:		Update List
*********************************************************************/

//------------------------------------------------------------------//
//					TIMER QUEUE										//
//------------------------------------------------------------------//


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




BOOL IsQueueEmpty(ProcessControlBlock *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}


void AddToTimerQueue(ProcessControlBlock **head, ProcessControlBlock *pcb)
{
	ProcessControlBlock *tmp;
	ProcessControlBlock *prev;

	if (IsQueueEmpty(*head)) 
	{
		*head = pcb;
		(*head)->nextPCB = NULL;
		return;
	}		

	// add by increasing of time
	if ( (*head)->wakeup_time > pcb->wakeup_time) {     //in front
		pcb->nextPCB = *head;
		*head = pcb;
		return;	
	}	

	tmp = *head;
	while ( (tmp != NULL) && (tmp->wakeup_time < pcb->wakeup_time)) {
		prev = tmp;
		tmp = tmp->nextPCB;
	}
	prev->nextPCB = pcb;								// insert in the Middle & End

	if (tmp != NULL) {									// insert in the Middle
		pcb->nextPCB = tmp;
		return;
	}

}


void AddToReadyQueue(ProcessControlBlock **head, ProcessControlBlock *pcb)
{
	ProcessControlBlock *tmp;
	ProcessControlBlock *prev;
	
	if (IsQueueEmpty(*head)) 
	{
		*head = pcb;
		(*head)->nextPCB = NULL;
		return;
	}		

	tmp = *head;
	while (tmp != NULL) {
		prev = tmp;
		tmp = tmp->nextPCB;
	}
	prev->nextPCB = pcb;								// insert in the Middle & End	
	pcb->nextPCB = tmp;

}


ProcessControlBlock *DeQueue(ProcessControlBlock **head)
{
	ProcessControlBlock *tmp;

	if (IsQueueEmpty(*head)) 
	{
		return NULL;
	}
	tmp = *head;
	tmp->nextPCB = NULL;								// release next pointer, go to anothe queue
	*head = (*head)->nextPCB;

	return tmp;
}



INT32 SizeQueue(ProcessControlBlock *head)
{
	int count = 0;
	if (IsQueueEmpty(head))
	{
		return 0;
	}
	else 
	{
		while (head != NULL)
		{
			count++;
			head = head->nextPCB;
		}
	}
	return count;
}


void PrintQueue(ProcessControlBlock *head)
{
	if (IsQueueEmpty(head))
	{
		//printf("@printpcbqueue: null \n");
		return;
	}
	else
	{
		//printf("@printpcbqueue: %d ",head->processid);
		while (head != NULL)
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


void DeleteQueue(ProcessControlBlock *head) 
{
	ProcessControlBlock *tmp = head;
	while(head != NULL) {
		head = head->nextPCB;
		FreePCB(tmp);
		tmp = head;
	}
}

//------------------------------------------------------------------//
//			       LINKED LIST PCB Table							//
//------------------------------------------------------------------//


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
		return NULL;
	
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
	return head;
	
}


void	RemoveLinkedList(ProcessControlBlock *head) 
{
	ProcessControlBlock *tmp = head;
	while(head!=NULL) {
		head = head->nextPCB;
		FreePCB(tmp);
	}

	return;
}


INT32 GetProcessID(ProcessControlBlock *ListHead, char* process_name) {
	 /* assume that "" processed in base.c. Only care process_name != "" */

	ProcessControlBlock *tmp = ListHead;
	
	while( (tmp != NULL) && (strcmp(tmp->process_name,process_name) != 0) ) {
		tmp = tmp->nextPCB;
	}

	if (tmp == NULL) {												// process id is not exist
		
		return PROCESS_ID_NOT_VALID;													// -1: no process id found
	}

	return tmp->process_id;

}


INT32 SizeLinkedListPID(ProcessControlBlock *ListHead) {
	 /* assume that "" processed in base.c. Only care process_name != "" */

	INT32				count = 0;
	ProcessControlBlock *tmp = ListHead;
	
	while( tmp != NULL ) {
		tmp = tmp->nextPCB;
		count++;
	}

	return count;
}