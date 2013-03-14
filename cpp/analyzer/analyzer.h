// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef ANALYZER_H
#define ANALYZER_H

#include <inttypes.h>
#include "element.h"

struct Result {
  uint64_t max_costs;
  uint64_t num_ops;
  uint64_t total_costs;
  double avg_costs;
};

Result* calculate_age(Element** elements, int num_ops);

#endif // ANALYZER_H
