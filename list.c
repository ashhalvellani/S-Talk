#include "list.h"

// static arrays to store all heads and nodes
static Node nodes[LIST_MAX_NUM_NODES];
static List lists[LIST_MAX_NUM_HEADS];

// linked lists to store free heads and nodes
static Node* freeNodes = NULL;
static List* freeHeads = NULL;

// flag to check if above LLs are populated only once
static int initialized = 0;

// helper function to initialize pool of nodes and heads
void initialize() {
    if (initialized) {return;}

    // initialize the free heads list as a stack
    freeHeads = &lists[0];
    for (int i = 0; i < LIST_MAX_NUM_HEADS - 1; i++) {
        lists[i].nextList = &lists[i + 1];
    }
    lists[LIST_MAX_NUM_HEADS - 1].nextList = NULL;

    // initialize the free nodes list as a stack
    freeNodes = &nodes[0];
    for (int i = 0; i < LIST_MAX_NUM_NODES - 1; i++) {
        nodes[i].next = &nodes[i + 1];
    }
    nodes[LIST_MAX_NUM_NODES - 1].next = NULL;

    initialized = 1;
}

List* List_create() {
    initialize();

    if (freeHeads == NULL) { return NULL;}  // no free list head

    // get a free head from the pool of heads
    List* newList = freeHeads;
    freeHeads = freeHeads->nextList;

    // initialize the new list
    newList->current = NULL;
    newList->head = NULL; 
    newList->tail = NULL; 
    newList->count = 0;
    newList->currState = LIST_OOB_START;

    return newList;
}

int List_count(List* pList) {
    return pList->count;
}

void* List_first(List* pList) {
    // return NULL if list is empty
    if (pList->head == NULL) {return NULL;}

    pList->current = pList->head;

    return pList->current->item;
}

void* List_last(List* pList) {
    // return NULL if list is empty
    if (pList->head == NULL) {return NULL;}

    pList->current = pList->tail;

    return pList->current->item;
}

void* List_next(List* pList) {
    if (pList->current == NULL && pList->currState == LIST_OOB_END) {
        return NULL;
    }
    // set currState to OOB end if it is the last element
    if (pList->current == pList->tail) {
        pList->current = NULL;
        pList->currState = LIST_OOB_END;
        return NULL;
    }
    // if it is OOB start, current = head
    if (pList->current == NULL && pList->currState == LIST_OOB_START) {
        pList->current = pList->head;
    }
    else {
        pList->current = pList->current->next;
    }

    return pList->current->item;
}

void* List_prev(List* pList) {
    if (pList->current == NULL && pList->currState == LIST_OOB_START) {
        return NULL;
    }
    // set currState to OOB start if it is the first element
    if (pList->current == pList->head) {
        pList->current = NULL;
        pList->currState = LIST_OOB_START;
        return NULL;
    }
    // if it is OOB end, current = tail
    if (pList->current == NULL && pList->currState == LIST_OOB_END) {
        pList->current = pList->tail;
    }
    else {
        pList->current = pList->current->prev;
    }

    return pList->current->item;
}

void* List_curr(List* pList) {
    return (pList->current != NULL) ? pList->current->item : NULL;
}

int List_insert_after(List* pList, void* pItem) {
    if (freeNodes == NULL) {return LIST_FAIL;} // no free node

    // get a free node from the pool of nodes and initialize it
    Node* newNode = freeNodes;
    freeNodes = freeNodes->next;

    newNode->item = pItem;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (pList->head == NULL) {
        // if  list is empty, set this node as both head and tail
        pList->head = newNode;
        pList->tail = newNode;
    } 
    else if (pList->current == NULL && pList->currState == LIST_OOB_START) {
        newNode->next = pList->head;
        pList->head->prev = newNode;
        pList->head = newNode;
    } 
    else if ((pList->count == 1) || 
    (pList->current == pList->tail) ||
    (pList->current == NULL && pList->currState == LIST_OOB_END)) {
        pList->tail->next = newNode;
        newNode->prev = pList->tail;
        pList->tail = newNode;
    }
    else {
        // insert the new node after the current item
        newNode->next = pList->current->next;
        newNode->prev = pList->current;
        pList->current->next->prev = newNode;
        pList->current->next = newNode;
    }
    pList->current = newNode;
    pList->count++;

    return LIST_SUCCESS;
}

int List_insert_before(List* pList, void* pItem) {
    if (freeNodes == NULL) {return LIST_FAIL;} // no free node

    // get a free node from the pool of nodes and initialize it
    Node* newNode = freeNodes;
    freeNodes = freeNodes->next;

    newNode->item = pItem;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (pList->head == NULL) {
        // if  list is empty, set this node as both head and tail
        pList->head = newNode;
        pList->tail = newNode;
    } 
    else if ((pList->count == 1) || 
    (pList->current == pList->head) ||
    (pList->current == NULL && pList->currState == LIST_OOB_START)) {
        newNode->next = pList->head;
        pList->head->prev = newNode;
        pList->head = newNode;
    } 
    else if (pList->current == NULL && pList->currState == LIST_OOB_END) {
        pList->tail->next = newNode;
        newNode->prev = pList->tail;
        pList->tail = newNode;
    }
    else {
        // insert the new node before the current item
        newNode->prev = pList->current->prev;
        newNode->next = pList->current;
        pList->current->prev->next = newNode;
        pList->current->prev = newNode;
    }
    pList->current = newNode;
    pList->count++;

    return LIST_SUCCESS;
}

int List_append(List* pList, void* pItem) {
    if (freeNodes == NULL) {return LIST_FAIL;} // no free node

    // get a free node from the pool of nodes and initialize it
    Node* newNode = freeNodes;
    freeNodes = freeNodes->next;

    newNode->item = pItem;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (pList->head == NULL) {
        // if  list is empty, set this node as both head and tail
        pList->head = newNode;
        pList->tail = newNode;
    } 
    else {
        // add the new node at the end of the list
        pList->tail->next = newNode;
        newNode->prev = pList->tail;
        pList->tail = newNode;
    }
    pList->current = newNode;
    pList->count++;

    return LIST_SUCCESS;
}

int List_prepend(List* pList, void* pItem) {
    if (freeNodes == NULL) {return LIST_FAIL;} // no free node

    // get a free node from the pool of nodes and initialize it
    Node* newNode = freeNodes;
    freeNodes = freeNodes->next;

    newNode->item = pItem;
    newNode->prev = NULL;
    newNode->next = NULL;

    if (pList->head == NULL) {
        // if  list is empty, set this node as both head and tail
        pList->head = newNode;
        pList->tail = newNode;
    } 
    else {
        // add the new node at the start of the list
        newNode->next = pList->head;
        pList->head->prev = newNode;
        pList->head = newNode;
    }
    pList->current = newNode;
    pList->count++;

    return LIST_SUCCESS;
}

void* List_remove(List* pList) {
    // return NULL if current pointer OOB or list empty
    if (pList->current == NULL || pList->count == 0) {return NULL;}

    Node* removedNode = pList->current;
    void* removedItem = removedNode->item;

    if (pList->count == 1) {
        // one item in the list
        pList->head = NULL;
        pList->current = NULL;
        pList->tail = NULL;
        pList->currState = LIST_OOB_END;
    } 
    else if (removedNode == pList->head) {
        // current item is head
        pList->head = removedNode->next;
        pList->current = pList->head;
        pList->head->prev = NULL;
    } 
    else if (removedNode == pList->tail) {
        // current item is tail
        pList->tail = removedNode->prev;
        pList->current = NULL;
        pList->tail->next = NULL;
        pList->currState = LIST_OOB_END;
    } 
    else {
        // current item anywhere else
        if (removedNode->prev != NULL) {
            removedNode->prev->next = removedNode->next;
        }
        if (removedNode->next != NULL) {
            removedNode->next->prev = removedNode->prev;
        }
        pList->current = removedNode->next;
    }
    pList->count--;

    // add removed node back to the pool of free nodes
    removedNode->next = freeNodes;
    freeNodes = removedNode;

    return removedItem;
}

void* List_trim(List* pList) {
    int size = List_count(pList);

    List_last(pList);
    void* const removedItem = List_remove(pList);
    List_last(pList);

    if (size == 1 && removedItem != NULL) {pList->currState = LIST_OOB_START;}

    return removedItem;
}

void List_concat(List* pList1, List* pList2) {
    if (pList2->head == NULL) {return;} // if pList2 is empty, there is nothing to concatenate

    // save pList1's curr
    Node* curr1 = pList1->current;

    if (pList1->head == NULL) {
        // if pList1 is empty, set its properties to pList2's properties
        pList1->head = pList2->head;
        pList1->tail = pList2->tail;
        pList1->count = pList2->count;
    } 
    else {
        // connect pList1's tail to pList2's head
        pList1->tail->next = pList2->head;
        if (pList2->head != NULL) {
            pList2->head->prev = pList1->tail;
        }
        pList1->tail = pList2->tail;
        pList1->count += pList2->count;
    }

    pList1->current = curr1;

    // add pList2's head back to the pool of free heads
    pList2->nextList = freeHeads;
    freeHeads = pList2;

    // reset pList2's properties
    pList2->head = NULL;
    pList2->tail = NULL;
    pList2->current = NULL;
    pList2->currState = LIST_OOB_START;
    pList2->count = 0;
}

void List_free(List* pList, FREE_FN pItemFreeFn){
    Node* currentNode = pList->head;

    // free the items and nodes to the pool of free nodes
    while (currentNode != NULL) {
        Node* nextNode = currentNode->next;
        if (pItemFreeFn != NULL) {(*pItemFreeFn)(currentNode->item);}

        currentNode->next = freeNodes;
        freeNodes = currentNode;
        currentNode = nextNode;
    }

    // add the head to the pool of free heads
    pList->nextList = freeHeads;
    freeHeads = pList;

    // reset pList's properties
    pList->head = NULL;
    pList->tail = NULL;
    pList->current = NULL;
    pList->currState = LIST_OOB_START;
    pList->count = 0;
}

void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg) {
    // return NULL if list empty
    if (pList->head == NULL) {return NULL;}

    // return NULL if current is OOB end
    if (pList->current == NULL && pList->currState == LIST_OOB_END){return NULL;}
    
    // start from head if currrent pointer is OOB start
    if (pList->current == NULL && pList->currState == LIST_OOB_START) {List_first(pList);}

    Node* currentNode = pList->current;

    while (currentNode != NULL) {
        if ((*pComparator)(currentNode->item, pComparisonArg)) {
            // if match found, return item
            pList->current = currentNode;
            return currentNode->item;
        }
        currentNode = currentNode->next;
    }

    // no match, return NULL
    pList->current = NULL;
    pList->currState = LIST_OOB_END;

    return NULL;
}