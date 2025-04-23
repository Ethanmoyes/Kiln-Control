#ifndef PID_CONTROL_H
#define PID_CONTROL_H

#include "pico/stdlib.h"
#include <stdio.h>

typedef struct { //PID Gains
    float kP, kI, kD;
} pid_gains;

typedef struct { //Drive characteristics
    int max, min, output;
} drive_char; 

typedef struct {
    float Px, Ix, Dx; // PID Terms
    float sp, pv, err; //Setpoint, Present Value, Floats
    float Ipv, Dpv; //Integral Present Value, Derivative Present Value
    float Imax, Imin; // Limit the values of integral term
} pid_param;

int calculate_terms(pid_param* pid, pid_gains* gains);
void init_pid(pid_param* pid);
int pid_drive(pid_param* pid, pid_gains* gains, drive_char* drive);

#endif