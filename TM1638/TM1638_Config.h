/* mbed TM1638 Library, for TM1638 LEDcontroller
 * Copyright (c) 2015, v01: WH, Initial version
 *               2016, v02: WH, Added ASCII alphabet display selector
 *               2016, v03: WH, Added QYF-TM1638 and LKM1638
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TM1638_CONFIG_H
#define TM1638_CONFIG_H

// Select one of the testboards for TM1638 LED controller
#define TM1638_TEST  0
#define LEDKEY8_TEST 1 
#define QYF_TEST     0 
#define LKM1638_TEST 0 
 
// Select the display mode: only digits and hex or ASCII
#define SHOW_ASCII   1 

#endif