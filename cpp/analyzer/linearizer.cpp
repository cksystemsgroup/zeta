// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include "operation.h"
#include "parser.h"
#include "linearizer.h"
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "element.h"
#include "analyzer.h"

void print_op(Operation* op) {

  assert(op != NULL);
  if (op->type() == Operation::INSERT) {
    printf("+ %"PRId64"   %"PRIu64"\n", op->value(), op->real_start());
  } else {
    printf("- %"PRId64"   %"PRIu64"\n", op->value(), op->real_start());
  } 
}

int main(int argc, char** argv) {

  if (argc < 4) {
    fprintf(stderr, "usage: ./linearizer <mode> <operations> <logfilename>\n");
    fprintf(stderr, "The following modes exist: max; invocation; lin_point; response; sum\n");
    exit(3);
  }

  char* mode = argv[1];
  int num_ops = atoi(argv[2]);
  char* filename = argv[3];
  
  const char* analyzer = "non";
  if (argc >= 5) {
    analyzer = argv[4];
  }

  Operation** ops = parse_logfile(filename, num_ops);
  
  Order** linearization;

  if (strcmp(mode, "max") == 0) {
    linearization = linearize_by_min_max(ops, num_ops);
  } else if (strcmp(mode, "invocation") == 0) {
    linearization = linearize_by_invocation(ops, num_ops);
  } else if (strcmp(mode, "lin_point") == 0) {
    linearization = linearize_by_lin_point(ops, num_ops);
  } else if (strcmp(mode, "response") == 0) {
    linearization = linearize_by_response(ops, num_ops);
  } else if (strcmp(mode, "sum") == 0) {
    // First we use the lin_point linearization as the initial linearization.
    linearization = linearize_by_lin_point(ops, num_ops);
    linearization = linearize_by_min_sum(ops, num_ops, linearization);
  } else {
    fprintf(stderr, "The mode %s does not exist, use either 'max', 'invocation', 'lin_point', 'response', or 'sum'\n", mode);
    exit(11);
  }

  if (strcmp(analyzer, "age") == 0) {
    fprintf(stderr, "Hello World\n");
    Element** elements = convert_order_to_elements(linearization, num_ops);
    Result* result = calculate_age(elements, num_ops);
    printf("max: %"PRIu64"; num_ops: %"PRIu64"; total: %"PRIu64"; average: %0.3f\n",
        result->max_costs, result->num_ops, result->total_costs, result->avg_costs);

  } else {
    for (int i = 0; i < num_ops; i++) {
      assert(linearization[i] != NULL);
      print_op(linearization[i]->operation);
    }
  }
  return 0;
}
