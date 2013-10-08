/*********************************************************************
	This incude file is using by hdduong for Z502
	Sep/08/13:		Created
	Otc/06/13:		Update MessageQueue
*********************************************************************/
#include "global.h"
#include "student_global.h"



typedef struct StructMessage {
	INT32				msg_id;
	INT32				target_id;								// sent to
	INT32				source_id;								// from
	INT32				send_length;
	INT32				actual_msg_length;
	char				msg_buffer[LEGAL_MESSAGE_LENGTH];
	BOOL				is_broadcast;
	struct				StructMessage							*nextMsg;
} Message;



typedef struct StructProcessControlBlock
{
	INT32				process_id;
	char				*process_name;
	INT32				priority;
	void				*context;								//pointer to context for the process
	INT32				state;									// state of the process
	INT32				wakeup_time;							// time to wake up
	struct				StructProcessControlBlock				* nextPCB; 
	
	Message				*inboxQueue;									// receive message queue
	Message				*sentBoxQueue;								// sent message queue
} ProcessControlBlock;


ProcessControlBlock		*CreateProcessControlBlockWithData(char *process_name, void *starting_address, INT32 priority , INT32 process_id);
BOOL					IsQueueEmpty(ProcessControlBlock *head);
void					AddToTimerQueue(ProcessControlBlock **head, ProcessControlBlock *pcb);
void					AddToReadyQueue(ProcessControlBlock **head, ProcessControlBlock *pcb);
void					AddToReadyQueueNotPriority(ProcessControlBlock **head, ProcessControlBlock *pcb);
ProcessControlBlock		*DeQueue(ProcessControlBlock **head);
INT32					SizeQueue(ProcessControlBlock *head);
void					PrintQueue(ProcessControlBlock *head);
void					FreePCB(ProcessControlBlock *pcb);
void					DeleteQueue(ProcessControlBlock *head);
BOOL					IsExistsProcessIDQueue(ProcessControlBlock *head, INT32 process_id);
void					RemoveProcessFromQueue(ProcessControlBlock **head, INT32 process_id); 
ProcessControlBlock		*PullProcessFromQueue(ProcessControlBlock **head, INT32 process_id); 
void					UpdateProcessPriorityQueue(ProcessControlBlock **head, INT32 process_id,INT32 new_priority); 

ProcessControlBlock		*InsertLinkedListPID(ProcessControlBlock *head, ProcessControlBlock *pcb);
BOOL					IsExistsProcessNameArray(ProcessControlBlock *head[], char *process_name, INT32 number_of_processes);
BOOL					IsExistsProcessIDArray(ProcessControlBlock *head[], INT32 process_id,INT32 number_of_processes);
void					RemoveFromArray(ProcessControlBlock *head[], INT32 process_id, INT32 number_of_processes);
void					RemoveLinkedList(ProcessControlBlock *head);
INT32					GetProcessID(ProcessControlBlock *Head[], char* process_name, INT32 num_processes);
INT32					CountActiveProcesses(ProcessControlBlock *head[], INT32 num_processes);

BOOL					IsExistsProcessIDList(ProcessControlBlock *head, INT32 process_id);
ProcessControlBlock		*PullFromSuspendList(ProcessControlBlock **head, INT32 process_id);
BOOL					IsListEmpty(ProcessControlBlock *head);
void					AddToSuspendList(ProcessControlBlock **head, ProcessControlBlock *pcb);
BOOL					IsKilledProcess(ProcessControlBlock *head[], INT32 process_id, INT32 num_processes);


Message					*CreateMessage(INT32 msg_id, INT32 target_id, INT32 source_id, INT32 send_length, INT32 actual_msg_length, char *msg_buffer, BOOL is_broadcast);
void					AddToSentBox(ProcessControlBlock *PCB_Table[], INT32 process_id, Message *msg,  INT32 number_of_processes);
void					AddToInbox(ProcessControlBlock *PCB_Table[], INT32 process_id, Message *msg,  INT32 number_of_processes);
void					AddToMsgQueue(Message **head, Message *msg);
BOOL					IsMsgQueueEmpty(Message *head);
BOOL					IsExistsMessageIDQueue(Message *head, INT32 msg_id);
INT32					IsMyMessageInArray(Message *head[], INT32 process_id, Message *inbox, INT32 num_messages);
void					AddToMsgSuspendList(ProcessControlBlock **head, ProcessControlBlock *pcb);

