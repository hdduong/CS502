/*********************************************************************
	This incude file is using by hdduong for Z502
	Sep/08/13:		Created
	Otc/06/13:		Update MessageQueue
*********************************************************************/
#include "global.h"
#include "student_global.h"



typedef struct StructMessage {
	INT32				msg_id;									// send and receive message is kept track by global message queue. So each message has a id	
	INT32				target_id;								// sent to
	INT32				source_id;								// from
	INT32				send_length;							// length of the buffer
	INT32				actual_msg_length;						// actual length of the message
	char				msg_buffer[LEGAL_MESSAGE_LENGTH];		// data of the message
	BOOL				is_broadcast;							// TRUE: a boardcast message. For future use
	INT32				message_state;							// Record if message already receivedd by a process this message wont be taken into another process inbox 
																// THIS IS FIX FOR broadcast that only one process can receive
	struct				StructMessage		*nextMsg;			
} Message;



typedef struct StructProcessControlBlock
{
	INT32				process_id;
	char				*process_name;									//name of the process. 
	INT32				priority;										//priority of the process
	void				*context;										//pointer to context for the process
	INT32				state;											//state of the process
	INT32				wakeup_time;									//time to wake up. It is the absolute time. Expected this time the interrupt occured
	struct				StructProcessControlBlock	* nextPCB; 
	
	Message				*inboxQueue;									// receive message queue
	Message				*sentBoxQueue;									// sent message queue

	UINT16				pcb_PageTable[MEMSIZE];							// Page table of each PCB 1024
} ProcessControlBlock;


typedef struct StructPageFrameMapping 
{
	INT32				process_id;										// who is using this frame
	INT16				page;											// logical page number using this frame. -1 is not used
	INT16				frame;											// frame number if occupied
	struct				StructPageFrameMapping			*nextPFM;		// point to next mapping	
} PageFrameMapping;

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
INT32					IsNewSendMsgInArray(Message *head[], INT32 target_pid,  INT32 source_pid, Message *inbox, INT32 num_messages);

INT16					FindLastOccupiedFrame(PageFrameMapping *head);
void					AddToMappingTableList(PageFrameMapping **head, PageFrameMapping *pfm, INT16 assignedFrame );
BOOL					IsMappingListEmpty(PageFrameMapping *head);