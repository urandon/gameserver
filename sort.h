/*
 * sort.h
 *
 *  Created on: 06.11.2012
 *      Author: urandon
 */

#ifndef SORT_H_
#define SORT_H_

/* buffer must have the size more or equal than 'length'
 * int cmp(int a, int b) != 0 if 'a' < 'b'; 0 else
 * 'sort' sorts 'a' in undescending order */
void sort(int * array, int length, int cmp(int, int), int * buffer);


#endif /* SORT_H_ */
