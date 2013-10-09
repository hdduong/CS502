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

	pcb->sentBoxQueue = NULL;
	pcb->inboxQueue = NULL;

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



void AddToReadyQueueNotPriority(ProcessControlBlock **head, ProcessControlBlock *pcb)
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


ProcessControlBlock	*PullProcessFromQueue(ProcessControlBlock **head, INT32 process_id) 
{
	// assume that process_id already exists

	ProcessControlBlock *prev = NULL;
	ProcessControlBlock *tmp = *head;

	if (tmp->process_id == process_id) {											// remove the head
		*head = (*head)->nextPCB;
		//FreePCB(tmp);
		return tmp;
	}

	while ( (tmp != NULL) && (tmp->process_id != process_id) ) {
		prev = tmp;
		tmp = tmp->nextPCB;
	}

	prev->nextPCB = tmp->nextPCB;
	return tmp;
}

void	UpdateProcessPriorityQueue(ProcessControlBlock **head, INT32 process_id,INT32 new_priority) 
{
	// assume that process_id already exists

	ProcessControlBlock *tmp = *head;

	while ( (tmp != NULL) && (tmp->process_id != process_id) ) {
		tmp = tmp->nextPCB;
	}

	if (tmp !=NULL) {												// make sure exists
		tmp->priority = new_priority;
		return;
	}
}


//------------------------------------------------------------------//
//			       LINKED LIST Suspend								//
//------------------------------------------------------------------//
BOOL	IsExistsProcessIDList(ProcessControlBlock *head, INT32 process_id)
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


BOOL IsListEmpty(ProcessControlBlock *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}



void AddToSuspendList(ProcessControlBlock **head, ProcessControlBlock *pcb)
{

	if (IsListEmpty(*head)) 
	{
		*head = pcb;
		(*head)->nextPCB = NULL;
		return;
	}		

	pcb->nextPCB = *head;
	*head = pcb;

}


void AddToMsgSuspendList(ProcessControlBlock **head, ProcessControlBlock *pcb)
{

	if (IsListEmpty(*head)) 
	{
		*head = pcb;
		(*head)->nextPCB = NULL;
		return;
	}		

	pcb->nextPCB = *head;
	*head = pcb;

}



ProcessControlBlock *PullFromSuspendList(ProcessControlBlock **head, INT32 process_id)
{
	ProcessControlBlock *tmp = NULL;
	ProcessControlBlock *prev = NULL;

	if (IsListEmpty(*head)) 
	{
		return NULL;
	}		

	tmp = *head;
	while (tmp != NULL) {
		if (tmp->process_id == process_id) {
			if (prev == NULL) {											// remove head
				(*head) = (*head)->nextPCB;
				return tmp;
			}
			prev->nextPCB = tmp->nextPCB;
			return tmp;
		}
		prev = tmp;
		tmp = tmp->nextPCB;
	}

	return NULL;
}


//------------------------------------------------------------------//
//			       LINKED LIST PCB Table: Obsolete					//
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
	
	INT32	count = 0;														// pcb_table starts at 0

	while (count < num_processes) {
		if ( (head[count] != NULL) 
			&& ( strcmp(head[count]->process_name,process_name) == 0) 
			&& (head[count]->state != PROCESS_STATE_TERMINATE) ) {
			break;
		}
		count++;

	}  

	if (head[count] == NULL) {												// process id is not exist
		
		return PROCESS_ID_NOT_VALID;													// -1: no process id found
	}

	return head[count]->process_id;

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

/* check to better printout */

BOOL IsKilledProcess(ProcessControlBlock *head[], INT32 process_id, INT32 num_processes) {
	INT32	index = 0;

	while (index < num_processes) {
		if ( (head[index] != NULL)  && (head[index]->process_id == process_id)
			&& (head[index]->state == PROCESS_STATE_TERMINATE) ) 
			return TRUE;
		index++;
	}  

	return FALSE;
}

//------------------------------------------------------------------//
//			       Message Queue        							//
//------------------------------------------------------------------//

Message	*CreateMessage(INT32 msg_id, INT32 target_id, INT32 source_id, INT32 send_length, INT32 actual_msg_length, char *msg_buffer, BOOL is_broadcast)
{
	Message *msg;

	msg = (Message*) malloc(sizeof(Message));
	if (msg == NULL) return NULL;

	msg->msg_id = msg_id;
	msg->target_id = target_id;
	msg->source_id = source_id;
	strcpy(msg->msg_buffer, msg_buffer);
	msg->send_length = send_length;
	msg->actual_msg_length = actual_msg_length;
	msg->is_broadcast = is_broadcast;
	msg->nextMsg = NULL;
	return msg;

}


// add message via PCB_Table otherwise have to loop seperately in ReadyQueue, TimerQueue and SuspendQueue

void	AddToSentBox(ProcessControlBlock *PCB_Table[], INT32 process_id, Message *msg,  INT32 number_of_processes) {

	INT32				index = 0;
	ProcessControlBlock *tmp;
	Message*			add_message;

	while (index <number_of_processes) {
		if ( PCB_Table[index] != NULL)  {
			if (PCB_Table[index]->process_id == process_id ) {
				if (!IsExistsMessageIDQueue(PCB_Table[index]->sentBoxQueue,msg->msg_id) ) {
					add_message = CreateMessage(msg->msg_id,msg->target_id, msg->source_id, msg->send_length,msg->actual_msg_length,msg->msg_buffer,msg->is_broadcast);
					AddToMsgQueue(& (PCB_Table[index]->sentBoxQueue),add_message);
				}
			}
		}
		index ++;
	}
	
}

void	AddToInbox(ProcessControlBlock *PCB_Table[], INT32 process_id, Message *msg,  INT32 number_of_processes) {

	INT32				index = 0;
	ProcessControlBlock *tmp;
	Message*			add_message;


	while (index <number_of_processes) {
		if ( PCB_Table[index] != NULL)  {
			if (PCB_Table[index]->process_id == process_id ) {
				if (!IsExistsMessageIDQueue(PCB_Table[index]->inboxQueue,msg->msg_id) ) {
					add_message = CreateMessage(msg->msg_id,msg->target_id, msg->source_id, msg->send_length,msg->actual_msg_length,msg->msg_buffer,msg->is_broadcast);
					AddToMsgQueue(& (PCB_Table[index]->inboxQueue),add_message);
				}
			}
		}
		index ++;
	}
	
}


void AddToMsgQueue(Message **head, Message *msg)
{
	Message *tmp;
	Message *prev;

	if (IsMsgQueueEmpty(*head)) 
	{
		*head = msg;
		(*head)->nextMsg = NULL;
		return;
	}		

	// add by increasing of msg_id
	if ( (*head)->msg_id > msg->msg_id) {     //in front
		msg->nextMsg = *head;
		*head = msg;
		return;	
	}	

	tmp = *head;
	while ( (tmp != NULL) && (tmp->msg_id < msg->msg_id)) {           // <= so prev always has value
		prev = tmp;
		tmp = tmp->nextMsg;
	}
	prev->nextMsg = msg;								// insert in the Middle & End
	msg->nextMsg = NULL;								// reset linker

	if (tmp != NULL) {									// insert in the Middle
		msg->nextMsg = tmp;
		return;
	}

}

BOOL IsMsgQueueEmpty(Message *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}



BOOL	IsExistsMessageIDQueue(Message *head, INT32 msg_id)
{
	Message *tmp = head;
	if (head == NULL)
		return FALSE;

	while (tmp!= NULL) {
		if (tmp->msg_id == msg_id)
			return TRUE;

		tmp = tmp->nextMsg;
	}

	return FALSE;
}


// return -1 then no one send me

INT32	IsMyMessageInArray(Message *head[], INT32 process_id, Message *inbox, INT32 num_messages)
{
	INT32	count = 0;
	INT32	maxBroadCast = 0;					
	INT32	maxOnly = 0;																			// get most recently message sent to me

	BOOL	runAssign  = FALSE;


	Message *tmp = head[0];
	if (head == NULL)
		return -1;

	while ( count < num_messages )   {
		if ( (head[count] != NULL) 
			&& ( 
				(head[count]->target_id == process_id)												    // send directly to me
				||
				( (head[count]->target_id == -1)														// or broadcast message
					&& 
					(head[count]->source_id != process_id) )
				)
			&& (! IsExistsMessageIDQueue(inbox,head[count]->msg_id)) ) {								// new message
			//return count;	
				if (head[count]->target_id == process_id) {
					maxOnly = count;
					runAssign = TRUE;
				}

				if ((head[count]->target_id == -1)														// or broadcast message
					&& 
					(head[count]->source_id != process_id) ) {
						maxBroadCast = count;
						runAssign = TRUE;
						break;
				}

		}
		count++;
	}      
	


	if (runAssign) return ( (maxOnly >= maxBroadCast )?maxOnly:maxBroadCast) ;
	
	if ( (head[count] == NULL)  || (count >= MAX_MESSAGES) )
		return -1;

	
}

INT32	IsNewSendMsgInArray(Message *head[], INT32 target_pid,  INT32 source_pid, Message *inbox, INT32 num_messages) {
	INT32	count = num_messages - 1;
	BOOL	runAssign = FALSE;


	Message *tmp = head[0];
	if (head == NULL)
		return -1;

	while ( count  > 0 )   {

		if ( (head[count] != NULL) 
			&& ( 
				(head[count]->source_id == target_pid)												    // send directly to me
				&&
				(head[count]->target_id == source_pid)
			
				)) {								// new message
					if (count < (num_messages - 1) )// just send
						return -1;
					else return count;
		}


		if ( (head[count] != NULL) 
			&& ( 
				(head[count]->source_id == source_pid)												    // send directly to me
				&&
				(head[count]->target_id == target_pid) )
			&& (! IsExistsMessageIDQueue(inbox,head[count]->msg_id)) ) {								// new message
				runAssign = TRUE;
				return count;	
		}

		count--;
	}      
	
	if ( (head[count] == NULL)  || (count >= MAX_MESSAGES) || (!runAssign))
		return -1;

}
