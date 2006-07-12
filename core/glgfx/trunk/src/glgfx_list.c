
#include "glgfx-config.h"

#include "glgfx_intern.h"

void glgfx_list_new(struct glgfx_list* list) {
  list->tailpred = (struct glgfx_node*) list;
  list->tail     = NULL;
  list->head     = (struct glgfx_node*) &list->tail;
}

bool glgfx_list_isempty(struct glgfx_list* list) {
  return list->tailpred == (struct glgfx_node*) list;
}

void glgfx_list_addhead(struct glgfx_list* list, struct glgfx_node* node) {
  node->succ       = list->head;
  node->pred       = (struct glgfx_node*) &list->head;
  list->head->pred = node;
  list->head       = node;
}

void glgfx_list_addtail(struct glgfx_list* list, struct glgfx_node* node) {
  node->succ           = (struct glgfx_node*) &list->tail;
  node->pred           = list->tailpred;
  list->tailpred->succ = node;
  list->tailpred       = node;
}

void glgfx_list_insert(struct glgfx_list* list, 
		       struct glgfx_node* node, 
		       struct glgfx_node* pred) {
  if (pred != NULL) {
    if (pred->succ != NULL) {
      node->succ       = pred->succ;
      node->pred       = pred;
      pred->succ->pred = node;
      pred->succ       = node;
    }
    else {
      glgfx_list_addtail(list, node);
    }
  }
  else {
    glgfx_list_addhead(list, node);
  }
}


void glgfx_list_enqueue(struct glgfx_list* list, 
			struct glgfx_node* node, 
			struct glgfx_hook* cmp_hook) {
  struct glgfx_node* next;

  GLGFX_LIST_FOR(list, next) {
    if ((long) glgfx_callhook(cmp_hook, node, next) > 0) {
      break;
    }
  }

  node->pred       = next->pred;
  next->pred->succ = node;
  next->pred       = node;
  node->succ       = next;
}


struct glgfx_node* glgfx_list_remove(struct glgfx_node* node) {
  node->pred->succ = node->succ;
  node->succ->pred = node->pred;

  return node;
}

struct glgfx_node* glgfx_list_remhead(struct glgfx_list* list) {
  if (glgfx_list_isempty(list)) {
    return NULL;
  }
   
  return glgfx_list_remove(list->head);
}

struct glgfx_node* glgfx_list_remtail(struct glgfx_list* list) {
  if (glgfx_list_isempty(list)) {
    return NULL;
  }

  return glgfx_list_remove(list->tailpred);
}

size_t glgfx_list_length(struct glgfx_list* list) {
  struct glgfx_node* node;
  size_t length = 0;

  GLGFX_LIST_FOR(list, node) {
    ++length;
  }
  
  return length;
}

void glgfx_list_foreach(struct glgfx_list* list, struct glgfx_hook* node_hook, void* message) {
  struct glgfx_node* node;
  struct glgfx_node* next;

  for (node = list->head; (next = node->succ) != NULL; node = next) {
    glgfx_callhook(node_hook, node, message);
  }
}
