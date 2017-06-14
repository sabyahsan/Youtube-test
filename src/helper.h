/*
 *
 *      Year   : 2013-2017
 *      Author : Saba Ahsan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <string.h>
#include <sys/time.h>

char *str_replace(char *orig, char *rep, char *with);
void printhelp(char *name);

static inline void memzero (void * ptr, int size)
{
	memset(ptr, 0, size);
}

static inline long long gettimelong()
{
	struct timeval start;

	gettimeofday(&start, NULL);
	return((long long)start.tv_sec * 1000000 + start.tv_usec);
}

static inline time_t gettimeshort()
{
	struct timeval start;

	gettimeofday(&start, NULL);
	return start.tv_sec;
}

#endif /* HELPER_H_ */
