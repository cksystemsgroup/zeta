// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

package latenessbounds;
import util.Operation;


public interface LatenessBounds {

	long getLowerBoundValue(Operation operation);
	long getUpperBoundValue(Operation operation);
}
