#include				 <stdio.h>
#include                 <stdlib.h>
#include                 <memory.h>
#include				 <string.h>


#include				"student_queue.h"
#include				"student_global.h"


/********************************** hdduong **************************************************
	This is the file implement queue, linkedlist coded by hdduong

	Sep/08/13:		Created
	Sep/16/13:		Update List
	Sep/29/13:		Version z502_v4: change PCB Table from List to Array
	Oct/02/13:		Version z502_v5: Priority Queue
	Oct/05/13:		Version z502_v6: update with MessageQueue for Send/Receive message
	Oct/10/13:		Version z502_v7: have to change broadcast so now only one can receive
************************************* hdduong *************************************************/

//------------------------------------------------------------------//
//							QUEUE									//
//					TimerQueue, ReadyQueue							//
//------------------------------------------------------------------//



/************************************************************************************************************************************
CreateProcessControlBlockWithData

ProcessControlBlock Allocate Memory for a process. 
Inbox and SentBox always set to NULL (not receive or send messages at first place)
Return a pointer to a new PCB created. Return NULL if Memory Error
Use in os_create_process
************************************************************************************************************************************/
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

	memset(pcb->pcb_PageTable,0,sizeof(pcb->pcb_PageTable));				// the default value = 0 for all memeory spots 
	pcb->disk_io.disk_id = -1;												// no use disk
	pcb->disk_io.operation = 0;												// default read

	pcb->flag = 0;

	return pcb;

}


/*****************************************************
IsQueueEmpty

Check a queue is empty or not. 
Return TRUE if is empty. Otherwise return FALSE
Apply to ReadyQueue, TimerQueue
*****************************************************/
BOOL IsQueueEmpty(ProcessControlBlock *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}


/***********************************************************************
AddToTimerQueue

Add a PCB to TimerQueue. New PCB will be ordered by wakeup time.
Wakeup time (absolute time) is the time interrupt expected ot occur.
************************************************************************/

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

/***********************************************************************
AddToReadyQueue

Add a PCB to ReadyQueue. New PCB will be ordered its Priority.
Apply from test1d to test1l (except test1k)
************************************************************************/

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


/***********************************************************************
AddToReadyQueueNotPriority

Add a PCB to ReadyQueue. New PCB is inserted to end of the Queue.
Apply to test1b, test1c
************************************************************************/

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

/***********************************************************************
DeQueue

Take out the very first PCB of the Queue. 
************************************************************************/

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


/***********************************************************************
SizeQueue

Returns number of PCBs in Queue
************************************************************************/

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



/***********************************************************************
PrintQueue

Print all the process_id of all PCB in Queue
This is obselte method. Use state_printer instead
************************************************************************/

void PrintQueue(ProcessControlBlock *head)
{
	if (IsQueueEmpty(head))
	{
		return;
	}
	else
	{
		while (head != NULL)
		{
			printf("%d ",head->process_id);
			head = head->nextPCB;
		}
		printf(";\n");
	}

}



/***********************************************************************
FreePCB

Dellocate Memeory of a PCB
************************************************************************/

void FreePCB(ProcessControlBlock *pcb)
{
	if (pcb != NULL) {
		pcb->nextPCB = NULL;
		free(pcb->process_name);
		free(pcb);
	}
	pcb = NULL;
}



/***********************************************************************
DeleteQueue

Remove Queue with all its PCB
************************************************************************/

void DeleteQueue(ProcessControlBlock *head) 
{
	ProcessControlBlock *tmp = head;
	while(head != NULL) {
		head = head->nextPCB;
		FreePCB(tmp);
		tmp = head;
	}
}



/***************************************************************************
IsExistsProcessIDQueue

Check whether a PCB with process_id is in Queue
Return TRUE if in Queue. Otherwise FALSE
****************************************************************************/
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



/*******************************************************************************************************
RemoveProcessFromQueue

Delete a PCB with process_id in Queue. Always call IsExistsProcessIDQueue before RemoveProcessFromQueue.
So this function always assumes that process_id already exists
********************************************************************************************************/
void	RemoveProcessFromQueue(ProcessControlBlock **head, INT32 process_id) 
{
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
	//FreePCB(tmp);																	//do not dellocate memory there. I want to keep track in PCB_Table
																					//PCB has TERMINATE state means this PCB already terminated.
}




/**********************************************************************************************************
PullProcessFromQueue

Take out a PCB with process_id in Queue. Always call IsExistsProcessIDQueue before PullProcessFromQueue.
So this function always assumes that process_id already exists
************************************************************************************************************/
ProcessControlBlock	*PullProcessFromQueue(ProcessControlBlock **head, INT32 process_id) 
{

	ProcessControlBlock *prev = NULL;
	ProcessControlBlock *tmp = *head;

	if (tmp->process_id == process_id) {											// take out the head
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


/**********************************************************************************************************************
UpdateProcessPriorityQueue

Update Priority of a PCB with process_id in Queue. Always call IsExistsProcessIDQueue before UpdateProcessPriorityQueue.
So this function always assumes that process_id already exists
***********************************************************************************************************************/

void	UpdateProcessPriorityQueue(ProcessControlBlock **head, INT32 process_id,INT32 new_priority) 
{
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
//			       LINKED LIST										//
//------------------------------------------------------------------//


/***********************************************************************************
IsExistsProcessIDList

Check whether a PCB with process_id exists in LinkdedList.
Return TRUE if it does. Otherwise returns FALSE
************************************************************************************/

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


/***********************************************************************************
IsListEmpty

Check whether a PCB with process_id exists in LinkdedList.
Return TRUE if it does. Otherwise returns FALSE
************************************************************************************/

BOOL IsListEmpty(ProcessControlBlock *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}




/***********************************************************************************
AddToSuspendList

Add a PCB to Suspend List. This list stores PCB when SUSPEND_PROCESS invoked.
************************************************************************************/
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



/********************************************************************************************
AddToSuspendList

Add a PCB to Message Suspend List. 
This list stores PCB when SEND_MESSAGE/ RECEIVE_MESSAGE invoked.
*********************************************************************************************/

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


/********************************************************************************************
PullFromSuspendList

Take out from Suspend List a PCB with process_id.
*********************************************************************************************/

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
//					Store all PCB by using Arrays					//
//						For refrence only							//
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


/***********************************************************************************************************
IsExistsProcessNameArray

Check whether a PCB with process_name is ever created. 
Return TRUE if already created. Return FALSE otherwise.
************************************************************************************************************/
BOOL	IsExistsProcessNameArray(ProcessControlBlock *head[], char* process_name, INT32 number_of_processes)
{
	INT32	count = 0;

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


/***********************************************************************************************************
IsExistsProcessNameArray

Check whether a PCB with process_id is ever created. 
Return TRUE if already created. Return FALSE otherwise.
************************************************************************************************************/
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


/******************************************************************************************************************
RemoveFromArray

Set state of a PCB to TERMINATE. So this PCB ismarked not to scheduled to run anymore.
*******************************************************************************************************************/

void RemoveFromArray(ProcessControlBlock *head[], INT32 process_id, INT32 num_processes)  
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




/***********************************************************************************************************
GetProcessID

Get process_id if process_name provided. Assume that process_name is already created.
************************************************************************************************************/

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

/***********************************************************************************************************
CountActiveProcesses

Get number of processes are not terminated yet. 
************************************************************************************************************/

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



/***********************************************************************************************************
IsKilledProcess

Check whether a process was terminated. 
Return TRUE if it was. Otherwise return FALSE
************************************************************************************************************/

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

/*****************************************************************************************************************************************************
CreateMessage

Allocate Memory for a message. 
Return a pointer to a new message created. Return NULL if Memory Error
is_broadcast is reserve for furture purpose. 
*******************************************************************************************************************************************************/

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
	msg->message_state = MESSAGE_STATE_FREE;
	return msg;

}



/*****************************************************************************************************************************************************
AddToSentBox

Add a legal message going to send to process sendBox. This message is stored in global message Queue. 
Add a message to a process via PCB_Table otherwise have to loop seperately in ReadyQueue, TimerQueue and SuspendQueue to find a process
*******************************************************************************************************************************************************/
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




/*****************************************************************************************************************************************************
AddToInbox

Go to global message Queue to check if new message sent to a process. If process finds a new message then add message to it inbox Queue. 
Add one messge because this process only send only one echo per time.
Add a message to a process via PCB_Table otherwise have to loop seperately in ReadyQueue, TimerQueue and SuspendQueue to find a process
*******************************************************************************************************************************************************/
void	AddToInbox(ProcessControlBlock *PCB_Table[], INT32 process_id, Message *msg,  INT32 number_of_processes) {

	INT32				index = 0;
	ProcessControlBlock *tmp;
	Message*			add_message;


	while (index <number_of_processes) {
		if ( PCB_Table[index] != NULL)  {
			if (PCB_Table[index]->process_id == process_id ) {
				if (!IsExistsMessageIDQueue(PCB_Table[index]->inboxQueue,msg->msg_id) ) {
					msg->message_state = MESSAGE_STATE_RECEIVED;
					add_message = CreateMessage(msg->msg_id,msg->target_id, msg->source_id, msg->send_length,msg->actual_msg_length,msg->msg_buffer,msg->is_broadcast);
					add_message->message_state = MESSAGE_STATE_RECEIVED;
					AddToMsgQueue(& (PCB_Table[index]->inboxQueue),add_message);
				}
			}
		}
		index ++;
	}
	
}


/*****************************************************************************************************************************************************
AddToMsgQueue

Add message to inbox or sentBox Queue. New message inserted at the end of Queue (Or increasing of message id).
*******************************************************************************************************************************************************/

void AddToMsgQueue(Message **head, Message *msg)
{
	Message *tmp = NULL;
	Message *prev = NULL;

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
	if (prev != NULL) {
		prev->nextMsg = msg;								// insert in the Middle & End
		msg->nextMsg = NULL;								// reset linker
	}

	if (tmp != NULL) {									// insert in the Middle
		msg->nextMsg = tmp;
		return;
	}

}



/*********************************************************************************
IsMsgQueueEmpty

Check a Queue has processes or not.
Return TRUE if has at least one process. Otherwise return FALSE
*********************************************************************************/

BOOL IsMsgQueueEmpty(Message *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}


/*********************************************************************************
IsExistsMessageIDQueue

Check a message with message_id is in Queue or not
Return TRUE if it in Queue. Otherwise return FALSE
*********************************************************************************/

BOOL	IsExistsMessageIDQueue(Message *head, INT32 msg_id)
{
	Message *tmp = head;
	if (head == NULL)
		return FALSE;

	while (tmp!= NULL) {
		if ( (tmp->msg_id == msg_id) )
			if (tmp->message_state != MESSAGE_STATE_FREE)
				return TRUE;
			else 
				return FALSE;
		tmp = tmp->nextMsg;
	}

	return FALSE;
}


/**************************************************************************************************
IsMyMessageInArray

Check if a new message is sent to current process or not. 
Find a new message by loop through global process queue.
There are two cases:
Other processes send message directly to current process by specified target process_id. (1)
Other processes send broadcast message and current process would want to receive it.	 (2)
With (1): would get the most recent messsage (based on message_id)
With (2): would get the last recent message (based on messsage_id)
Return index if a process can find a message it would want to receive. 
Return -1 if a no process sends a message to current process. 
***************************************************************************************************/

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
			&& (! IsExistsMessageIDQueue(inbox,head[count]->msg_id)) 
			&& (head[count]->message_state != MESSAGE_STATE_RECEIVED ) ) {								// new message
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



/**************************************************************************************************************************************************
IsNewSendMsgInArray

This function is coded primary to make test1l to work successfully. 
Check whether a newer echo message is available. This deal with a case: Before a process resume (in Case 3), it already stored a previous echo
message (from Case 2) because of suspend. Then when this process resumes, it going to to send the previous message, not the lasted message. 
This is due to the limitation structure of Queue (FIFO). Maybe Stack (LIFO) might work.

There are two cases:
Other processes send message directly to current process by specified target process_id. If the message can be found, this message should be newer 
than the message it is storing in process's inbox (1)
Other processes send broadcast message and current process would want to receive it. If the message can be found, this message should be newer 
than the message it is storing in process's inbox (2)
With (1): would get the most recent messsage (based on message_id) which newer than lasted current message. 
With (2): would get the last recent message (based on messsage_id) which newer than lasted current message.
If both (1) and (2) satisfy then compare the index to get the lastest between two. 
Return index if a process can find a message it would want to receive. 
Return -1 if a no process sends a message to current process. 
*************************************************************************************************************************************************/

INT32	IsNewSendMsgInArray(Message *head[], INT32 target_pid,  INT32 source_pid, Message *inbox, INT32 num_messages) {
	INT32	countOnly = num_messages - 1;
	BOOL	runAssignOnly = FALSE;
	INT32	indexOnly = - 1;

	INT32	countMax = 0;
	INT32	runAssignMax = FALSE;
	INT32	indexMax = 0;

	Message *tmp = head[0];
	if (head == NULL)
		return -1;

	while ( countOnly  > 0 )   {

		if ( (head[countOnly] != NULL) 
			&& ( 
				(head[countOnly]->source_id == target_pid)												    // send directly to me
				&&
				(head[countOnly]->target_id == source_pid)
				&& (head[countOnly]->message_state = MESSAGE_STATE_FREE)
				)) {								// new message
					if (countOnly < (num_messages - 1) ) {// just send
						runAssignOnly = FALSE;
					}
					else {
						indexOnly = countOnly;
						runAssignOnly = TRUE;
						break;
					}
		}
		countOnly--;
	}

	while ( countMax  < num_messages) {
		if ( (head[countMax] != NULL) 
			&& ( 
				(head[countMax]->source_id == source_pid)												    // send directly to me
				&&
				(head[countMax]->target_id == -1) )
			&& (! IsExistsMessageIDQueue(inbox,head[countMax]->msg_id)) ) {								    // new message
				runAssignMax = TRUE;
				indexMax = countMax;
		}
	countMax++;
	}      
	
	if ( (!runAssignMax) && (!runAssignOnly)) return -1;

	if ( (runAssignOnly) && (!runAssignMax) ) return indexOnly;

	if ( (runAssignMax) && (!runAssignOnly) ) return (( indexOnly == -1)?indexOnly:indexMax);

	if ( (runAssignMax) && (runAssignOnly) ) return (( indexMax> indexOnly)? indexMax:indexOnly);

}


/********************************************************************************************************



*********************************************************************************************************/



BOOL IsMappingListEmpty(PageFrameMapping *head)
{
	if (head == NULL)  
	{
		return TRUE;
	}
	return FALSE;
}



void AddToMappingTableList(PageFrameMapping **head, PageFrameMapping *pfm, INT16 assignedFrame )
{
	
	PageFrameMapping	*tmp;
	
	if (IsMappingListEmpty(*head)) 
	{
		*head = pfm;
		(*head)->nextPFM = NULL;
		pfm->frame = 0;
		return;
	}		

	// add at the end
	
	tmp = *head;
	while (tmp->nextPFM != NULL) {           // <= so prev always has value	
		tmp = tmp->nextPFM;
	}
	
	tmp->nextPFM = pfm;
	pfm->frame = assignedFrame;
	pfm->nextPFM = NULL;								// reset linker

}

void AddToDiskQueue(ProcessControlBlock **head, ProcessControlBlock *pcb)
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


ProcessControlBlock *DeQueueWithDiskId(ProcessControlBlock **head, INT32 disk_id)
{
	ProcessControlBlock *tmp = *head;
	ProcessControlBlock *prev = NULL;

	if (IsQueueEmpty(*head)) 
	{
		return NULL;
	}
	
	while (tmp != NULL) {
		if ( (tmp->disk_io.disk_id == disk_id) ) { // && (tmp->state != PROCESS_STATE_TERMINATE) 
			//First one
			if (tmp == *head) {
				*head = tmp->nextPCB;
				tmp->nextPCB = NULL;
				return tmp;
			}
			// last one 
			else if (tmp->nextPCB == NULL) {
				prev->nextPCB = NULL;
				return tmp;
			}
			else { // middile
				prev->nextPCB = tmp->nextPCB;
				tmp->nextPCB = NULL;
				return tmp;
			}

		}
		prev = tmp;
		tmp = tmp->nextPCB;
	}

	if ( (tmp == NULL)  &&(prev != NULL) )
		return NULL;
}


void AddToDataWrittenQueue(Disk **head, Disk *data)
{
	Disk *tmp;
	Disk *prev;
	
	if (*head == NULL)
	{
		*head = data;
		(*head)->nextData = NULL;
		return;
	}		

	tmp = *head;
	while (tmp != NULL) {
		prev = tmp;
		tmp = tmp->nextData;
	
	}
	prev->nextData = data;								// insert to end	
	data->nextData = tmp;

}


Disk* GetDataWithInfo(Disk *head, INT32 disk_id, INT32 sector)
{
	Disk *tmp = head;
	Disk *prev = NULL;

	if (head == NULL)
	{
		return NULL;
	}
	
	while (tmp != NULL) {
		if ( (tmp->disk_id == disk_id)  && (tmp->sector == sector) ) { 
			return tmp;
		}
		prev = tmp;
		tmp = tmp->nextData;
	}

	if ( (tmp == NULL)  &&(prev != NULL) )
		return NULL;
}


Disk* GetDataWithInfoPid(Disk *head, INT32 disk_id, INT32 sector, INT32 pid)
{
	Disk *tmp = head;
	Disk *prev = NULL;

	if (head == NULL)
	{
		return NULL;
	}
	
	while (tmp != NULL) {
		if ( (tmp->disk_id == disk_id)  && (tmp->sector == sector) && (tmp->process_id == pid) ) { 
			return tmp;
		}
		prev = tmp;
		tmp = tmp->nextData;
	}

	if ( (tmp == NULL)  &&(prev != NULL) )
		return NULL;
}



void AddToShadowTable(ShadowTable **head, ShadowTable *newEntry, INT32 *countEntry) {
	ShadowTable *tmp = (*head);
	*countEntry = *countEntry + 1;

	newEntry->shadow_id = *countEntry;

	if (tmp == NULL) {
		(*head) = newEntry;
		return;
	}

	while (tmp->next != NULL) {
		tmp = tmp->next;
	}
	tmp->next = newEntry;

}



