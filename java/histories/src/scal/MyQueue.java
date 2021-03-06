// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

package scal;

public interface MyQueue<E> {

	boolean enqueue(E item);
	E dequeue();
	int size();
}
