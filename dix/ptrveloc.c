/*
 *
 * Copyright © 2006-2008 Simon Thum             simon dot thum at gmx dot de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <math.h>
#include <ptrveloc.h>
#include <inputstr.h>
#include <assert.h>

/*****************************************************************************
 * Predictable pointer ballistics
 *
 * 2006-2008 by Simon Thum (simon [dot] thum [at] gmx de)
 *
 * Serves 3 complementary functions:
 * 1) provide a sophisticated ballistic velocity estimate to improve
 *    the relation between velocity (of the device) and acceleration
 * 2) make arbitrary acceleration profiles possible
 * 3) decelerate by two means (constant and adaptive) if enabled
 *
 * Important concepts are the
 *
 * - Scheme
 *      which selects the basic algorithm
 *      (see devices.c/InitPointerAccelerationScheme)
 * - Profile
 *      which returns an acceleration
 *      for a given velocity
 *
 *  The profile can be selected by the user (potentially at runtime).
 *  the classic profile is intended to cleanly perform old-style
 *  function selection (threshold =/!= 0)
 *
 ****************************************************************************/

/* fwds */
static inline void
FeedFilterStage(FilterStagePtr s, float value, int tdiff);
extern void
InitFilterStage(FilterStagePtr s, float rdecay, int lutsize);
void
CleanupFilterChain(DeviceVelocityPtr s);
int
SetAccelerationProfile(DeviceVelocityPtr s, int profile_num);
void
InitFilterChain(DeviceVelocityPtr s, float rdecay, float degression,
                int stages, int lutsize);
void
CleanupFilterChain(DeviceVelocityPtr s);
static float
SimpleSmoothProfile(DeviceVelocityPtr pVel, float threshold, float acc);


/********************************
 *  Init/Uninit etc
 *******************************/

/**
 * Init struct so it should match the average case
 */
void
InitVelocityData(DeviceVelocityPtr s)
{
    s->lrm_time = 0;
    s->velocity  = 0;
    s->corr_mul = 10.0;      /* dots per 10 milisecond should be usable */
    s->const_acceleration = 1.0;   /* no acceleration/deceleration  */
    s->reset_time = 300;
    s->last_reset = FALSE;
    s->last_dx = 0;
    s->last_dy = 0;
    s->use_softening = 1;
    s->min_acceleration = 1.0; /* don't decelerate */
    s->coupling = 0.25;
    s->profile_private = NULL;
    memset(&s->statistics, 0, sizeof(s->statistics));
    memset(&s->filters, 0, sizeof(s->filters));
    SetAccelerationProfile(s, AccelProfileClassic);
    InitFilterChain(s, (float)1.0/20.0, 1, 1, 40);
}


/**
 * Clean up
 */
static void
FreeVelocityData(DeviceVelocityPtr s){
    CleanupFilterChain(s);
    SetAccelerationProfile(s, -1);
}


/*
 *  dix uninit helper, called through scheme
 */
void
AccelerationDefaultCleanup(DeviceIntPtr pDev){
    /*sanity check*/
    if( pDev->valuator->accelScheme.AccelSchemeProc == acceleratePointerPredictable
            && pDev->valuator->accelScheme.accelData != NULL){
        pDev->valuator->accelScheme.AccelSchemeProc = NULL;
        FreeVelocityData(pDev->valuator->accelScheme.accelData);
        xfree(pDev->valuator->accelScheme.accelData);
        pDev->valuator->accelScheme.accelData = NULL;
    }
}

/*********************
 * Filtering logic
 ********************/

/**
Initialize a filter chain.
Expected result is a series of filters, each progressively more integrating.

This allows for two strategies: Either you have one filter which is reasonable
and is being coupled to account for fast-changing input, or you have 'one for
every situation'. You might want to have loose coupling then, i.e. > 1.
E.g. you could start around 1/2 of your anticipated delta t and
scale up until several motion deltas are 'averaged'.
*/
void
InitFilterChain(DeviceVelocityPtr s, float rdecay, float progression, int stages, int lutsize)
{
    int fn;
    if((stages > 1 && progression < 1.0f) || 0 == progression){
	ErrorF("(dix ptracc) invalid filter chain progression specified\n");
	return;
    }
    for(fn = 0; fn < MAX_VELOCITY_FILTERS; fn++){
	if(fn < stages){
	    InitFilterStage(&s->filters[fn], rdecay, lutsize);
	}else{
	    InitFilterStage(&s->filters[fn], 0, 0);
	}
	rdecay /= progression;
    }
}


void
CleanupFilterChain(DeviceVelocityPtr s)
{
    int fn;

    for(fn = 0; fn < MAX_VELOCITY_FILTERS; fn++)
	InitFilterStage(&s->filters[fn], 0, 0);
}

static inline void
StuffFilterChain(DeviceVelocityPtr s, float value)
{
    int fn;

    for(fn = 0; fn < MAX_VELOCITY_FILTERS; fn++){
	if(s->filters[fn].rdecay != 0)
	    s->filters[fn].current = value;
	else break;
    }
}


/**
 * Adjust weighting decay and lut for a stage
 * The weight fn is designed so its integral 0->inf is unity, so we end
 * up with a stable (basically IIR) filter. It always draws
 * towards its more current input values, which have more weight the older
 * the last input value is.
 */
void
InitFilterStage(FilterStagePtr s, float rdecay, int lutsize)
{
    int x;
    float *newlut;
    float *oldlut;

    s->fading_lut_size  = 0; /* prevent access */
    /* mb(); concurrency issues may arise */

    if(lutsize > 0){
        newlut = xalloc (sizeof(float)* lutsize);
        if(!newlut)
            return;
        for(x = 0; x < lutsize; x++)
            newlut[x] = pow(0.5, ((float)x) * rdecay);
    }else{
        newlut = NULL;
    }
    oldlut = s->fading_lut;
    s->fading_lut = newlut;
    s->rdecay = rdecay;
    s->fading_lut_size = lutsize;
    s->current = 0;
    if(oldlut != NULL)
        xfree(oldlut);
}


static inline void
FeedFilterChain(DeviceVelocityPtr s, float value, int tdiff)
{
    int fn;

    for(fn = 0; fn < MAX_VELOCITY_FILTERS; fn++){
	if(s->filters[fn].rdecay != 0)
	    FeedFilterStage(&s->filters[fn], value, tdiff);
	else break;
    }
}


static inline void
FeedFilterStage(FilterStagePtr s, float value, int tdiff){
    float fade;
    if(tdiff < s->fading_lut_size)
        fade = s->fading_lut[tdiff];
    else
        fade = pow(0.5, ((float)tdiff) * s->rdecay);
    s->current *= fade;    /* fade out old velocity */
    s->current += value * (1.0f - fade);    /* and add up current */
}

/**
 * Select the most filtered matching result. Also, the first
 * mismatching filter may be set to value (coupling).
 */
static inline float
QueryFilterChain(
    DeviceVelocityPtr s,
    float value)
{
    int fn, rfn = 0, cfn = -1;
    float cur, result = value;

    /* try to retrieve most integrated result 'within range'
     * Assumption: filter are in order least to most integrating */
    for(fn = 0; fn < MAX_VELOCITY_FILTERS; fn++){
	if(0.0f == s->filters[fn].rdecay)
	    break;
	cur = s->filters[fn].current;

	if (fabs(value - cur) <= 1.0f ||
	    fabs(value - cur) / (value + cur) <= s->coupling){
	    result = cur;
	    rfn = fn + 1; /*remember result determining filter */
	} else if(cfn == -1){
	    cfn = fn; /* rememeber first mismatching filter */
	}
    }

    s->statistics.filter_usecount[rfn]++;
#ifdef PTRACCEL_DEBUGGING
    ErrorF("(dix ptraccel) result from stage %i,  input %.2f, output %.2f\n",
           rfn, value, result);
#endif

    /* override first mismatching current (coupling) so the filter
     * catches up quickly. */
    if(cfn != -1)
        s->filters[cfn].current = result;

    return result;
}

/********************************
 *  velocity computation
 *******************************/

/**
 * return the axis if mickey is insignificant and axis-aligned,
 * -1 otherwise
 * 1 for x-axis
 * 2 for y-axis
 */
static inline short
GetAxis(int dx, int dy){
    if(dx == 0 || dy == 0){
        if(dx == 1 || dx == -1)
            return 1;
        if(dy == 1 || dy == -1)
            return 2;
        return -1;
    }else{
        return -1;
    }
}


/**
 * Perform velocity approximation
 * return true if non-visible state reset is suggested
 */
static short
ProcessVelocityData(
    DeviceVelocityPtr s,
    int dx,
    int dy,
    int time)
{
    float cvelocity;

    int diff = time - s->lrm_time;
    int cur_ax = GetAxis(dx, dy);
    int last_ax = GetAxis(s->last_dx, s->last_dy);
    short reset = (diff >= s->reset_time);

    if(cur_ax != last_ax && cur_ax != -1 && last_ax != -1 && !reset){
        /* correct for the error induced when diagonal movements are
           reported as alternating axis mickeys */
        dx += s->last_dx;
        dy += s->last_dy;
        diff += s->last_diff;
        s->last_diff = time - s->lrm_time; /* prevent repeating add-up */
#ifdef PTRACCEL_DEBUGGING
        ErrorF("(dix ptracc) axial correction\n");
#endif
    }else{
        s->last_diff = diff;
    }

    /*
     * cvelocity is not a real velocity yet, more a motion delta. constant
     * acceleration is multiplied here to make the velocity an on-screen
     * velocity (pix/t as opposed to [insert unit]/t). This is intended to
     * make multiple devices with widely varying ConstantDecelerations respond
     * similar to acceleration controls.
     */
    cvelocity = (float)sqrt(dx*dx + dy*dy) * s->const_acceleration;

    s->lrm_time = time;

    if (s->reset_time < 0 || diff < 0) { /* reset disabled or timer overrun? */
        /* simply set velocity from current movement, no reset. */
        s->velocity = cvelocity;
        return FALSE;
    }

    if (diff == 0)
        diff = 1; /* prevent div-by-zero, though it shouldn't happen anyway*/

    /* translate velocity to dots/ms (somewhat untractable in integers,
       so we multiply by some per-device adjustable factor) */
    cvelocity = cvelocity * s->corr_mul / (float)diff;

    /* short-circuit: when nv-reset the rest can be skipped */
    if(reset == TRUE){
	StuffFilterChain(s, cvelocity);
	s->velocity = cvelocity;
	s->last_reset = TRUE;
	return TRUE;
    }

    if(s->last_reset == TRUE){
	/*
	 * when here, we're probably processing the second mickey of a starting
	 * stroke. This happens to be the first time we can reasonably pretend
	 * that cvelocity is an actual velocity. Thus, to opt precision, we
	 * stuff that into the filter chain.
	 */
	s->last_reset = FALSE;
	StuffFilterChain(s, cvelocity);
	s->velocity = cvelocity;
	return FALSE;
    }

    /* feed into filter chain */
    FeedFilterChain(s, cvelocity, diff);

    /* perform coupling and decide final value */
    s->velocity = QueryFilterChain(s, cvelocity);

#ifdef PTRACCEL_DEBUGGING
    ErrorF("(dix ptracc) guess: vel=%.3f diff=%d   |%i|%i|%i|%i|\n",
           s->velocity, diff,
           s->statistics.filter_usecount[0], s->statistics.filter_usecount[1],
           s->statistics.filter_usecount[2], s->statistics.filter_usecount[3]);
#endif
    return FALSE;
}


/**
 * this flattens significant ( > 1) mickeys a little bit for more steady
 * constant-velocity response
 */
static inline float
ApplySimpleSoftening(int od, int d)
{
    float res = d;
    if (d <= 1 && d >= -1)
        return res;
    if (d > od)
        res -= 0.5;
    else if (d < od)
        res += 0.5;
    return res;
}


static void
ApplySofteningAndConstantDeceleration(
        DeviceVelocityPtr s,
        int dx,
        int dy,
        float* fdx,
        float* fdy,
        short do_soften)
{
    if (do_soften && s->use_softening) {
        *fdx = ApplySimpleSoftening(s->last_dx, dx);
        *fdy = ApplySimpleSoftening(s->last_dy, dy);
    } else {
        *fdx = dx;
        *fdy = dy;
    }

    *fdx *= s->const_acceleration;
    *fdy *= s->const_acceleration;
}



/*****************************************
 *  Acceleration functions and profiles
 ****************************************/

/**
 * Polynomial function similar previous one, but with f(1) = 1
 */
static float
PolynomialAccelerationProfile(DeviceVelocityPtr pVel, float ignored, float acc)
{
   return pow(pVel->velocity, (acc - 1.0) * 0.5);
}


/**
 * returns acceleration for velocity.
 * This profile selects the two functions like the old scheme did
 */
static float
ClassicProfile(
    DeviceVelocityPtr pVel,
    float threshold,
    float acc)
{

    if (threshold) {
	return SimpleSmoothProfile (pVel,
                                    threshold,
                                    acc);
    } else {
	return PolynomialAccelerationProfile (pVel,
                                              0,
                                              acc);
    }
}


/**
 * Power profile
 * This has a completely smooth transition curve, i.e. no jumps in the
 * derivatives.
 *
 * This has the expense of overall response dependency on min-acceleration.
 * In effect, min_acceleration mimics const_acceleration in this profile.
 */
static float
PowerProfile(
    DeviceVelocityPtr pVel,
    float threshold,
    float acc)
{
    float vel_dist;

    acc = (acc-1.0) * 0.1f + 1.0; /* without this, acc of 2 is unuseable */

    if (pVel->velocity <= threshold)
        return pVel->min_acceleration;
    vel_dist = pVel->velocity - threshold;
    return (pow(acc, vel_dist)) * pVel->min_acceleration;
}


/**
 * just a smooth function in [0..1] -> [0..1]
 *  - point symmetry at 0.5
 *  - f'(0) = f'(1) = 0
 *  - starts faster than sinoids, C1 (Cinf if you dare to ignore endpoints)
 */
static inline float
CalcPenumbralGradient(float x){
    x *= 2.0f;
    x -= 1.0f;
    return 0.5f + (x * sqrt(1.0f - x*x) + asin(x))/M_PI;
}


/**
 * acceleration function similar to classic accelerated/unaccelerated,
 * but with smooth transition in between (and towards zero for adaptive dec.).
 */
static float
SimpleSmoothProfile(
    DeviceVelocityPtr pVel,
    float threshold,
    float acc)
{
    float velocity = pVel->velocity;
    if(velocity < 1.0f)
        return CalcPenumbralGradient(0.5 + velocity*0.5) * 2.0f - 1.0f;
    if(threshold < 1.0f)
        threshold = 1.0f;
    if (velocity <= threshold)
        return 1;
    velocity /= threshold;
    if (velocity >= acc)
        return acc;
    else
        return 1.0f + (CalcPenumbralGradient(velocity/acc) * (acc - 1.0f));
}


/**
 * This profile uses the first half of the penumbral gradient as a start
 * and then scales linearly.
 */
static float
SmoothLinearProfile(
    DeviceVelocityPtr pVel,
    float threshold,
    float acc)
{
    if(acc > 1.0f)
        acc -= 1.0f; /*this is so acc = 1 is no acceleration */
    else
        return 1.0f;

    float nv = (pVel->velocity - threshold) * acc * 0.5f;
    float res;
    if(nv < 0){
        res = 0;
    }else if(nv < 2){
        res = CalcPenumbralGradient(nv*0.25f)*2.0f;
    }else{
        nv -= 2.0f;
        res = nv * 2.0f / M_PI  /* steepness of gradient at 0.5 */
              + 1.0f; /* gradient crosses 2|1 */
    }
    res += pVel->min_acceleration;
    return res;
}


static float
LinearProfile(
    DeviceVelocityPtr pVel,
    float threshold,
    float acc)
{
    return acc * pVel->velocity;
}


/**
 * Set the profile by number.
 * Intended to make profiles exchangeable at runtime.
 * If you created a profile, give it a number here and in the header to
 * make it selectable. In case some profile-specific init is needed, here
 * would be a good place, since FreeVelocityData() also calls this with -1.
 * returns FALSE (0) if profile number is unavailable.
 */
int
SetAccelerationProfile(
    DeviceVelocityPtr s,
    int profile_num)
{
    PointerAccelerationProfileFunc profile;
    switch(profile_num){
        case -1:
            profile = NULL;  /* Special case to uninit properly */
            break;
        case AccelProfileClassic:
            profile = ClassicProfile;
            break;
        case AccelProfileDeviceSpecific:
            if(NULL == s->deviceSpecificProfile)
        	return FALSE;
            profile = s->deviceSpecificProfile;
            break;
        case AccelProfilePolynomial:
            profile = PolynomialAccelerationProfile;
            break;
        case AccelProfileSmoothLinear:
            profile = SmoothLinearProfile;
            break;
        case AccelProfileSimple:
            profile = SimpleSmoothProfile;
            break;
        case AccelProfilePower:
            profile = PowerProfile;
            break;
        case AccelProfileLinear:
            profile = LinearProfile;
            break;
        case AccelProfileReserved:
            /* reserved for future use, e.g. a user-defined profile */
        default:
            return FALSE;
    }
    if(s->profile_private != NULL){
        /* Here one could free old profile-private data */
        xfree(s->profile_private);
        s->profile_private = NULL;
    }
    /* Here one could init profile-private data */
    s->Profile = profile;
    s->statistics.profile_number = profile_num;
    return TRUE;
}

/**
 * device-specific profile
 *
 * The device-specific profile is intended as a hook for a driver
 * which may want to provide an own acceleration profile.
 * It should not rely on profile-private data, instead
 * it should do init/uninit in the driver (ie. with DEVICE_INIT and friends).
 * Users may override or choose it.
 */
extern void
SetDeviceSpecificAccelerationProfile(
        DeviceVelocityPtr s,
        PointerAccelerationProfileFunc profile)
{
    if(s)
	s->deviceSpecificProfile = profile;
}

/**
 * Use this function to obtain a DeviceVelocityPtr for a device. Will return NULL if
 * the predictable acceleration scheme is not in effect.
 */
DeviceVelocityPtr
GetDevicePredictableAccelData(
	DeviceIntPtr pDev)
{
    /*sanity check*/
    if( pDev->valuator &&
	pDev->valuator->accelScheme.AccelSchemeProc ==
	    acceleratePointerPredictable &&
	pDev->valuator->accelScheme.accelData != NULL){

	return (DeviceVelocityPtr)pDev->valuator->accelScheme.accelData;
    }
    return NULL;
}

/********************************
 *  acceleration schemes
 *******************************/

/**
 * Modifies valuators in-place.
 * This version employs a velocity approximation algorithm to
 * enable fine-grained predictable acceleration profiles.
 */
void
acceleratePointerPredictable(DeviceIntPtr pDev, int first_valuator,
                             int num_valuators, int *valuators, int evtime)
{
    float mult = 0.0;
    int dx = 0, dy = 0;
    int *px = NULL, *py = NULL;
    DeviceVelocityPtr velocitydata =
	(DeviceVelocityPtr) pDev->valuator->accelScheme.accelData;
    float fdx, fdy; /* no need to init */

    if (!num_valuators || !valuators || !velocitydata)
        return;

    if (first_valuator == 0) {
        dx = valuators[0];
        px = &valuators[0];
    }
    if (first_valuator <= 1 && num_valuators >= (2 - first_valuator)) {
        dy = valuators[1 - first_valuator];
        py = &valuators[1 - first_valuator];
    }

    if (dx || dy){
        /* reset nonvisible state? */
        if (ProcessVelocityData(velocitydata, dx , dy, evtime)) {
            /* set to center of pixel */
            pDev->last.remainder[0] = pDev->last.remainder[1] = 0.5f;
            /* prevent softening (somewhat quirky solution,
            as it depends on the algorithm) */
            velocitydata->last_dx = dx;
            velocitydata->last_dy = dy;
        }

        if (pDev->ptrfeed && pDev->ptrfeed->ctrl.num) {
            /* invoke acceleration profile to determine acceleration */
            mult = velocitydata->Profile(velocitydata,
                                pDev->ptrfeed->ctrl.threshold,
                                (float)pDev->ptrfeed->ctrl.num /
                                (float)pDev->ptrfeed->ctrl.den);

#ifdef PTRACCEL_DEBUGGING
            ErrorF("(dix ptracc) resulting speed multiplier : %.3f\n", mult);
#endif
            /* enforce min_acceleration */
            if (mult < velocitydata->min_acceleration) {
#ifdef PTRACCEL_DEBUGGING
                ErrorF("(dix ptracc) enforced min multiplier : %.3f\n",
                        velocitydata->min_acceleration);
#endif
                mult = velocitydata->min_acceleration;
	    }

            if(mult != 1.0 || velocitydata->const_acceleration != 1.0) {
                ApplySofteningAndConstantDeceleration( velocitydata,
                                                       dx, dy,
                                                       &fdx, &fdy,
                                                       mult > 1.0);
                if (dx) {
                    pDev->last.remainder[0] = mult * fdx + pDev->last.remainder[0];
                    *px = (int)pDev->last.remainder[0];
                    pDev->last.remainder[0] = pDev->last.remainder[0] - (float)*px;
                }
                if (dy) {
                    pDev->last.remainder[1] = mult * fdy + pDev->last.remainder[1];
                    *py = (int)pDev->last.remainder[1];
                    pDev->last.remainder[1] = pDev->last.remainder[1] - (float)*py;
                }
            }
        }
    }
    /* remember last motion delta (for softening/slow movement treatment) */
    velocitydata->last_dx = dx;
    velocitydata->last_dy = dy;
}



/**
 * Originally a part of xf86PostMotionEvent; modifies valuators
 * in-place. Retained mostly for embedded scenarios.
 */
void
acceleratePointerLightweight(
    DeviceIntPtr pDev,
    int first_valuator,
    int num_valuators,
    int *valuators,
    int ignored)
{
    float mult = 0.0;
    int dx = 0, dy = 0;
    int *px = NULL, *py = NULL;

    if (!num_valuators || !valuators)
        return;

    if (first_valuator == 0) {
        dx = valuators[0];
        px = &valuators[0];
    }
    if (first_valuator <= 1 && num_valuators >= (2 - first_valuator)) {
        dy = valuators[1 - first_valuator];
        py = &valuators[1 - first_valuator];
    }

    if (!dx && !dy)
        return;

    if (pDev->ptrfeed && pDev->ptrfeed->ctrl.num) {
        /* modeled from xf86Events.c */
        if (pDev->ptrfeed->ctrl.threshold) {
            if ((abs(dx) + abs(dy)) >= pDev->ptrfeed->ctrl.threshold) {
                pDev->last.remainder[0] = ((float)dx *
                                             (float)(pDev->ptrfeed->ctrl.num)) /
                                             (float)(pDev->ptrfeed->ctrl.den) +
                                            pDev->last.remainder[0];
                if (px) {
                    *px = (int)pDev->last.remainder[0];
                    pDev->last.remainder[0] = pDev->last.remainder[0] -
                                                (float)(*px);
                }

                pDev->last.remainder[1] = ((float)dy *
                                             (float)(pDev->ptrfeed->ctrl.num)) /
                                             (float)(pDev->ptrfeed->ctrl.den) +
                                            pDev->last.remainder[1];
                if (py) {
                    *py = (int)pDev->last.remainder[1];
                    pDev->last.remainder[1] = pDev->last.remainder[1] -
                                                (float)(*py);
                }
            }
        }
        else {
	    mult = pow((float)dx * (float)dx + (float)dy * (float)dy,
                       ((float)(pDev->ptrfeed->ctrl.num) /
                        (float)(pDev->ptrfeed->ctrl.den) - 1.0) /
                       2.0) / 2.0;
            if (dx) {
                pDev->last.remainder[0] = mult * (float)dx +
                                            pDev->last.remainder[0];
                *px = (int)pDev->last.remainder[0];
                pDev->last.remainder[0] = pDev->last.remainder[0] -
                                            (float)(*px);
            }
            if (dy) {
                pDev->last.remainder[1] = mult * (float)dy +
                                            pDev->last.remainder[1];
                *py = (int)pDev->last.remainder[1];
                pDev->last.remainder[1] = pDev->last.remainder[1] -
                                            (float)(*py);
            }
        }
    }
}
