//
// SciRuby is Copyright (c) 2010 - 2012, Ruby Science Foundation
// NMatrix is Copyright (c) 2012, Ruby Science Foundation
//
// Please see LICENSE.txt for additional copyright notices.
//
// == Contributing
//
// By contributing source code to SciRuby, you agree to be bound by
// our Contributor Agreement:
//
// * https://github.com/SciRuby/sciruby/wiki/Contributor-Agreement
//
// == sl_list.cpp
//
// Singly-linked list implementation

/*
 * Standard Includes
 */

#include <ruby.h>

/*
 * Project Includes
 */

#include "types.h"

#include "data/data.h"

#include "sl_list.h"

namespace nm { namespace list {

/*
 * Macros
 */

/*
 * Global Variables
 */
 

/*
 * Forward Declarations
 */

/*
 * Functions
 */

////////////////
// Lifecycle //
///////////////

/*
 * Creates an empty linked list.
 */
LIST* create(void) {
  LIST* list;
  
  //if (!(list = malloc(sizeof(LIST)))) return NULL;
  list = ALLOC( LIST );

  //fprintf(stderr, "    create_list LIST: %p\n", list);

  list->first = NULL;
  return list;
}

/*
 * Deletes the linked list and all of its contents. If you want to delete a
 * list inside of a list, set recursions to 1. For lists inside of lists inside
 *  of the list, set it to 2; and so on. Setting it to 0 is for no recursions.
 */
void del(LIST* list, size_t recursions) {
  NODE* next;
  NODE* curr = list->first;

  while (curr != NULL) {
    next = curr->next;

    if (recursions == 0) {
      //fprintf(stderr, "    free_val: %p\n", curr->val);
      free(curr->val);
      
    } else {
      //fprintf(stderr, "    free_list: %p\n", list);
      del((LIST*)curr->val, recursions - 1);
    }

    free(curr);
    curr = next;
  }
  //fprintf(stderr, "    free_list: %p\n", list);
  free(list);
}

/*
 * Documentation goes here.
 */
void mark(LIST* list, size_t recursions) {
  NODE* next;
  NODE* curr = list->first;

  while (curr != NULL) {
    next = curr->next;
    
    if (recursions == 0) {
    	rb_gc_mark(*((VALUE*)(curr->val)));
    	
    } else {
    	mark((LIST*)curr->val, recursions - 1);
    }
    
    curr = next;
  }
}

///////////////
// Accessors //
///////////////

/* 
 * Given a list and a key/value-ptr pair, create a node (and return that node).
 * If NULL is returned, it means insertion failed.
 * If the key already exists in the list, replace tells it to delete the old
 * value and put in your new one. !replace means delete the new value.
 */
NODE* insert(LIST* list, bool replace, size_t key, void* val) {
  NODE *ins;

  if (list->first == NULL) {
  	// List is empty
  	
    //if (!(ins = malloc(sizeof(NODE)))) return NULL;
    ins = ALLOC(NODE);
    ins->next             = NULL;
    ins->val              = val;
    ins->key              = key;
    list->first           = ins;
    
    return ins;

  } else if (key < list->first->key) {
  	// Goes at the beginning of the list
  	
    //if (!(ins = malloc(sizeof(NODE)))) return NULL;
    ins = ALLOC(NODE);
    ins->next             = list->first;
    ins->val              = val;
    ins->key              = key;
    list->first           = ins;
    
    return ins;
  }

  // Goes somewhere else in the list.
  ins = find_nearest_from(list->first, key);

  if (ins->key == key) {
    // key already exists
    if (replace) {
      free(ins->val);
      ins->val = val;
      
    } else {
    	free(val);
    }
    
    return ins;

  } else {
  	return insert_after(ins, key, val);
  }
}

/*
 * Documentation goes here.
 */
NODE* insert_after(NODE* node, size_t key, void* val) {
  NODE* ins;

  //if (!(ins = malloc(sizeof(NODE)))) return NULL;
  ins = ALLOC(NODE);

  // insert 'ins' between 'node' and 'node->next'
  ins->next  = node->next;
  node->next = ins;

  // initialize our new node
  ins->key   = key;
  ins->val   = val;

  return ins;
}

/*
 * Analog functions list_insert but this insert copy of value.
 */
NODE* insert_with_copy(LIST *list, size_t key, void *val, size_t size) {
  void *copy_val = ALLOC_N(char, size);
  memcpy(copy_val, val, size);


  return insert(list, false, key, copy_val);
}
/*
 * Returns the value pointer (not the node) for some key. Note that it doesn't
 * free the memory for the value stored in the node -- that pointer gets
 * returned! Only the node is destroyed.
 */
void* remove(LIST* list, size_t key) {
  NODE *f, *rm;
  void* val;

  if (!list->first || list->first->key > key) { // empty list or def. not present
  	return NULL;
  }

  if (list->first->key == key) {
    val = list->first->val;
    rm  = list->first;
    
    list->first = rm->next;
    free(rm);
    
    return val;
  }

  f = find_preceding_from(list->first, key);
  if (!f || !f->next) { // not found, end of list
  	return NULL;
  }

  if (f->next->key == key) {
    // remove the node
    rm      = f->next;
    f->next = rm->next;

    // get the value and free the memory for the node
    val = rm->val;
    free(rm);

    return val;
  }

  return NULL; // not found, middle of list
}


/*
 * Recursive removal of lists that may contain sub-lists. Stores the value ultimately removed in rm.
 *
 * FIXME: Could be made slightly faster by using a variety of find which also returns the previous node. This way,
 * FIXME: we can remove directly instead of calling remove() and doing the search over again.
 */
bool remove_recursive(LIST* list, const size_t* coords, const size_t* offset, size_t r, const size_t& dim, void* rm) {

  if (r < dim-1) { // nodes here are lists
    // find the current coordinates in the list
    NODE* n = find(list, coords[r] + offset[r]);

    if (n) {
      // from that sub-list, call remove_recursive.
      bool remove_parent = remove_recursive(reinterpret_cast<LIST*>(n->val), coords, offset, r+1, dim, rm);

      if (remove_parent) { // now empty -- so remove the sub-list
        free(remove(list, coords[r] + offset[r]));
      }
    }

  } else { // nodes here are not lists, but actual values
    rm = remove(list, coords[r] + offset[r]);
  }

  if (!list->first) return true; // if current list is now empty, signal its removal

  return false;
}

///////////
// Tests //
///////////


/////////////
// Utility //
/////////////

/*
 * Find some element in the list and return the node ptr for that key.
 */
NODE* find(LIST* list, size_t key) {
  NODE* f;
  if (!list->first) {
  	// empty list -- does not exist
  	return NULL;
  }

  // see if we can find it.
  f = find_nearest_from(list->first, key);
  
  if (!f || f->key == key) {
  	return f;
  }
  
  return NULL;
}

/*
 * Finds the node that should go before whatever key we request, whether or not
 * that key is present.
 */
NODE* find_preceding_from(NODE* prev, size_t key) {
  NODE* curr = prev->next;

  if (!curr || key <= curr->key) {
  	return prev;
  	
  } else {
  	return find_preceding_from(curr, key);
  }
}

/*
 * Finds the node or, if not present, the node that it should follow. NULL
 * indicates no preceding node.
 */
NODE* find_nearest(LIST* list, size_t key) {
  return find_nearest_from(list->first, key);
}

/*
 * Finds a node or the one immediately preceding it if it doesn't exist.
 */
NODE* find_nearest_from(NODE* prev, size_t key) {
  NODE* f;

  if (prev && prev->key == key) {
  	return prev;
  }

  f = find_preceding_from(prev, key);

  if (!f->next) {
  	return f;
  	
  } else if (key == f->next->key) {
  	return f->next;
  	
  } else {
  	return prev;
  }
}

/////////////////////////
// Copying and Casting //
/////////////////////////


/*
 * Copy the contents of a list.
 */
template <typename LDType, typename RDType>
void cast_copy_contents(LIST* lhs, const LIST* rhs, size_t recursions) {
  NODE *lcurr, *rcurr;

  if (rhs->first) {
    // copy head node
    rcurr = rhs->first;
    lcurr = lhs->first = ALLOC( NODE );

    while (rcurr) {
      lcurr->key = rcurr->key;

      if (recursions == 0) {
      	// contents is some kind of value

        lcurr->val = ALLOC( LDType );

        *reinterpret_cast<LDType*>(lcurr->val) = *reinterpret_cast<RDType*>( rcurr->val );

      } else {
      	// contents is a list

        lcurr->val = ALLOC( LIST );

        cast_copy_contents<LDType, RDType>(
          reinterpret_cast<LIST*>(lcurr->val),
          reinterpret_cast<LIST*>(rcurr->val),
          recursions-1
        );
      }

      if (rcurr->next) {
      	lcurr->next = ALLOC( NODE );

      } else {
      	lcurr->next = NULL;
      }

      lcurr = lcurr->next;
      rcurr = rcurr->next;
    }

  } else {
    lhs->first = NULL;
  }
}

}} // end of namespace nm::list

extern "C" {

  /*
   * C access for copying the contents of a list.
   */
  void nm_list_cast_copy_contents(LIST* lhs, const LIST* rhs, dtype_t lhs_dtype, dtype_t rhs_dtype, size_t recursions) {
    LR_DTYPE_TEMPLATE_TABLE(nm::list::cast_copy_contents, void, LIST*, const LIST*, size_t);

    ttable[lhs_dtype][rhs_dtype](lhs, rhs, recursions);
  }

  /*
   * Sets up a hash with an appropriate default values. That means that if recursions == 0, the default value is default_value,
   * but if recursions == 1, the default value is going to be a hash with default value of default_value, and if recursions == 2,
   * the default value is going to be a hash with default value of hash with default value of default_value, and so on.
   * In other words, it's recursive.
   */
  static VALUE empty_list_to_hash(const dtype_t dtype, size_t recursions, VALUE default_value) {
    VALUE h = rb_hash_new();
    if (recursions) {
      RHASH_IFNONE(h) = empty_list_to_hash(dtype, recursions-1, default_value);
    } else {
      RHASH_IFNONE(h) = default_value;
    }
    return h;
  }


  /*
   * Copy a list to a Ruby Hash
   */
  VALUE nm_list_copy_to_hash(const LIST* l, const dtype_t dtype, size_t recursions, VALUE default_value) {

    // Create a hash with default values appropriately specified for a sparse matrix.
    VALUE h = empty_list_to_hash(dtype, recursions, default_value);

    if (l->first) {
      NODE* curr = l->first;

      while (curr) {

        size_t key = curr->key;

        if (recursions == 0) { // content is some kind of value
          rb_hash_aset(h, INT2FIX(key), rubyobj_from_cval(curr->val, dtype).rval);
        } else { // content is a list
          rb_hash_aset(h, INT2FIX(key), nm_list_copy_to_hash(reinterpret_cast<const LIST*>(curr->val), dtype, recursions-1, default_value));
        }

        curr = curr->next;

      }

    }

    return h;
  }


} // end of extern "C" block

