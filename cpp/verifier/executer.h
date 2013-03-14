// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef EXECUTER_H
#define EXECUTER_H

class Histogram;
class Operation;

class Executer {
 public:
  virtual void execute(Histogram* histgram) = 0;
};

#endif  // EXECUTER_H
