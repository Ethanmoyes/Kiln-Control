#include "pid_control.h"

const int kP = 500;
const int kI = 2;
const int kD = 1000;
const int drive_minimum = 0; // The minimum value that the output can deliver
const int drive_maximum = 8100; // The maximum value that the output can deliver
const int integral_maximum = drive_maximum-1000; //The maximum the integral can grow to
const int integral_minimum = 0; //The mimimum the integral can grow to

pid_gains gains = {
    .kP = 0,
    .kI = 0,
    .kD = 0
};
drive_char drive = {drive_maximum,drive_minimum,0};
pid_param pid_terms = {
    .Imax = integral_maximum,
    .Imin = integral_minimum
};

int calculate_terms(pid_param* pid, pid_gains* gains){
    pid->err = pid->sp - pid->pv;
    
    pid->Px = pid->err * gains->kP; // Proportional Gain * Error = P Term
        
    
    pid->Ipv = (pid->Ipv + pid->err); // Integral Present Value = Integral Present Value + Error - Slow Wind Down
    if ((pid->Ipv*gains->kI) > pid->Imax){ // If Ipv is greater than max, set to max
        pid->Ipv = pid->Imax/gains->kI;
    }
    if ((pid->Ipv*gains->kI) < pid->Imin){ // If Ipv is less than min, set to min
        pid->Ipv = pid->Imin/gains->kI;
    }
    pid->Ix = pid->Ipv * gains->kI; // I term = Integral Present Value * Integral Gain
        
     

    pid->Dx = (pid->Dpv - pid->pv) * gains->kD; // D term = Change in pv * Derivative Gain
    pid->Dpv = pid->pv; // Set current pv to Dpv
    return (int)(pid->Px + pid->Ix + pid->Dx);
};

void init_pid(pid_param* pid){
    pid->Px = pid->Ix = pid->Dx = 0; //Set terms to 0
    pid->Ipv = 0; //Set Integral present values to 0
    pid->Dpv = pid->pv; // Set Derivative present value to pv
    pid->sp = 0;
}

int pid_drive(pid_param* pid, pid_gains* gains, drive_char* drive){
    
    drive->output = calculate_terms(pid,gains); //Set drive to sum of terms
    if (drive->output > drive->max){ // Bound drive output to min and max of drive
        drive->output = drive->max;
    }
    if (drive->output < drive->min){
        drive->output = drive->min;
    }
    return drive->output;
}


