
// Amiga-like lists

struct node {
    struct node* succ;
    struct node* pred;
};

struct list {
    struct node* head;
    struct node* tail;
    struct node* tailpred;
};

#define LIST_INITIALIZER { 

static inline void list_new(struct list* list) {
  list->head     = (struct node*) &list->tail;
  list->tail     = NULL;
  list->tailpred = (struct node*) list;
}

static inline int list_isempty(struct list* list) {
  return list->tailpred == (struct node*) list;
}

static inline void list_addhead(struct list* list, struct node* node) {
  node->succ       = list->head;
  node->pred       = (struct node*) &list->head;
  list->head->pred = node;
  list->head       = node;
}

static inline void list_addtail(struct list* list, struct node* node) {
  node->succ           = (struct node*) &list->tail;
  node->pred           = list->tailpred;
  list->tailpred->succ = node;
  list->tailpred       = node;
}

static inline struct node* list_remove(struct node* node) {
  node->pred->succ = node->succ;
  node->succ->pred = node->pred;
  return node;
}

static inline struct node* list_remhead(struct list* list) {
  if (list_isempty(list)) {
    return NULL;
  }
   
  return list_remove(list->head);
}

static inline struct node* list_remtail(struct list* list) {
  if (list_isempty(list)) {
    return NULL;
  }

  return list_remove(list->tailpred);
}


#define LIST_FOR(l, n) for (n = (void*) ((struct list*) (l))->head;	\
			    ((struct node*) (n))->succ != NULL;	\
			    n = (void*) ((struct node*) (n))->succ)
