// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

package test;

import latenessbounds.FirstAttemptToDequeueBounds;
import latenessbounds.LatenessBounds;
import calculate.CalculateLateness;
import util.Operation;


public class TestLateness {

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		// Expected results:
//		average lateness: 0
//		number of late ops: 0
//		number of mega late ops (>#threads): 0
//		maximum lateness: 0
		Operation[] noOverlappings = new Operation[] {
				new Operation(1,  1, 10, 2, 1, 1, 0),
				new Operation(3,  3,  7,  4, 2, 1, 1),
				new Operation(5, 5, 15, 6, 3, 1, 2),
				new Operation(7, 7, 17, 8, 4, 1, 3),
			};
		
//		Expected results:
//		average lateness: 3
//		number of late ops: 1
//		number of mega late ops (>#threads): 1
//		maximum lateness: 3
		Operation[] allOverlap = new Operation[] {
				new Operation(1,  1, 10, 5, 1, 1, 3),
				new Operation(2,  2,  7, 6, 2, 1, 0),
				new Operation(3, 3, 15, 7, 3, 1, 1),
				new Operation(4, 4, 17, 8, 4, 1, 2),
			};

//		Expected results:
//		average lateness: 2
//		number of late ops: 3
//		number of mega late ops (>#threads): 2
//		maximum lateness: 3
		Operation[] oneOverAll = new Operation[] {
				new Operation(2,  2,  7,  3, 2, 1, 2),
				new Operation(4, 4, 15, 5, 3, 1, 1),
				new Operation(6, 6, 17, 7, 4, 1, 0),
				new Operation(1,  1, 10, 8, 1, 1, 3),
			};
		
		LatenessBounds bounds = new FirstAttemptToDequeueBounds();
		
		CalculateLateness.calculateLateness(noOverlappings, 2, bounds);
		
		CalculateLateness.calculateLateness(allOverlap, 1, bounds);
		
		CalculateLateness.calculateLateness(oneOverAll, 2, bounds);
	}
}
