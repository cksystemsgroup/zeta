// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

package latenessbounds;

import calculate.CalculateLateness;
import util.Operation;

public class FirstAttemptToDequeueBounds implements LatenessBounds {

	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateLateness.calculateLateness(operations, 24, new FirstAttemptToDequeueBounds());
	}
	
	@Override
	public long getLowerBoundValue(Operation operation) {
		return operation.getFirstAttempt();
	}

	@Override
	public long getUpperBoundValue(Operation operation) {
		return operation.getDequeueIndex();
	}

}
