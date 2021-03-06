// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

package calculate;
import latenessbounds.StartTimeToDequeueBounds;
import util.Operation;

public class CalculateLatenessWithStartTime {

	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateLateness.calculateLateness(operations, 24, new StartTimeToDequeueBounds());
	}
}
