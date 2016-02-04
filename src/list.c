#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
 
#include "list.h"
 
#define printf(...) 

void list_new(list *list, int elementSize, freeFunction freeFn)
{
  printf("list_new:\n");
  assert(elementSize > 0);
  list->logicalLength = 0;
  list->elementSize = elementSize;
  list->head = list->tail = NULL;
  list->freeFn = freeFn;
}
 
void list_destroy(list *list)
{
  printf("list_destroy:\n");
  listNode *current;
  while(list->head != NULL) {
    current = list->head;
    list->head = current->next;

    if(list->freeFn) {
      list->freeFn(current->data);
    }

    //free(current->data);
    free(current);
  }
}
 
void list_prepend(list *list, void *element)
{
  printf("list_prepend:\n");
  listNode *node = malloc(sizeof(listNode));
  node->data = element; //malloc(list->elementSize);
  //memcpy(node->data, element, list->elementSize);

  node->next = list->head;
  list->head = node;

  // first node?
  if(!list->tail) {
    list->tail = list->head;
  }

  list->logicalLength++;
}
 
void list_append(list *list, void *element)
{
  printf("list_append:\n");
  listNode *node = malloc(sizeof(listNode));
  node->data = element; //malloc(list->elementSize);
  node->next = NULL;

  //memcpy(node->data, element, list->elementSize);

  if(list->logicalLength == 0) {
    list->head = list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }

  list->logicalLength++;
}
 
void list_for_each(list *list, listIterator iterator)
{
  printf("list_for_each:\n");
  assert(iterator != NULL);
 
  listNode *node = list->head;
  bool result = TRUE;
  while(node != NULL && result) {
    result = iterator(node->data);
    node = node->next;
  }
}
 
void list_head(list *list, void *element, bool removeFromList)
{
  printf("list_head:\n");
  assert(list->head != NULL);
 
  listNode *node = list->head;
  memcpy(element, node->data, list->elementSize);
 
  if(removeFromList) {
    list->head = node->next;
    list->logicalLength--;
 
    free(node->data);
    free(node);
  }
}
 
void* list_pop_front(list *list)
{
  printf("list_pop_front:\n");
  assert(list->head != NULL);
  listNode *node = list->head;
  list->head = node->next;
  list->logicalLength--;

  //free(node->data);
  void* data = node->data;
  free(node);
  return data;
}

//remove node with this data
//TODO use doubly linked list
void* list_remove_node(list* list, void* data)
{
	printf("list_remove_node:<%p>\n", data);
	assert(list != NULL);
	listNode *node = list->head;
	listNode *prev = NULL;
	while(node != NULL)
	{
		if(node->data == data)
		{
			if(prev)
				prev->next = node->next;
			else
				list->head = node->next;
			list->logicalLength--;
			break;
		}
		prev = node;
		node = node->next;
	}
	free(node);
	return data;
}

//retVal 0 = not found
int list_search_node(list* list, void* data)
{
  	printf("list_search_node:\n");
	assert(list != NULL);
	listNode *node = list->head;
	while(node != NULL && node->data != data)
	{
		node = node->next;
	}
	if(node)
		return 1;
	return 0;
}

void list_tail(list *list, void *element)
{
  assert(list->tail != NULL);
  listNode *node = list->tail;
  memcpy(element, node->data, list->elementSize);
}
 
int list_size(list *list)
{
  return list->logicalLength;
}
