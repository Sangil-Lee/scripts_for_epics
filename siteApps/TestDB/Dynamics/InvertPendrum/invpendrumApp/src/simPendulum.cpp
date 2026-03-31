/* simPendulum.c */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <dbDefs.h>
#include <registryFunction.h>
#include <subRecord.h>
#include <aSubRecord.h>
#include <epicsExport.h>

/*
 * [Input Arguments]
 * A: Force (N) - Control Input
 * B: Reset (0/1)
 * C: Cart Mass (M)
 * D: Pole Mass (m)
 * E: Pole Length (l)
 * F: Gravity (g)
 * G: Time Step (dt)
 * H: Prev Pos (x)
 * I: Prev Vel (dx/dt)
 * J: Prev Angle (theta)
 * K: Prev AngVel (dtheta/dt)
 *
 * [Output Arguments]
 * VALA: New Pos (x)
 * VALB: New Vel (dx/dt)
 * VALC: New Angle (theta)
 * VALD: New AngVel (dtheta/dt)
 */

static long simPendulumProcess(aSubRecord *prec) {
    // 1. Inputs reading
    double F     = *(double *)prec->a;
    int    reset = (int)(*(double *)prec->b);
    double M     = *(double *)prec->c; // Cart Mass
    double m     = *(double *)prec->d; // Pole Mass
    double l     = *(double *)prec->e; // Pole Length
    double g     = *(double *)prec->f;
    double dt    = *(double *)prec->g;

    // Previous State
    double x      = *(double *)prec->h;
    double dx     = *(double *)prec->i;
    double theta  = *(double *)prec->j;
    double dtheta = *(double *)prec->k;

    // 2. Reset Logic
    if (reset) {
        *(double *)prec->vala = 0.0;
        *(double *)prec->valb = 0.0;
        *(double *)prec->valc = 0.1; // Start with slight tilt (0.1 rad)
        *(double *)prec->vald = 0.0;
        //printf("DEBUG: Reset=%d, Force=%.2f, Theta=%.4f\n", reset, F, theta); 
        return 0;
    }

    // 3. Physics Engine (Equations of Motion)
    // Helper variables
    double sin_th = sin(theta);
    double cos_th = cos(theta);
    double m_sin2 = m * sin_th * sin_th;
    
    // Denominator for acceleration
    // x_acc equation derived from Lagrangian
    double denom = M + m * (1 - cos_th * cos_th); // = M + m*sin^2(theta)

    // Calculate Accelerations (ddx, ddtheta)
    // Term 1: Force + Centripetal force of pole
    double term1 = F + m * l * dtheta * dtheta * sin_th;
    // Term 2: Gravity effect coupled via angle
    double term2 = m * g * sin_th * cos_th;

    double ddx = (term1 - term2) / denom;
    
    // Angular acceleration
    double ddtheta = ( (M + m) * g * sin_th - cos_th * (F + m * l * dtheta * dtheta * sin_th) ) / (l * denom);

    // 4. Euler Integration (Update State)
    double new_dx = dx + ddx * dt;
    double new_x  = x + new_dx * dt;

    double new_dtheta = dtheta + ddtheta * dt;
    double new_theta  = theta + new_dtheta * dt;

    // 5. Write Outputs
    *(double *)prec->vala = new_x;
    *(double *)prec->valb = new_dx;
    *(double *)prec->valc = new_theta;
    *(double *)prec->vald = new_dtheta;

    return 0;
}

/* Register the function for the IOC */
epicsRegisterFunction(simPendulumProcess);
