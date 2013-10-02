#include				 <stdio.h>
#include                 <stdlib.h>
#include                 <memory.h>
#include				 <string.h>

#include				"student_queue.h"
#include				"student_global.h"


/***********************************************************************
	This is the file implement queue used by hdduong
	Sep/08/13:		Created
	Sep/16/13:		Update List
	Sep/29/13		Version z502_v4: change PCB Table from List to Array
***********************************************************************/

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
	while ( (tmp != NULL) && (tmp->wakeup_time <= pcb->wakeup_time)) {           // <= so prev always has value
		prev = tmp;
		tmp = tmp->nextPCB;
	}
	prev->nextPCB = pcb;								// insert in the Middle & End
	pcb->nextPCB = NULL;								// reset linker

	if (tmp != NULL) {									// insert in the Middle
		pcb->nextPCB = tmp;
		return;
	}

}

// Oct 2: Add with priority

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

	if ( pcb->priority < (*head)->priority  )	{		// infront of queue
		pcb->nextPCB = *head;
		*head = pcb;
		return;
	}


	tmp = *head;
	while ( (tmp != NULL) && (tmp->priority <= pcb->priority) ) {
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
	*head = (*head)->nextPCB;
	tmp->nextPCB = NULL;								// release next pointer, go to anothe queue

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
			printf("%d ",head->process_id);
			head = head->nextPCB;
		}
		printf(";\n");
	}

}

void FreePCB(ProcessControlBlock *pcb)
{
	if (pcb != NULL) {
		pcb->nextPCB = NULL;
		free(pcb->process_name);
		free(pcb);
	}
	pcb = NULL;
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


BOOL	IsExistsProcessIDQueue(ProcessControlBlock *head, INT32 process_id)
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


void	RemoveProcessFromQueue(ProcessControlBlock **head, INT32 process_id) 
{
	// assume that process_id already exists
	ProcessControlBlock *prev = NULL;
	ProcessControlBlock *tmp = *head;

	if (tmp->process_id == process_id) {											// remove the head
		*head = (*head)->nextPCB;
		//FreePCB(tmp);
		return;
	}

	while ( (tmp != NULL) && (tmp->process_id != process_id) ) {
		prev = tmp;
		tmp = tmp->nextPCB;
	}

	prev->nextPCB = tmp->nextPCB;
	//FreePCB(tmp);
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

void	RemoveLinkedList(ProcessControlBlock *head) 
{
	ProcessControlBlock *tmp = head;
	while(head!=NULL) {
		head = head->nextPCB;
		FreePCB(tmp);
	}

	return;
}

//------------------------------------------------------------------//
//			       Array PCB Table									//
//------------------------------------------------------------------//

BOOL	IsExistsProcessNameArray(ProcessControlBlock *head[], char* process_name, INT32 number_of_processes)
{
	INT32	count = 0;

	//if (head[0] == NULL)
	//	return FALSE;

	while (count <number_of_processes) {
		if ( (head[count] != NULL) 
			&& ( strcmp(head[count]->process_name,process_name) == 0) 
			&& (head[count]->state != PROCESS_STATE_TERMINATE) ) {
			return TRUE;
		}
		count++;

	}

	return FALSE;
}



BOOL	IsExistsProcessIDArray(ProcessControlBlock *head[], INT32 process_id, INT32 num_processes)
{
	INT32	count = 0;

	ProcessControlBlock *tmp = head[0];
	if (head == NULL)
		return FALSE;

	while ( count < num_processes )   {
		if ( (head[count] != NULL) && (head[count]->process_id == process_id) ) {
			break;	
		}
		count++;
	}      

	if (head[count] == NULL) 
		return FALSE;

	return TRUE;
}


void RemoveFromArray(ProcessControlBlock *head[], INT32 process_id, INT32 num_processes)   //will move to TerminateList later
{
	INT32	count = 0;
	

	if  (head[0] == NULL)
		return;
	
	
	
	while ( count < num_processes )   {
		if ( (head[count] != NULL) && (head[count]->process_id == process_id) ) {
			break;	
		}
		count++;
	}                                  
	
	if (head[count] != NULL) {					//found
		//FreePCB(head[count]);
		head[count]->state = PROCESS_STATE_TERMINATE;
	}
	
	return;
}





INT32 GetProcessID(ProcessControlBlock *head[], char* process_name, INT32 num_processes) {
	
	INT32	count = 0;

	while (count < num_processes) {
		if ( (head[count] != NULL) 
			&& ( strcmp(head[count]->process_name,process_name) == 0) 
			&& (head[count]->state != PROCESS_STATE_TERMINATE) ) {
			return TRUE;
		}
		count++;

	}  

	if (head[count] == NULL) {												// process id is not exist
		
		return PROCESS_ID_NOT_VALID;													// -1: no process id found
	}

	return head[count] ->process_id;

}


INT32 CountActiveProcesses(ProcessControlBlock *head[], INT32 num_processes) {
	INT32	count = 0;
	INT32	index = 0;

	while (index < num_processes) {
		if ( (head[index] != NULL) 
			&& (head[index]->state != PROCESS_STATE_TERMINATE) ) 
			count++;
		index++;
	}  

	return count;
}
