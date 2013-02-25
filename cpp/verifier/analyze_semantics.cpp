#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "operation.h"
#include "fifoExecuterLowerBound.h"
#include "fifoExecuterUpperBound.h"
#include "histogram.h"

void enable_frequency_scaling() {
  int result =
      system(
          "for i in  /sys/devices/system/cpu/cpu? /sys/devices/system/cpu/cpu??; do sudo sh -c \"echo ondemand > $i/cpufreq/scaling_governor\"; done");
  if (result) {
    printf("frequency scaling error");
  }
}

void disable_frequency_scaling() {
  int result =
      system(
          "for i in  /sys/devices/system/cpu/cpu? /sys/devices/system/cpu/cpu??; do sudo sh -c \"echo performance > $i/cpufreq/scaling_governor\"; done");
  if (result) {
    printf("frequency scaling error");
  }
}

void calculate_pr_graph(Operations* operations) {

  int64_t num = 0;
//  int64_t total = 0;
//  int64_t max = 0;
//  int64_t tmp = 0;


  int64_t performance_index = 0;
  do {
    Operation** history = operations->create_history_with_performance_index(performance_index);

    Operations ops;
    ops.Initialize(history, operations->num_all_ops(), false);
    ops.CalculateOverlaps();
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);

    Histogram histogram;
//    executer->calculate_new_element_fairness(Operation::lin_point_start_time, Operation::compare_operations_by_start_time);
    executer->execute(&histogram);
    executer->calculate_order();
    num = executer->aggregate_semantical_error();

//    executer->ef_all_dequeues(operations, &num, &total, &max, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, Operation::lin_point_lin_order, Operation::compare_operations_by_lin_order);

//    double average = total;
//    average /= num;

//    printf("%lu %.3f %lu\n", performance_index, average, max);

    for (int i = 0; i < operations->num_all_ops(); i++) {
      delete history[i];
    }
    delete[] history;

    delete executer;

    performance_index++;
  } while (num > 0);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: ./analyzer <logfilename> <operations> <mode>\n");
    return 1;
  }

  char* order = argv[3];
  int operations = atoi(argv[1]);
  char* filename = argv[2];
  int execution_time_quotient = 1;

  if (argc >= 5) {
    execution_time_quotient = atoi(argv[4]);
  }

//  int bound = atoi(argv[4]); //0 lower bound, 1 upper bound
//  char* mode = argv[5];
//  int param = atoi(argv[6]);

//  disable_frequency_scaling();





//  } else {
//    executer = new FifoExecuterUpperBound(&ops);
//  }

  if (strcmp(order, "tool_op") == 0) {

    // Adjust start-times of remove operations because otherwise the tool calculates lin-points for
    // insert-operations which may be before their invocation.
    Operations ops(filename, operations, true);
    ops.shorten_execution_times(execution_time_quotient);
    ops.CalculateOverlaps();
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    Histogram histogram;
    executer->execute(&histogram);
    executer->calculate_order();
    executer->calculate_op_fairness();
  } else if (strcmp(order, "tool") == 0) {
    Operations ops(filename, operations, true);
    if (execution_time_quotient > 1) {
      ops.shorten_execution_times(execution_time_quotient);
    }
    ops.CalculateOverlaps();
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    Histogram histogram;
    executer->execute(&histogram);
    executer->aggregate_semantical_error();
  } else if (strcmp(order, "op_lin") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_response_order(true);
    executer->calculate_op_fairness_typeless();
  } else if (strcmp(order, "response") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_response_order(false);
    executer->calculate_op_fairness_typeless();
  } else if (strcmp(order, "element") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_element_fairness();
  } else if (strcmp(order, "new") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_new_element_fairness(Operation::lin_point_start_time, Operation::compare_operations_by_start_time);
  } else if (strcmp(order, "ef_response") == 0) {
      Operations ops(filename, operations, true);
      FifoExecuter *executer;
      executer = new FifoExecuterLowerBound(&ops);
      executer->calculate_new_element_fairness(Operation::lin_point_end_time, Operation::compare_operations_by_end_time);
  } else if (strcmp(order, "ef_lin") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_new_element_fairness(Operation::lin_point_lin_time, Operation::compare_operations_by_lin_time);
} else if (strcmp(order, "diff") == 0) {
  Operations ops(filename, operations, false);
  FifoExecuter *executer;
  executer = new FifoExecuterLowerBound(&ops);
  executer->calculate_diff(Operation::lin_point_start_time, Operation::compare_operations_by_start_time, Operation::lin_point_end_time, Operation::compare_operations_by_end_time);
} else if (strcmp(order, "new_sane") == 0) {
    Operations ops(filename, operations, true);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_new_element_fairness(Operation::lin_point_start_time, Operation::compare_operations_by_start_time);
  } else if (strcmp(order, "perf") == 0) {
      Operations ops(filename, operations, false);
      FifoExecuter *executer;
      executer = new FifoExecuterLowerBound(&ops);
      executer->calculate_performance_index();
  } else if (strcmp(order, "prgraph") == 0) {
    Operations ops(filename, operations, true);
    calculate_pr_graph(&ops);
  } else {
    printf("Invalid mode, use tool_op, tool, response, element, new, or new_sane, not %s\n", order);
    exit(-1);
  }

//  if (strcmp(datastructure, "fifo") == 0) {
//    if (strcmp(mode, "histogram") == 0) {
//      histogram.print();
//    } else if (strcmp(mode, "sum") == 0) {
//      printf("%" PRIu64 "\n", histogram.commulativeError());
//    } else if (strcmp(mode, "sumparam") == 0) {
//      printf("%d %" PRIu64 " %" PRIu64 " %f %f\n", param, histogram.commulativeError(), histogram.max(), histogram.mean(), histogram.stdv());
//    } else {
//      printf("wrong mode\n");
//    }
//  } else {
//    printf("Fatal Error: datastructure not supported\n");
//    enable_frequency_scaling();
//    exit(2);
//  }
//  enable_frequency_scaling();
  return 0;
}


