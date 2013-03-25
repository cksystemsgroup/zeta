// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "operation.h"
#include "element.h"
#include "linearizer.h"

Operation** parse_logfile(const char* filename, int num_ops);

Element** parse_linearization(FILE* input, int num_ops);

Element** convert_order_to_elements(Order** ops, int num_ops);

#endif // PARSER_H
