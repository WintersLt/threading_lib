#include "mythread.h"
//#include "_mythread.h"
#include "list.h"
#include "stdio.h"
#include "stdlib.h"
#include "ucontext.h"

#define STACK_SIZE 8*1024 //8 KB

typedef enum
{
	READY = 0,
	RUNNING,
	BLOCKED,
	INVALID
}STATE_e;

typedef struct _MyThreadCtx
{
	ucontext_t* p_uctx;
	struct _MyThreadCtx* p_parent; // parent to be used to unblock parent when child exits
	list* p_child_list;  //To be used ny join all, contains pointers to children
	list* p_wait_list;  //Contains pointers to threads this process needs to join with
	STATE_e state;
}MyThreadCtx_t;

/*Internal variable declarations*/
static list readyQ;
static list blockedQ;
static ucontext_t _loop_ctx; //Context to which every thread returns to. This is not main
static ucontext_t _main_ctx; //main context of calling unix process
static int _do_exit = 0; //To be set when readyQ is empty
static int _is_initialized = 0; //to be set in _my_thread_init

/*Internal function declarations*/
void _my_thread_init(void(*start_funct)(void *), void *args);
MyThreadCtx_t* _create_thread_ctx(void(*start_funct)(void *), void *args);
void _my_thread_loop();
void _cleanup_ctx(MyThreadCtx_t*);

////////////////////////////////////////////////////////

void MyThreadInit(void(*start_funct)(void *), void *args)
{
	//extract main context
	getcontext(&_main_ctx);	//_main_context->uc_link will be that of main unix proc
	//initialize internal queues
	if(!_do_exit)
	{
		_my_thread_init(*start_funct, args);
		//start ready loop
		_my_thread_loop();
	}
	else
	{
		//return to main unix thread
		return;
	}
}

void _my_thread_init(void(*start_funct)(void *), void *args)
{
	if(!_is_initialized)
	{
		//initQs
		list_new(&readyQ, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
		list_new(&blockedQ, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
		//create context
		MyThreadCtx_t* new_thread_ctx = _create_thread_ctx(start_funct, args);
		if(new_thread_ctx)
		{
			list_append(&readyQ, (void*)new_thread_ctx);
		}
		else
		{
			//TODO
		}
		_is_initialized = 1;
	}
	else
	{
		printf("MyThread already initialized");
	}
}

MyThreadCtx_t* _create_thread_ctx(void(*start_funct)(void *), void *args)
{
	MyThreadCtx_t* my_thread_ctx = (MyThreadCtx_t*)calloc(1, sizeof(MyThreadCtx_t));
	if(!my_thread_ctx)
	{
		printf("error");
		//TODO handle
		return NULL;
	}
	my_thread_ctx->p_uctx = (ucontext_t*)calloc(1, sizeof(ucontext_t));
	//prepare context
	getcontext(my_thread_ctx->p_uctx);
	//allocate stack
    my_thread_ctx->p_uctx->uc_stack.ss_sp = (char*)calloc(STACK_SIZE, sizeof(char));
    my_thread_ctx->p_uctx->uc_stack.ss_size = STACK_SIZE;
	//set uclink to current context
	my_thread_ctx->p_uctx->uc_link = &_loop_ctx;
	makecontext(my_thread_ctx->p_uctx, (void (*)())start_funct, 1, args);

	my_thread_ctx->p_parent = NULL;
	my_thread_ctx->p_child_list = (list*)calloc(1, sizeof(list));
	list_new(my_thread_ctx->p_child_list, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
	my_thread_ctx->p_wait_list = (list*)calloc(1, sizeof(list));
	list_new(my_thread_ctx->p_wait_list, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
	my_thread_ctx->state = READY;

	return my_thread_ctx;
}

void _my_thread_loop()
{
	while(readyQ.logicalLength)
	{
		//getcontext(&_loop_ctx);
		MyThreadCtx_t* ctx_to_exec = (MyThreadCtx_t*)readyQ.head->data;
		//switch context to top of ready queue 
		printf("_my_thread_init: swapping to thread ctx\n");
		ctx_to_exec->state = RUNNING;
		swapcontext(&_loop_ctx, ctx_to_exec->p_uctx);
		printf("_my_thread_init: back to _loop_ctx\n");
		//in case head changed while I was sleeping
		if(readyQ.logicalLength)
		{
			ctx_to_exec = (MyThreadCtx_t*)readyQ.head->data;
			//state will be ready, if thread yeilded or called MythreadExit...
			if(ctx_to_exec->state == RUNNING)
			{
				MyThreadExit();
			}
		}
		//_check_parent_status block unblock
		//_check_child_status set parent to null
		//_cleanup(ctx_to_exec);
		//remove from ready queue
		//check parent if it is blocked
		//release resources it holds
		//later check semaphore status
	}
	//retunr to main unix program gracefully, after deleting any resurces in queues
	//For that we'll have to keep the main context saved
	_do_exit = 1;
	setcontext(&_main_ctx);
}

void MyThreadExit(void)
{
	printf("MyThreadExit: Enter for ctx<%p>\n", readyQ.head);
	listNode* curr_node = readyQ.head; //Thread that called exit
	MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)curr_node->data;
	//delete from child list
	if(curr_thread_ctx->p_parent)
	{
		list_remove_node(curr_thread_ctx->p_parent->p_child_list, curr_node->data);
	}
	//join
	if(curr_thread_ctx->p_parent
		&& curr_thread_ctx->p_parent->state == BLOCKED)
	{
		list_remove_node(curr_thread_ctx->p_parent->p_wait_list, curr_node->data);
		//if wait_list of parent is empty, unblock it
		if(!curr_thread_ctx->p_parent->p_wait_list->logicalLength)
		{
			//unblocking parent
			//TODO perform semaphore block check as well
			list_remove_node(&blockedQ, curr_thread_ctx->p_parent);
			curr_thread_ctx->p_parent->state = READY;
			list_append(&readyQ, curr_thread_ctx->p_parent);
		}
	}
	//unset p_parent in all of its children
	listNode* child = curr_thread_ctx->p_child_list->head;	
	while(child != NULL)
	{
		MyThreadCtx_t* child_ctx = (MyThreadCtx_t*)child->data;	
		child_ctx->p_parent = NULL;
		child = child->next;
	}
	//_cleanup
	list_pop_front(&readyQ);
	_cleanup_ctx(curr_thread_ctx);
	//swap to loop
	setcontext(&_loop_ctx);
}

void _cleanup_ctx(MyThreadCtx_t* ctx)
{
	printf("_cleanup_ctx: Enter<%p>\n", ctx);
	list_destroy(ctx->p_wait_list);
	list_destroy(ctx->p_child_list);
	free(ctx->p_uctx->uc_stack.ss_sp);
	free(ctx->p_uctx);
	free(ctx);
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	MyThreadCtx_t* new_thread_ctx = _create_thread_ctx(start_funct, args);
	if(new_thread_ctx)
	{
		new_thread_ctx->p_parent = (MyThreadCtx_t*)readyQ.head->data;
		list_append(new_thread_ctx->p_parent->p_child_list, new_thread_ctx);
		list_append(&readyQ, (void*)new_thread_ctx);
	}
	return (MyThread)new_thread_ctx;
}

void MyThreadYield(void)
{
	if(readyQ.logicalLength > 1)
	{
		MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)list_pop_front(&readyQ);
		curr_thread_ctx->state = READY;
		list_append(&readyQ, curr_thread_ctx);
		swapcontext(curr_thread_ctx->p_uctx, &_loop_ctx);
	}
}

//Only a parent can join a child, no any to any join support
int MyThreadJoin(MyThread thread)
{
	MyThreadCtx_t* thread_to_join_ctx = (MyThreadCtx_t*)thread;
	if(thread_to_join_ctx 
		/*Only parent can join*/
		&& thread_to_join_ctx->p_parent == (MyThreadCtx_t*)readyQ.head->data 
		/*check if its a child*/
		&& (list_search_node(thread_to_join_ctx->p_parent->p_child_list, thread_to_join_ctx)) 
		/*check if not already waiting*/
		&& !list_search_node(thread_to_join_ctx->p_parent->p_wait_list, thread_to_join_ctx))
	{
		list_append(thread_to_join_ctx->p_parent->p_wait_list, (void*)thread_to_join_ctx);
		MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)list_pop_front(&readyQ);
		curr_thread_ctx->state = BLOCKED;
		list_append(&blockedQ, curr_thread_ctx);
		swapcontext(curr_thread_ctx->p_uctx, &_loop_ctx);
		return 0;
	}
	return 1;
}

void MyThreadJoinAll(void)
{
	listNode* child = readyQ.head;
	MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)readyQ.head->data;
	int inserted = 0;
	while(child != NULL)
	{
		MyThreadCtx_t* child_ctx = (MyThreadCtx_t*)child->data;
		if(!list_search_node(curr_thread_ctx->p_wait_list, child_ctx))	
		{
			list_append(curr_thread_ctx->p_wait_list, (void*)child_ctx);
			inserted = 1;
		}
		child = child->next;
	}
	if(inserted)
	{
		curr_thread_ctx = (MyThreadCtx_t*)list_pop_front(&readyQ);
		curr_thread_ctx->state = BLOCKED;
		list_append(&blockedQ, curr_thread_ctx);
		swapcontext(curr_thread_ctx->p_uctx, &_loop_ctx);
	}
}
