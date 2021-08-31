/*!
 * @file qpid.c   
 * @author J. Camilo Gomez C.
 * @note This file is part of the qTools distribution.
 **/

#include "qpid.h"

/*============================================================================*/
int qPID_Setup( qPID_controller_t *c, float kc, float ki, float kd, float dt )
{
    int retVal = 0;
    if ( ( NULL != c ) && ( dt > 0.0f ) ) {
        c->kc = kc;
        c->ki = ki;
        c->kd = kd;
        c->dt = dt;
        c->min = 0.0f;
        c->max = 100.0f;
        c->kw = 1.0f;
        c->epsilon = FLT_MIN;
        c->init = 1u;
        qPID_Reset( c );
        retVal = 1;
    }
    return retVal;
}
/*============================================================================*/
int qPID_Reset( qPID_controller_t *c )
{
    int retVal = 0;
    if ( ( NULL != c ) && ( 0u == c->init) ) {
        c->e1 = 0.0f;
        c->ie = 0.0f;
        c->init = 0u;
        retVal = 1;
    }
    return retVal;
}
/*============================================================================*/
int qPID_SetSaturation( qPID_controller_t *c, float min, float max, float kw )
{
    int retVal = 0;
    if ( ( NULL != c ) && ( max > min ) && ( 0u == c->init ) && ( kw > 0.0f ) ) {
        c->min = min;
        c->max = max;
        c->kw = kw;
        retVal = 1;
    }
    return retVal;
}
/*============================================================================*/
int qPID_SetParallel( qPID_controller_t *c )
{
    int retVal = 0;
    if ( ( NULL != c ) && ( 0u == c->init) ) {
        float ti, td, tmp;
        ti = c->kc/c->ki;
        td = c->kd/c->kc;
        tmp = 1.0f + ( td/ti );
        c->kc = c->kc*tmp;
        c->ki = c->kc/( ti*tmp );
        c->kd = c->kc*( td/tmp );
        retVal = 1;
    }
    return retVal;
}
/*============================================================================*/
int qPID_SetEpsilon( qPID_controller_t *c, float eps )
{
    int retVal = 0;
    if ( ( NULL != c ) && ( 0u == c->init) && ( eps > 0.0f) ) {
        c->epsilon = eps;
        retVal = 1;
    }
    return retVal;
}
/*============================================================================*/
float qPID_Control( qPID_controller_t *c, float w, float y )
{
    float u = w;
    if ( ( NULL != c ) && ( 0u == c->init) ) {
        float e, v, de;
        e = w - y;
        if ( fabs( e ) <= ( c->epsilon ) ) {
            e = 0.0f;
            c->ie += c->u1*( c->dt );
            de = -c->e1/c->dt;
            v  = c->ki*c->ie + c->kd*de;
        }
        else {
            c->ie += ( e + c->u1 )*( c->dt );
            de = ( e - c->e1 )/c->dt;
            v  = c->kc*e + c->ki*c->ie + c->kd*de;
        }

        u = v;
        if ( u > c->max ) {
            u = c->max;
        }
        if ( u < c->min ) {
            u = c->min;
        }
        c->u1 = c->kw*( u - v );
        c->e1 = e;
    }
    return u;
}
/*============================================================================*/