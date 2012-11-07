/*
 * sort.c
 *
 *  Created on: 05.11.2012
 *      Author: urandon
 */

/* buffer must have the size more or equal than 'length'
 * int cmp(int a, int b) != 0 if 'a' < 'b'; 0 else
 * 'sort' sorts 'a' in undescending order */
void sort(int * a, int length, int cmp(int, int), int * buffer)
{
	const int l_hi = length/2, r_hi = length;
	int left = 0, right = length/2;
	int pos = 0;

	if(length > 1){
		sort(a, right, cmp, buffer);
		sort(a + right, length - right, cmp, buffer);
		while(left < l_hi && right < r_hi){
			if(left >= l_hi){
				buffer[pos++] = a[right++];
			} else
			if(right >= r_hi){
				buffer[pos++] = a[left++];
			} else {
				buffer[pos++] = (cmp(a[left], a[right])) ? a[left++] : a[right++];
			}
		}
	}
}
