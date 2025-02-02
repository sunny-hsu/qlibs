/*!
 * @file qltisys.c
 * @author J. Camilo Gomez C.
 * @note This file is part of the qTools distribution.
 **/

#include "qltisys.h"

static float qLTISys_DiscreteUpdate( qLTISys_t * const sys,
                                     const float u );
static float qLTISys_ContinuosUpdate( qLTISys_t * const sys,
                                      const float u );

/*============================================================================*/
static float qLTISys_DiscreteUpdate( qLTISys_t * const sys,
                                     const float u )
{
    float v = u;
    size_t i;

    /*using direct-form 2*/
    for ( i = 0 ; i < sys->na ; ++i ) {
        v -= sys->a[ i ]*sys->x[ i ];
    }

    return qLTISys_DiscreteFIRUpdate( sys->x, sys->b, sys->n, v );
}
/*============================================================================*/
static float qLTISys_ContinuosUpdate( qLTISys_t * const sys,
                                      const float u )
{
    float y = 0.0f;

    if ( 1u == sys->n ) {
        sys->x[ 0 ] += ( u - ( sys->x[ 0 ]*sys->a[ 0 ] ) )*sys->dt;
        y = ( sys->b[ 0 ] - ( sys->a[ 0 ]*sys->b0 ) )*sys->x[ 0 ];
    }
    else {
        size_t i;
        float dx0 = 0.0f;
        /*compute states of the system by using the controlable canonical form*/
        for (  i = ( sys->n - 1u ) ; i >= 1u ; --i ) {
            dx0 += sys->a[ i ]*sys->x[ i ]; /*compute the first derivative*/
            /*integrate to obtain the remaining states*/
            sys->x[ i ] += sys->x[ i - 1u ]*sys->dt;
            /*compute the first part of the output*/
            y += ( sys->b[ i ] - ( sys->a[ i ]*sys->b0 ) )*sys->x[ i ];
        }
        /*compute remaining part of the output that depends of the first state*/
        dx0 = u - ( dx0 + ( sys->a[ 0 ]*sys->x[ 0 ] ) );
        sys->x[ 0 ] += dx0*sys->dt; /*integrate to get the first state*/
        /*compute the remaining part of the output*/
        y += ( sys->b[ 0 ] - ( sys->a[ 0 ]*sys->b0 ) )*sys->x[ 0 ];
    }

    return y;
}
/*============================================================================*/
float qLTISys_Excite( qLTISys_t * const sys,
                      float u )
{
    float y = 0.0f;

    if ( 1 == qLTISys_IsInitialized( sys ) ) {
        if ( NULL != sys->tDelay.head ) { /*check if has delay*/
            qTDL_InsertSample( &sys->tDelay, u ); /*delay the output*/
            /*excite the system with the most delayed input*/
            u = qTDL_GetOldest( &sys->tDelay );
        }
        y = sys->sysUpdate( sys, u ); /*evaluate the system*/
        /*saturate*/
        if ( y < sys->min ) {
            y = sys->min;
        }
        else if ( y > sys->max ) {
            y = sys->max;
        }
        else {
            /*do nothing*/
        }
    }

    return y;
}
/*============================================================================*/
int qLTISys_SetDelay( qLTISys_t * const sys,
                      float * const w,
                      const size_t n,
                      const float initval )
{
    int retValue = 0;

    if ( 1 == qLTISys_IsInitialized( sys ) ) {
        qTDL_Setup( &sys->tDelay, w, n, initval );
        retValue = 1;
    }

    return retValue;
}
/*============================================================================*/
int qLTISys_SetSaturation( qLTISys_t * const sys,
                           const float min,
                           const float max )
{
    int retValue = 0;

    if ( ( 1 == qLTISys_IsInitialized( sys ) ) && ( max > min ) ) {
        sys->min = min;
        sys->max = max;
        retValue = 1;
    }

    return retValue;
}
/*============================================================================*/
int qLTISys_IsInitialized( const qLTISys_t * const sys )
{
    int retValue = 0;

    if ( NULL != sys ) {
        retValue = (int)( ( NULL != sys->sysUpdate ) && ( NULL != sys->x ) );
    }

    return retValue;
}
/*============================================================================*/
int qLTISys_Setup( qLTISys_t * const sys,
                   float *num,
                   float *den,
                   float *x,
                   const size_t nb,
                   const size_t na,
                   const float dt )
{
    int retValue = 0;

    if ( ( NULL != sys ) && ( NULL != num ) && ( NULL != den ) && ( NULL != x ) && ( na > 0u ) ) {
        float a0;
        size_t i;

        if ( dt < 0.0f ) { /*discrete system*/
            sys->b = num;
            sys->na = na;
            sys->nb = nb;
            sys->n = ( na > nb ) ? na : nb;
            sys->sysUpdate = &qLTISys_DiscreteUpdate;
        }
        else { /*continuos system*/
            sys->b = &num[ 1 ];
            sys->n = na - 1u;
            sys->nb = sys->n;
            sys->sysUpdate = &qLTISys_ContinuosUpdate;
        }
        sys->a = &den[ 1 ];
        sys->x = x;
        sys->dt = dt;
        /*normalize the transfer function coefficients*/
        a0 = den[ 0 ];
        for ( i = 0 ; i < sys->nb ; ++i ) {
            num[ i ] /= a0;
        }
        for ( i = 0; i < sys->na ; ++i ) {
            den[ i ] /= a0;
        }
        sys->b0 = num[ 0 ];
        sys->tDelay.head = NULL;
        retValue = qLTISys_SetSaturation( sys, -FLT_MAX, FLT_MAX );
    }

    return retValue;
}
/*============================================================================*/
float qLTISys_DiscreteFIRUpdate( float *w,
                                 const float * const c,
                                 const size_t wsize,
                                 const float x )
{
    size_t i;
    float y = 0.0f;

    if ( NULL != c ) {
        for ( i = ( wsize - 1u ) ; i >= 1u ; --i ) {
            w[ i ] = w[ i - 1u ];
            y += w[ i ]*c[ i ];
        }
        y += c[ 0 ]*x;
    }
    else {
        for ( i = ( wsize - 1u ) ; i >= 1u ; --i ) {
            w[ i ] = w[ i - 1u ];
            y += w[ i ];
        }
        y += x;
    }
    w[ 0 ] = x;

    return y;
}
/*============================================================================*/