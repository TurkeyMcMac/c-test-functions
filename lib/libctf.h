/* MIT License
 *
 * Copyright (c) 2019 Jude Melton-Houghton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LIBCTF_H_
#define LIBCTF_H_

#include <libc89tf.h>

/* This is now a wrapper around libc89tf.h with minor convenience added. */

/* CTF_TEST(name, ...) makes a test function with the given name and a body
 * formed of the remaining arguments. This will only be present with the symbol
 * CTF_TESTS_ENABLED defined. The body fails if it crashes or returns nonzero.
 * If it has no return statement and does not crash, it will still succeed. Make
 * sure that you are exporting symbols by default in the compiled executable, as
 * the test symbols must be present for the test runner to pick up on the tests.
 */
#if CTF_TESTS_ENABLED
#	define CTF_TEST(name, ...) CTF_TEST_FUN(name) { __VA_ARGS__; return 0; }
#else
#	define CTF_TEST(name, ...)
#endif

#endif /* LIBCTF_H_ */
