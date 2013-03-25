// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "linearizer.h"
#include <limits.h>

namespace sum {

struct Node {
  // The operation stored in this node.
  Operation* operation;
  // The operation matching the operation stored in this node, e.g. an insert
  // operation if this node contains a remove operation. The matching operation
  // of a null-remove operation is the remove operation itself.
  Node* matching_op;
  // The next node in the doubly-linked list.
  Node* next;
  // The node which precedes this node in the doubly-linked list.
  Node* prev;
  // The costs of this operation calculated by the cost function.
  int costs;
  // The order in which this node was selected, i.e. removed from the
  // doulby-linked list.
  int order;
  // The order for the selectable calculation.
  int selectable_order;
  // The first response of any operation in the first overlap group at the time
  // this operation was selected.
  uint64_t latest_lin_point;
  // This flag indicates that the matching insert operations has already been
  // added to the linearization, only necessary for remove operations.
  bool insert_added;
};

struct Operations {
  // Array of insert operations
  Node** insert_ops;
  Node** insert_ops_order;
  // Doubly-linked list of insert operations.
  Node* insert_list;
  // The order used to calculate the selectable function.
  Node* insert_order;
  // Number of insert operations.
  int num_insert_ops;
  // Number of insert operations + null returns.
  int num_insert_ops_order;
  // Array of remove operations.
  Node** remove_ops;
  Node** remove_ops_order;
  // Doubly-linked list of remove operations.
  Node* remove_list;
  // The order used to calculate the selectable function.
  Node* remove_order;
  // Number of remove operations
  int num_remove_ops;
};

void match_operations(Node** insert_ops, int num_insert_ops,
                      Node** remove_ops, int num_remove_ops);
void create_insert_and_remove_array(Operations* operations, Operation** ops, int num_ops);
void create_doubly_linked_lists(Node* head, Node** ops, int num_ops);
void initialize(Operations* operations, Operation** ops, int num_ops, Order** order);
void create_orders (Operations* operations, Order** order, int num_ops);
int get_remove_costs(Operations* operations, Node* remove_op);

int compare_orders_by_start_time(const void* left, const void* right) {
  return (*(Order**) left)->operation->start() > (*(Order**) right)->operation->start();
}

int compare_operations_by_start_time(const void* left, const void* right) {
  return (*(Node**) left)->operation->start() > (*(Node**) right)->operation->start();
}

int compare_operations_by_value(const void* left, const void* right) {
  return (*(Node**) left)->operation->value() > (*(Node**) right)->operation->value();
}

int compare_operations_by_order(const void* left, const void* right) {
  return (*(Node**) left)->order > (*(Node**) right)->order;
}

void remove_from_list(Node* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = NULL;
  node->prev = NULL;
}

void insert_to_list(Node* node, Node* predecessor) {
  node->prev = predecessor;
  node->next = predecessor->next;
  predecessor->next->prev = node;
  predecessor->next = node;
}

int calculate_cost_increase_insert
(Operations* operations, Node* insert_op, Node* op, int cost_decrease, uint64_t last_start) {
  int cost_increase = 0;
  Node* tmp = operations->insert_order->next;

  while (tmp->operation != op->operation) {
    
    if (tmp->matching_op->operation->start() >
        insert_op->matching_op->operation->end() &&
        tmp->operation->end() < last_start) {

      // We only count operations operations which end before the last
      // of the cost-decreasing operations is started.
      cost_increase++;
    }

    tmp = tmp->next;
  }

  return cost_increase;
}

bool is_selectable_insert (Operations* operations, Node* insert_op, uint64_t first_end) {

  int cost_increase = 0;
  int cost_decrease = 0;
//    bool overlap = false;
  uint64_t last_start = 0;
  bool cost_decrease_found = false;

  Node* op = operations->insert_order->next;

  while (op != operations->insert_order) {
   
    if (op->operation == insert_op->operation) {
      // Do nothing for the operation itself.
    } else if (op->operation->is_null_return() 
        && op->operation->start() > first_end
        && op->matching_op->order < insert_op->matching_op->order
        ) {

      // Consider null-return dequeues.
      cost_decrease++;
      cost_decrease_found = true;
      if (op->operation->start() > last_start) {
        last_start = op->operation->start();
      }
    } else if (op->operation->start() > insert_op->operation->end()) {
      // The remove_op strictly precedes op. We can stop the calculation.
      break;
    } else if (op->operation->start() > first_end &&
        op->matching_op->order <
        insert_op->matching_op->order) {
      // If insert_op is linearized after op, then the costs of insert_op
      // would be decreased by one.
      cost_decrease++;
      cost_decrease_found = true;
      if (op->operation->start() > last_start) {
        last_start = op->operation->start();
      }
    } else if (op->matching_op->order >
        insert_op->matching_op->order) {
      // If remove_op is linearized after op, then the costs of op would be
      // increased by one.
      
      if (!cost_decrease_found) {
        // This op is not preceded by a sequence of operations which decrease
        // the costs of remove_op. We do nothing.
      } else {
        // This is the first operation which would have increased costs after
        // a sequence of operations which decrease the costs of remove_op.
        // Check if this sequence was enough to make remove_op unselectable.
        // We count the number of operations which precede op, which would
        // increase the costs, and which respond before the last start of a
        // cost-decreasing operation. If this number is higher than the number
        // of cost-decreasing operations, then remove_op is not selectable.

        cost_decrease_found = false;

        cost_increase = calculate_cost_increase_insert(
            operations, insert_op, op, cost_decrease, last_start);

        if (cost_decrease >= cost_increase) {
          return false;
        }
      }
    }

    op = op->next;
  }

  if (cost_decrease_found) {
    // The operation op is either the end of the list, or the first operation
    // which does not overlap with insert_op.
    cost_increase = calculate_cost_increase_insert(
        operations, insert_op, op, cost_decrease, last_start);

    if (cost_decrease >= cost_increase) {
      return false;
    } else {
      return true;
    }
  } else {
    return true;
  }

//  int cost_increase = 0;
//  int cost_decrease = 0;
//
//  Node* op = operations->insert_order->next;
//
//  while (op != operations->insert_order) {
//
//    if (op == insert_op) {
//      // Do nothing.
//    } else if (op->operation->is_null_return() 
//        && op->operation->start() > first_end
//        && op->matching_op->order < insert_op->matching_op->order
//        ) {
//
//      cost_decrease++;
//      if (cost_decrease >= cost_increase) {
//        return false;
//      }
//    } else if (op->operation->start() > insert_op->operation->end()) {
//      // Upper bound for the calculation.
//      break;
//    } else if(op->matching_op->order >
//        insert_op->matching_op->order) {
//
//      cost_increase++;
//    } else if (op->operation->start() > first_end &&
//        op->matching_op->order <
//        insert_op->matching_op->order) {
//
//      cost_decrease++;
//      if (cost_decrease >= cost_increase) {
//        return false;
//      }
//    }
//
//    op = op->next;
//  }
//
//  return true;
}

int calculate_cost_increase_remove
(Operations* operations, Node* remove_op, Node* op, int cost_decrease, uint64_t last_start) {
  int cost_increase = 0;
  Node* tmp = operations->remove_order->next;

  while (tmp->operation != op->operation) {
    
    if (tmp->matching_op->operation->start() >
        remove_op->matching_op->operation->end() &&
        tmp->operation->end() < last_start) {

      // We only count operations operations which end before the last
      // of the cost-decreasing operations is started.
      cost_increase++;
    }

    tmp = tmp->next;
  }

  return cost_increase;
}

bool is_selectable_remove (Operations* operations, Node* remove_op, uint64_t first_end) {

  if (remove_op->operation->is_null_return()) {

    int costs = get_remove_costs(operations, remove_op);

    if (costs == 0) {
      // A remove operation is always selectable when its costs are zero.
      return true;
    }
    
    int possible_costs = costs;

    Node* op = operations->remove_order->next;

    Node* insert_op = operations->insert_order->next;
    bool done = false;

    while (!done && op != operations->remove_order) {

      while (insert_op != operations->insert_order && insert_op->order < op->order) {
        // Iterate over all insert operations which precede the remove operation
        // op in the last linearization.

        if (insert_op->operation->end() < remove_op->operation->start()) {
          // This insert operation is already considered in the costs by the
          // cost function.
        } else if (insert_op->operation->start() > remove_op->operation->end()) {
          // This insert operation does not overlap with the remove operation
          // anymore. We are done.
          done = true;
          break;
        } else {
          // If remove_op is linearized after op, then insert_op would precede
          // remove_op and the costs of remove_op would increase by 1.
          possible_costs++;
        }
        insert_op = insert_op->next;
      }

      if (done) {
        break;
      }

      if (op == remove_op) {
        // Do nothing.
      } else if (op->operation->start() > remove_op->operation->end()) {
        // This remove operation does not overlap with the remove operation
        // anymore. We are done.
        break;
      } else {
        // The costs of a null-return remove operation decrement with ever remove
        // operation.
        possible_costs--;
        if (possible_costs < costs) {
          return false;
        }
      }
    }

    return true;
  } else { // if the operation is not a null return

    int cost_increase = 0;
    int cost_decrease = 0;
//    bool overlap = false;
    uint64_t last_start = 0;
    bool cost_decrease_found = false;

    Node* op = operations->remove_order->next;

    while (op != operations->remove_order) {
     
      if (op->operation == remove_op->operation) {
        // Do nothing for the operation itself.
      } else if (op->operation->start() > remove_op->operation->end()) {
        // The remove_op strictly precedes op. We can stop the calculation.
        break;
      } else if (op->operation->start() > first_end &&
          op->matching_op->operation->end() <
          remove_op->matching_op->operation->start()) {
        // If remove_op is linearized after op, then the costs of remove_op
        // would be decreased by one.
        cost_decrease++;
        cost_decrease_found = true;
        if (op->operation->start() > last_start) {
          last_start = op->operation->start();
        }
      } else if (op->matching_op->operation->start() >
          remove_op->matching_op->operation->end()) {
        // If remove_op is linearized after op, then the costs of op would be
        // increased by one.
        
        if (!cost_decrease_found) {
          // This op is not preceded by a sequence of operations which decrease
          // the costs of remove_op. We do nothing.
        } else {
          // This is the first operation which would have increased costs after
          // a sequence of operations which decrease the costs of remove_op.
          // Check if this sequence was enough to make remove_op unselectable.
          // We count the number of operations which precede op, which would
          // increase the costs, and which respond before the last start of a
          // cost-decreasing operation. If this number is higher than the number
          // of cost-decreasing operations, then remove_op is not selectable.

          cost_decrease_found = false;

          cost_increase = calculate_cost_increase_remove(
              operations, remove_op, op, cost_decrease, last_start);

          if (cost_decrease >= cost_increase) {
            return false;
          }
        }
      }

      op = op->next;
    }

    if (cost_decrease_found) {
      cost_increase = calculate_cost_increase_remove(
          operations, remove_op, op, cost_decrease, last_start);

      if (cost_decrease >= cost_increase) {
        return false;
      } else {
        return true;
      }
    } else {
      return true;
    }
  }
}


///
// This function calculates the costs of insert operations.
//
// operations All operations.
// insert_op The insert operation the costs are calculated for.
int get_insert_costs(Operations* operations, Node* insert_op) {

  Node* matching_remove = insert_op->matching_op;
  int costs = 0;
  Node* next_op = operations->remove_list->next;
  while (next_op != operations->remove_list) {

    if(!next_op->operation->is_null_return()) {

      if (next_op->order < matching_remove->order) {
        costs++;
      } else {
        break;
      }

      next_op = next_op->next;
    } else {
      // Null returns do not affect the costs of insert operations. We remove
      // them from the list.
      next_op->prev->next = next_op->next;
      next_op->next->prev = next_op->prev;

      next_op = next_op->next;
    }
  }
  assert(costs >= 0);
  return costs;
}

///
// This function calculates the costs of remove operations.
//
// operations All operations.
// remove_op The remove operation the costs are calculated for.
int get_remove_costs(Operations* operations, Node* remove_op) {

  Node* matching_insert = remove_op->matching_op;
  remove_op->costs = 0;
  Node* next_op = operations->insert_list->next;
  while (next_op != operations->insert_list) {

    // No further insert operations can be found which strictly precede the
    // matching insert operation.
    if (next_op->operation->start() > matching_insert->operation->start()) {
      break;
    }
    // The operation op strictly precedes matching_insert. Costs increase by
    // one.
    if (next_op->operation->end() < matching_insert->operation->start()) {
      remove_op->costs++;
    }

    next_op = next_op->next;
  }
  assert(remove_op->costs >= 0);
  return remove_op->costs;
}

void linearize_insert_ops(Operations* operations) {

  qsort(operations->remove_ops, operations->num_remove_ops, sizeof(Node*), compare_operations_by_order);
  operations->remove_list = new Node();
  operations->remove_list->operation = NULL;
  create_doubly_linked_lists(operations->remove_list,
      operations->remove_ops, operations->num_remove_ops);

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list,
      operations->insert_ops, operations->num_insert_ops);

  operations->insert_order = new Node();
  operations->insert_order->operation = NULL;
  create_doubly_linked_lists(operations->insert_order,
      operations->insert_ops_order, operations->num_insert_ops_order);

  int next_order = 0;

  while (operations->insert_list->next != operations->insert_list) {

    Node* op = operations->insert_list->next;
    uint64_t first_end = op->operation->end();

    Node* tmp = op;
    Node* minimal_costs_op = op;
    // Calculate the end of the first overlap group.
    while (tmp != operations->insert_list && tmp->operation->start() <= first_end) {

      if (tmp->operation->end() < first_end) {
        first_end = tmp->operation->end();
        // The operation with the first end is the initial minimal_costs_op
        // because it is guaranteed to be selectable.
        minimal_costs_op = tmp;
      }

      tmp = tmp->next;
    }

    int minimal_costs = -1;

    // Only operations which start before the first insert operation ends are
    // within the first overlap group.
    while (op != operations->insert_list && op->operation->start() <= first_end) {

      if (op == minimal_costs_op) {
        // This op is already the minimal_costs_op.
      } else if (op->matching_op->operation->start() >
          minimal_costs_op->matching_op->operation->end()) {
        // This op cannot have lower costs than the minimal_costs_op.
      } else if (op->matching_op->operation->end() <
          minimal_costs_op->matching_op->operation->start()
          && is_selectable_insert(operations, op, first_end)) {
        minimal_costs_op = op;
        minimal_costs = -1;
        // This op has definitely lower costs than the minimal_costs_op.
      } else if (is_selectable_insert(operations, op, first_end)) {

        if (minimal_costs == -1) {
          // We have not yet calculated the minimal costs.
          minimal_costs = get_insert_costs(operations, minimal_costs_op);
        }
        int costs = get_insert_costs(operations, op);
        if (costs < minimal_costs) {

          minimal_costs = costs;
          minimal_costs_op = op;
        } else if (costs == minimal_costs) {
          fprintf(stderr,
              "Two insert operations have the same costs, this should not happen.\n");
          exit(7);
        }
      }
      op = op->next;
    }

//    int costs = get_insert_costs(operations, minimal_costs_op);
//    if (costs != 0) {
//      fprintf(stderr, "Costs for +%"PRIu64": %d\n", minimal_costs_op->operation->value(), costs);
//    }

    Node* temp = operations->insert_order->next;
    while (temp != operations->insert_order) {

      // Remove null-return dequeues from the insert_order list if it does not
      // influence the selectability of other insert operations anymore.
      if (temp->operation->is_null_return() && temp->operation->end() < first_end) {

        Node* next = temp->next;
        remove_from_list(temp);
        temp = next;
      } else if (temp->operation == minimal_costs_op->operation) {

        temp->order = next_order;
        remove_from_list(temp);
        break;
      } else {
        temp = temp->next;
      }
    }

    // Remove the operation with minimal costs from the list of unselected
    // operations.
    remove_from_list(minimal_costs_op);

    // Assign the order index to the removed operation.
    minimal_costs_op->order = next_order;
    next_order++;
    // The first response of any operation in the first overlap group is the
    // last possible linearization point of the operation.
    minimal_costs_op->latest_lin_point = first_end;

    // Remove the matching insert operation from its list, but only if the
    // minimal_cost_op is not a null-return remove operation.
    if (minimal_costs_op != minimal_costs_op->matching_op) {
      remove_from_list(minimal_costs_op->matching_op);
    }
  }

  delete(operations->remove_list);
  operations->remove_list = NULL;
  delete(operations->insert_list);
  operations->insert_list = NULL;
  delete(operations->insert_order);
  operations->insert_order = NULL;

}

void linearize_remove_ops(Operations* operations) {

  // The resulting remove linearization.

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list,
      operations->insert_ops, operations->num_insert_ops);

  operations->remove_list = new Node();
  operations->remove_list->operation = NULL;
  create_doubly_linked_lists(operations->remove_list,
      operations->remove_ops, operations->num_remove_ops);

  operations->remove_order = new Node();
  operations->remove_order->operation = NULL;
  create_doubly_linked_lists(operations->remove_order,
      operations->remove_ops_order, operations->num_remove_ops);

  operations->insert_order = new Node();
  operations->insert_order->operation = NULL;
  create_doubly_linked_lists(operations->insert_order,
      operations->insert_ops_order, operations->num_insert_ops_order);

  // We need to adjust the start time of null-return remove operations to be
  // able to calculate the cost-function for these operations correctly. See
  // test case "test_null_return_start_time".
  uint64_t latest_selected_invocation = 0;

  int next_order = 0;
  while (operations->remove_list->next != operations->remove_list) {

    Node* op = operations->remove_list->next;
    uint64_t first_end = op->operation->end();

    Node* tmp = op;
    Node* minimal_costs_op = op;
    // Calculate the end of the first overlap group.
    while (tmp != operations->remove_list && tmp->operation->start() <= first_end) {

      if (tmp->operation->end() < first_end) {
        first_end = tmp->operation->end();
        // The remove operation with the first response is the initial
        // minimal_costs_op because it is guaranteed to be selectable.
        minimal_costs_op = tmp;
      }

      tmp = tmp->next;
    }

    int minimal_costs = -1;

    // Only operations which start before the first remove operation ends are
    // within the first overlap group.
    while (op != operations->remove_list && op->operation->start() <= first_end) {

      // We adjust the invocation time of null-return operations.
      if (op->operation->is_null_return()) {
        if (op->operation->start() < latest_selected_invocation) {
          
          op->operation->adjust_start(latest_selected_invocation);
        }
      }

      if (op == minimal_costs_op) {
        // This op is already the minimal_costs_op.
      } else if (op->matching_op->operation->start() >
          minimal_costs_op->matching_op->operation->end()) {
        // This op cannot have lower costs than the minimal_costs_op.
      } else if (op->matching_op->operation->end() <
          minimal_costs_op->matching_op->operation->start()
          && is_selectable_remove(operations, op, first_end)) {
        minimal_costs_op = op;
        minimal_costs = -1;
        // This op has definitely lower costs than the minimal_costs_op.
      } else if (is_selectable_remove(operations, op, first_end)) {

        if (minimal_costs == -1) {
          // We have not yet calculated the minimal costs.
          minimal_costs = get_remove_costs(operations, minimal_costs_op);
        }
        int costs = get_remove_costs(operations, op);
        if (costs < minimal_costs) {

          minimal_costs = costs;
          minimal_costs_op = op;
        } else if (costs == minimal_costs && op->operation->is_null_return()) {
          minimal_costs = costs;
          minimal_costs_op = op;
        } else if (costs == minimal_costs && minimal_costs_op->operation->is_null_return()) {
          // Do nothing. We prefer null-returns.
        } else if (costs == minimal_costs &&
            op->matching_op->operation->end() <
            minimal_costs_op->matching_op->operation->end()) {

          minimal_costs_op = op;
        }
      }
      op = op->next;
    }

    if (latest_selected_invocation < minimal_costs_op->operation->start()) {
      latest_selected_invocation = minimal_costs_op->operation->start();
    }

//    int costs = get_remove_costs(operations, minimal_costs_op);
//    if (costs != 0) {
//      fprintf(stderr, "Costs for +%"PRIu64": %d\n", minimal_costs_op->operation->value(), costs);
//    }

    tmp = operations->remove_order->next;
    while (tmp != operations->remove_order) {

      if (tmp->operation == minimal_costs_op->operation) {

        tmp->order = next_order;
        remove_from_list(tmp);
        remove_from_list(tmp->matching_op);
        break;
      }
      tmp = tmp->next;
    }

    // Remove the operation with minimal costs from the list of unselected
    // operations.
//    fprintf(stderr, "minimal_costs_op: %"PRId64"\n", minimal_costs_op->operation->value());
    remove_from_list(minimal_costs_op);

    // Assign the order index to the removed operation.
    minimal_costs_op->order = next_order;
    next_order++;
    // The first response of any operation in the first overlap group is the
    // last possible linearization point of the operation.
    minimal_costs_op->latest_lin_point = first_end;

    // Remove the matching insert operation from its list, but only if the
    // minimal_cost_op is not a null-return remove operation.
    if (minimal_costs_op != minimal_costs_op->matching_op) {
      remove_from_list(minimal_costs_op->matching_op);
    }
  }

  delete(operations->remove_list);
  operations->remove_list = NULL;
  delete(operations->insert_list);
  operations->insert_list = NULL;
  delete(operations->remove_order);
  operations->remove_order = NULL;
  delete(operations->insert_order);
  operations->insert_order = NULL;
}

void adjust_start_times(Operations* operations) {

  for (int i = 0; i < operations->num_remove_ops; i++) {

    Operation* remove_op = operations->remove_ops[i]->operation;
    if (!remove_op->is_null_return()) {

      Operation* insert_op = operations->remove_ops[i]->matching_op->operation;

      if (remove_op->start() < insert_op->start()) {
        remove_op->adjust_start(insert_op->start());
      }
      if (insert_op->end() > remove_op->end()) {
        insert_op->adjust_end(remove_op->end());
      }
    }
  }
}

void initialize(Operations* operations, Operation** ops, int num_ops, Order** order) {

  create_insert_and_remove_array(operations, ops, num_ops);
  match_operations(operations->insert_ops, operations->num_insert_ops,
                   operations->remove_ops, operations->num_remove_ops);
  adjust_start_times(operations);
  create_orders(operations, order, num_ops);

  qsort(operations->insert_ops, operations->num_insert_ops,
      sizeof(Node*), compare_operations_by_start_time);

  qsort(operations->remove_ops, operations->num_remove_ops,
      sizeof(Node*), compare_operations_by_start_time);
}

void match_operations(Node** insert_ops, int num_insert_ops,
    Node** remove_ops, int num_remove_ops) {

  qsort(insert_ops, num_insert_ops, sizeof(Node*),
      compare_operations_by_value);

  qsort(remove_ops, num_remove_ops, sizeof(Node*),
      compare_operations_by_value);

  int insert_index = 0;

  int i = 0;
  for (int remove_index = 0; remove_index < num_remove_ops; remove_index++) {

    int64_t value = remove_ops[remove_index]->operation->value();

    // If the remove operation is not a null-return.
    if (!remove_ops[remove_index]->operation->is_null_return()) {
      while (insert_ops[insert_index]->operation->value() < value) {
        fprintf(stderr, "insert without remove: %"PRIu64" because of value %"PRIu64"\n",
          insert_ops[insert_index]->operation->value(), value);
        insert_ops[insert_index]->matching_op = NULL;
        insert_index++;
        if (insert_index >= num_insert_ops) {
          fprintf(stderr,
              "FATAL13: The value %"PRId64" gets removed but never inserted.\n", value);
          exit(13);
        }
      }

      if (insert_ops[insert_index]->operation->value() != value) {
        fprintf(stderr,
            "FATAL14: The value %"PRId64" gets removed but never inserted.\n", value);
        exit(14);

      }

      i++;

      insert_ops[insert_index]->matching_op =
        remove_ops[remove_index];
      remove_ops[remove_index]->matching_op =
        insert_ops[insert_index];

      insert_index++;
    }
    else {
      remove_ops[remove_index]->matching_op =
        remove_ops[remove_index];
      remove_ops[remove_index]->insert_added = true;
    }
  }
}

void destroy_insert_and_remove_array(Operations* operations) {
  for (int i = 0; i < operations->num_insert_ops; i++) {
    delete (operations->insert_ops[i]);
  }
  for (int i = 0; i < operations->num_remove_ops; i++) {
    delete (operations->remove_ops[i]);
  }
  delete (operations->insert_ops);
  delete (operations->remove_ops);
}

void create_insert_and_remove_array(Operations* operations, Operation** ops, int num_ops) {
  // Create two arrays which contain the insert operations and the remove
  // operations.
  operations->insert_ops = new Node*[num_ops];
  operations->remove_ops = new Node*[num_ops];

  int next_insert = 0;
  int next_remove = 0;
  for (int i = 0; i < num_ops; i++) {

    Node* node = new Node();
    node->operation = ops[i];
    node->costs = -1;
    node->insert_added = false;
    if (ops[i]->type() == Operation::INSERT) {
      operations->insert_ops[next_insert] = node;
      next_insert++;
    } else {
      operations->remove_ops[next_remove] = node;
      next_remove++;
    }
  }

  assert(next_insert + next_remove == num_ops);
  operations->num_insert_ops = next_insert;
  operations->num_remove_ops = next_remove;
//  printf("Arrays created; %d insert operations and %d remove operations\n",
//      next_insert, next_remove);

}

int compare_orders_by_id(const void* left, const void* right) {
  return (*(Order**) left)->operation->id() > (*(Order**) right)->operation->id();
}

int compare_operations_by_id(const void* left, const void* right) {
  return (*(Node**) left)->operation->id() > (*(Node**) right)->operation->id();
}

int compare_operations_by_selectable_order(const void* left, const void* right) {
  return (*(Node**) left)->selectable_order > (*(Node**) right)->selectable_order;
}

void destroy_orders (Operations* operations) {

  for (int i = 0; i < operations->num_remove_ops; i++) {
    delete (operations->remove_ops_order[i]);
    operations->remove_ops_order[i] = NULL;
  }
  for (int i = 0; i < operations->num_insert_ops_order; i++) {
    delete (operations->insert_ops_order[i]);
    operations->insert_ops_order[i] = NULL;
  }

  delete (operations->remove_ops_order);
  delete (operations->insert_ops_order);
}

/// Precondition: operations->insert_ops and operations->remove_ops exists
// already and insert and remove operations are already matched.
void create_orders (Operations* operations, Order** order, int num_ops) {

  operations->insert_ops_order = new Node*[operations->num_insert_ops + operations->num_remove_ops];
  operations->remove_ops_order = new Node*[operations->num_remove_ops];

  int insert_position = 0;

  for (int i = 0; i < operations->num_remove_ops; i++) {

    Node* remove_node = new Node();
    operations->remove_ops_order[i] = remove_node;
    remove_node->operation = operations->remove_ops[i]->operation;

    if (remove_node->operation->is_null_return()) {

      // Add an additional node in the insert_ops_order, because null-returns
      // have to be considered in the order of insert operations.
    
      Node* null_return_node = new Node();
      operations->insert_ops_order[insert_position] = null_return_node;
      insert_position++;
      null_return_node->operation =remove_node->operation;
      null_return_node->matching_op = remove_node; 
      remove_node->matching_op = null_return_node;

    } else {

      Node* matching_op = operations->remove_ops[i]->matching_op;
      Node* insert_node = new Node();
      operations->insert_ops_order[insert_position] = insert_node;
      insert_position++;
      insert_node->operation = matching_op->operation;
      remove_node->matching_op = insert_node;
      insert_node->matching_op = remove_node;
    }
  }
  operations->num_insert_ops_order = insert_position;

  qsort(order, num_ops, sizeof(Order*), compare_orders_by_id);
  qsort(operations->insert_ops_order, operations->num_insert_ops_order, sizeof(Node*),
      compare_operations_by_id);
  qsort(operations->remove_ops_order, operations->num_remove_ops, sizeof(Node*),
      compare_operations_by_id);

  int insert_index = 0;
  int remove_index = 0;
  for (int i = 0; i < num_ops; i++) {
 
    if (order[i]->operation->type() == Operation::INSERT) {

      assert (order[i]->operation == operations->insert_ops_order[insert_index]->operation);

      operations->insert_ops_order[insert_index]->selectable_order = order[i]->order;
      insert_index++;

    } else if (order[i]->operation->is_null_return()){
 

      assert (order[i]->operation == operations->insert_ops_order[insert_index]->operation);
      assert (order[i]->operation == operations->remove_ops_order[remove_index]->operation);
 
      operations->insert_ops_order[insert_index]->selectable_order = order[i]->order;
      insert_index++;

      operations->remove_ops_order[remove_index]->selectable_order = order[i]->order;
      remove_index++;

    } else {

      assert (order[i]->operation == operations->remove_ops_order[remove_index]->operation);

      operations->remove_ops_order[remove_index]->selectable_order = order[i]->order;
      remove_index++;
    }
  }
  qsort(operations->insert_ops_order, operations->num_insert_ops_order, sizeof(Node*),
      compare_operations_by_selectable_order);
  qsort(operations->remove_ops_order, operations->num_remove_ops, sizeof(Node*),
      compare_operations_by_selectable_order);
}

void create_doubly_linked_lists(Node* head, Node** ops, int num_ops) {


  for (int i = 1; i < num_ops - 1; i++) {
    ops[i]->next = ops[i + 1];
    ops[i]->prev = ops[i - 1];
  }

  ops[0]->next = ops[1];
  ops[0]->prev = head;

  ops[num_ops - 1]->next = head;
  ops[num_ops - 1]->prev = ops[num_ops - 2];

  head->next = ops[0];
  head->prev = ops[num_ops - 1];

//  printf("list created\n");
}

void next_order(Order** order, Operation* operation, int* next_index) {

  Order* new_order = new Order();
  new_order->operation = operation;
  new_order->order = (uint64_t)(*next_index);
  order[*next_index] = new_order;
  (*next_index)++;
}

Order** merge_linearizations(Operations* operations) {

  Order** result = new Order*[operations->num_insert_ops + operations->num_remove_ops];

  qsort(operations->insert_ops, operations->num_insert_ops, sizeof(Node*), compare_operations_by_order);
  qsort(operations->remove_ops, operations->num_remove_ops, sizeof(Node*), compare_operations_by_order);

  int next_index = 0;

  int j = 0;
  int i = 0;
  while (i < operations->num_insert_ops || j < operations->num_remove_ops) {

    if (i >= operations->num_insert_ops) {
      Node* remove_op = operations->remove_ops[j];
      next_order(result, remove_op->operation, &next_index);
      j++;
    } else if (j >= operations->num_remove_ops) {
      Node* insert_op = operations->insert_ops[i];
      next_order(result, insert_op->operation, &next_index);
      insert_op->matching_op->insert_added = true;
      i++;
    } else {
      Node* remove_op = operations->remove_ops[j];
      Node* insert_op = operations->insert_ops[i];
      if (remove_op->insert_added && 
          remove_op->operation->start() <= insert_op->latest_lin_point) {
        next_order(result, remove_op->operation, &next_index);
        j++;
      } else {
        next_order(result, insert_op->operation, &next_index);
        insert_op->matching_op->insert_added = true;
        i++;
      }
    }
  }

  return result;
}

}

int compare_orders_by_order(const void* left, const void* right) {
  return (*(Order**) left)->order > (*(Order**) right)->order;
}

bool fixed_point_reached(Order** left, Order** right, int num_ops) {

  qsort(left, num_ops, sizeof(Order*), compare_orders_by_order);
  qsort(right, num_ops, sizeof(Order*), compare_orders_by_order);

  for (int i = num_ops - 1; i >= 0; i--) {

    if (left[i]->operation != right[i]->operation) {
      
//      fprintf(stderr, "Difference at position %d, %"PRIu64" != %"PRIu64"\n", i,
//          left[i]->operation->id(), right[i]->operation->id());
      return false;
    }
  }
  return true;
}

void print_op(Operation* op);

Order** linearize_by_min_sum(Operation** ops, int num_ops, Order** order) {

//  static uint64_t bla = 1;
//  uint64_t round = bla++;

  using namespace sum;
  Operations* operations = new Operations();

  initialize(operations, ops, num_ops, order);

  linearize_remove_ops(operations);
  linearize_insert_ops(operations);

//  printf("linearizations finished\n");
  
  Order** new_order = merge_linearizations(operations);

  destroy_orders(operations);
  destroy_insert_and_remove_array(operations);
  delete (operations);

//  printf("round %"PRIu64":\n", round);

  bool done = fixed_point_reached(order, new_order, num_ops);


//  for (int i = 0; i < num_ops; i++) {
//    print_op(order[i]->operation);
//  }

  if ( done
//      fixed_point_reached(order, new_order, num_ops) 
//      || round == 6 
      ) {
//    printf("round: %"PRIu64"\n", round);
    return new_order;
  } else {
    return linearize_by_min_sum(ops, num_ops, new_order);
  }
}
