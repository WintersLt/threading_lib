//TODO
//1. Extra credit2: Mark the main context as special using some boolean. This
//   can be used to deprioritize main_ctx whenever it shows up in readyQ and
//   there are still threads in readyQ
//2. NULL checks for all pointers
//3. Error handling for system calls
//4. Zombie thread case -: Thread creates 10 children and starts joining them
//   one by one. So dont delete threads if the parent has not yet exited and 
//   the current thread is not in waitQ of parent. And so when parent exits we
//   have to delete all remaining childrent, which didn;t get to join. Also, 
//   record how the library initialization has been done. This helps to decide
//   ehether or not to switch to main_ctx at the end of loop.
//TODO
//1. Remove all unnecessary prints
 
//#include "mythread.h"
#include "mythreadextra.h"
//#include "_mythread.h"
#include "list.h"
#include "stdio.h"
#include "stdlib.h"
#include "ucontext.h"
#define printf(...) myfunc()
#define STACK_SIZE 8*1024 //8 KB

void myfunc()
{}
typedef enum
{
	READY = 0,
	RUNNING,
	BLOCKED,
	SEM_WAIT,
	INVALID
}STATE_e;

typedef struct _MyThreadCtx
{
	ucontext_t* p_uctx;
	struct _MyThreadCtx* p_parent; // parent to be used to unblock parent when child exits
	list* p_child_list;  //To be used ny join all, contains pointers to children
	list* p_wait_list;  //Contains pointers to threads this process needs to join with
	STATE_e state;
	int is_main_ctx;  //0 = Noot main ctx, used only by _extra functions
}MyThreadCtx_t;

typedef struct _MySemaphore
{
	int value;
	list* waitQ;
}MySemaphore_t;

/*Internal variable declarations*/
static list readyQ;
static list blockedQ;
static list semList;
static ucontext_t _loop_ctx; //Context to which every thread returns to. This is not main
static ucontext_t _main_ctx; //main context of calling unix process
static int _do_exit = 0; //To be set when readyQ is empty
static int _is_initialized = 0; //to be set in _my_thread_init
static int _is_initialized_extra = 0; //to be set in _my_thread_init_extra

/*Internal function declarations*/
void _my_thread_init_extra(ucontext_t* p_uctx);
void _my_thread_init(void(*start_funct)(void *), void *args);
void _my_thread_init_queues();
MyThreadCtx_t* _create_thread_ctx_from_uctx(ucontext_t* p_uctx);
MyThreadCtx_t* _create_thread_ctx(void(*start_funct)(void *), void *args);
void _my_thread_loop_extra();
void _my_thread_loop();
void _cleanup_ctx(MyThreadCtx_t*);
void _cleanup_mythreadlib();

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

void MyThreadInitExtra()
{
	//initialize internal queues
	ucontext_t* p_uctx = (ucontext_t*)calloc(1, sizeof(ucontext_t));
	getcontext(p_uctx);	
	if(_is_initialized_extra)	//So that the function returns when the context resumes
		return;
	if(!_do_exit)
	{
		_my_thread_init_extra(p_uctx);
		//start ready loop
		_my_thread_loop_extra();
	}
	else
	{
		//return to main unix thread
		return;
	}
}

void _my_thread_init_queues()
{
	if(!(_is_initialized || _is_initialized_extra))
	{
		list_new(&readyQ, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
		list_new(&blockedQ, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
		list_new(&semList, sizeof(MySemaphore_t), NULL/*freeFunc*/);
	}
}

void _my_thread_init_extra(ucontext_t* p_uctx)
{
	if(!_is_initialized_extra)
	{
		//initQs
		_my_thread_init_queues();
		//create context
		MyThreadCtx_t* new_thread_ctx = _create_thread_ctx_from_uctx(p_uctx);
		if(new_thread_ctx)
		{
			new_thread_ctx->is_main_ctx = 1;
			list_append(&readyQ, (void*)new_thread_ctx);
		}
		else
		{
			//TODO
		}
		_is_initialized_extra = 1;
	}
	else
	{
		printf("_my_thread_init_extra(): MyThread already initialized");
	}
}

void _my_thread_init(void(*start_funct)(void *), void *args)
{
	if(!_is_initialized)
	{
		//initQs
		_my_thread_init_queues();
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

MyThreadCtx_t* _create_thread_ctx_from_uctx(ucontext_t* p_uctx)
{
	MyThreadCtx_t* my_thread_ctx = (MyThreadCtx_t*)calloc(1, sizeof(MyThreadCtx_t));
	if(!my_thread_ctx)
	{
		printf("error");
		//TODO handle
		return NULL;
	}
	my_thread_ctx->p_uctx = p_uctx;

	my_thread_ctx->p_parent = NULL;
	my_thread_ctx->p_child_list = (list*)calloc(1, sizeof(list));
	list_new(my_thread_ctx->p_child_list, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
	my_thread_ctx->p_wait_list = (list*)calloc(1, sizeof(list));
	list_new(my_thread_ctx->p_wait_list, sizeof(MyThreadCtx_t), NULL/*freeFunc*/);
	my_thread_ctx->state = READY;

	return my_thread_ctx;
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

void _my_thread_loop_extra()
{
	while(readyQ.logicalLength)
	{
		MyThreadCtx_t* ctx_to_exec = (MyThreadCtx_t*)readyQ.head->data;
		if(ctx_to_exec->is_main_ctx == 1
			&& readyQ.logicalLength > 1)
		{
			//put it at the back of Queue. 
			list_pop_front(&readyQ);
			list_append(&readyQ, ctx_to_exec);
			ctx_to_exec = (MyThreadCtx_t*)readyQ.head->data;
		}
		//switch context to top of ready queue 
		printf("_my_thread_init_extra: swapping to thread ctx\n");
		ctx_to_exec->state = RUNNING;
		swapcontext(&_loop_ctx, ctx_to_exec->p_uctx);
		printf("_my_thread_init_extra: back to _loop_ctx\n");
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
	}
	//everything done
	//TODO _cleanup
	_cleanup_mythreadlib();
	exit(0);
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
	//return to main unix program gracefully, after deleting any resurces in queues
	//For that we'll have to keep the main context saved
	_do_exit = 1;
	_cleanup_mythreadlib();
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
		/*Only to prevent unnecessary check in next condition*/
		&& curr_thread_ctx->p_parent->state == BLOCKED 
		/*check if parent is waiting on this thread*/
		&& list_search_node(curr_thread_ctx->p_parent->p_wait_list, curr_thread_ctx))
	{
		list_remove_node(curr_thread_ctx->p_parent->p_wait_list, curr_node->data);
		//if wait_list of parent is empty, unblock it
		if(!curr_thread_ctx->p_parent->p_wait_list->logicalLength)
		{
			//unblocking parent
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
	if(!ctx->is_main_ctx)
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
	MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)(MyThreadCtx_t*)readyQ.head->data;
	if(thread_to_join_ctx 
		/*check if its a child*/
		&& list_search_node(curr_thread_ctx->p_child_list, thread_to_join_ctx) 
		/*check if not already waiting*/
		&& !list_search_node(curr_thread_ctx->p_wait_list, thread_to_join_ctx))
	{
		list_append(curr_thread_ctx->p_wait_list, (void*)thread_to_join_ctx);
		list_pop_front(&readyQ);
		curr_thread_ctx->state = BLOCKED;
		list_append(&blockedQ, curr_thread_ctx);
		swapcontext(curr_thread_ctx->p_uctx, &_loop_ctx);
		return 0;
	}
	return 1;
}

//TODO test this
void MyThreadJoinAll(void)
{
	MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)readyQ.head->data;
	listNode* child = curr_thread_ctx->p_child_list->head;
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

/////////////////////////////////////////
//Semaphore operations
/////////////////////////////////////////

MySemaphore MySemaphoreInit(int initialValue)
{
	//create and init new sem
	if(initialValue < 0)
		return NULL;
	MySemaphore_t* sem = (MySemaphore_t*)calloc(1, sizeof(MySemaphore_t));
	sem->value = initialValue;
	sem->waitQ = (list*)calloc(1, sizeof(list));
	list_new(sem->waitQ, sizeof(MyThreadCtx_t), NULL/*freeFn*/);
	//add to global sem list
	list_append(&semList, sem);
	return (MySemaphore)sem;
}

void MySemaphoreSignal(MySemaphore sem)
{
	//check if semaphore valid
	if(!list_search_node(&semList, sem))
	{
		return;
	}
	MySemaphore_t* _sem = (MySemaphore_t*)sem;
	_sem->value++;
	if(_sem->value <= 0)
	{
		//remove one proc from waitQ, put it on readyQ
		MyThreadCtx_t* unblocked_thread_ctx = (MyThreadCtx_t*)list_pop_front(_sem->waitQ);
		unblocked_thread_ctx->state = READY;
		list_append(&readyQ, unblocked_thread_ctx);
	}
}

void MySemaphoreWait(MySemaphore sem)
{
	//check if semaphore valid
	if(!list_search_node(&semList, sem))
	{
		return;
	}
	MySemaphore_t* _sem = (MySemaphore_t*)sem;
	_sem->value--;
	if(_sem->value < 0)
	{
		//put the process in wait queue
		MyThreadCtx_t* curr_thread_ctx = (MyThreadCtx_t*)list_pop_front(&readyQ);
		curr_thread_ctx->state = SEM_WAIT;
		list_append(_sem->waitQ, curr_thread_ctx);
		swapcontext(curr_thread_ctx->p_uctx, &_loop_ctx);
	}
}

int MySemaphoreDestroy(MySemaphore sem)
{
	//check if semaphore valid
	if(!list_search_node(&semList, sem))
	{
		return -1;
	}
	//Cleanup the threads in waiting queue
	//delete waiting queue
	//remove semaphore from semList
	MySemaphore_t* _sem = (MySemaphore_t*)sem;
	if(_sem->waitQ->logicalLength)
	{
		return -1;
	}
	free(_sem->waitQ);
	list_remove_node(&semList, sem);
	free(sem);
	return 0;
}

void _cleanup_mythreadlib()
{
	//cleanup all the queues, contexts ...
	//clear global blocked queue
	//clear block queue of every semaphore
	while(blockedQ.logicalLength)
	{
		MyThreadCtx_t* ctx_to_del = (MyThreadCtx_t*)blockedQ.head->data;
		_cleanup_ctx(ctx_to_del);
		list_pop_front(&blockedQ);
	}

	while(semList.logicalLength)
	{
		MySemaphore_t* _sem = (MySemaphore_t*)semList.head->data;
		while(_sem->waitQ->logicalLength)
		{
			MyThreadCtx_t* ctx_to_del = (MyThreadCtx_t*)_sem->waitQ->head->data;
			_cleanup_ctx(ctx_to_del);
			list_pop_front(_sem->waitQ);
		}
		free(_sem);
		list_pop_front(&semList);
	}
}

