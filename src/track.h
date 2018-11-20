/*
 * track.h
 *
 *  Created on: Nov 17, 2018
 *      Author: lxx
 */

#ifndef SRC_TRACK_H_
#define SRC_TRACK_H_

double ideal_power(int state);
double ideal_total_power();
double real_total_power();

void snapshot();
void snapshot_end();


#endif /* SRC_TRACK_H_ */
