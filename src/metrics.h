/*
 * metrics.h
 *
 *  Created on: Oct 24, 2013
 *      Author: sahsan
 */

#ifndef METRICS_H_
#define METRICS_H_

#include <stdbool.h>

void checkstall(bool end);
void printvalues();
void printinterim (double bytesnow, double dlspeed, int idx);

#endif /* METRICS_H_ */
