//
// Created by User on 26.04.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zInterpolator.h"

bool zInterpolator::set() {
    InterpolatorParams* pp;
    type = NONE;
    if(index >= params.size()) {
        index = 0;
        if(!loop) return false;
    }
    pp = &params[index];
    frames = pp->frames; frame = 0;
    vStart = (index == 0 ? start : vDest); vDest = pp->dest;
    type = pp->type; index++;
    return true;
}

bool zInterpolator::update(float& p) {
    if(type == NONE) { if(!set()) return false; }
    p = formula(type, (float)frame, vStart, (float)frames, vDest - vStart);
    auto ret(frame++ < frames);
    if(!ret) ret = set();
    return ret;
}

float zInterpolator::formula(INTERPOLATOR_TYPE type, float t, float b, float d, float c) {
    float t1;
    switch (type) {
            // simple linear interpolation - no easing
        case LINEAR: return (c * t / d + b);
            // quadratic (t^2) easing in - с ускорением от нуля
        case EASEINQUAD: t1 = t / d; return (c * t1 * t1 + b);
            // quadratic (t^2) easing out - с торможением до нуля
        case EASEOUTQUAD: t1 = t / d; return (-c * t1 * (t1 - 2) + b);
            // quadratic easing in/out - ускорение до половины, потом торможение
        case EASEINOUTQUAD:
            t1 = t / d * 2;
            if (t1 < 1) return (c / 2 * t1 * t1 + b);
            else { t1 = t1 - 1; return (-c / 2 * (t1 * (t1 - 2) - 1) + b); }
            // cubic easing in - с ускорением от нуля
        case EASEINCUBIC: t1 = t / d; return (c * t1 * t1 * t1 + b);
            // cubic easing out - с торможением до нуля
        case EASEOUTCUBIC: t1 = t / d - 1; return (c * (t1 * t1 * t1 + 1) + b);
            // cubic easing in - ускорение до половины, потом торможение
        case EASEINOUTCUBIC:
            t1 = t / d * 2;
            if (t1 < 1) return (c / 2 * t1 * t1 * t1 + b);
            else { t1 -= 2; return (c / 2 * (t1 * t1 * t1 + 2) + b); }
            // quartic easing in - с ускорением от нуля
        case EASEINQUART: t1 = t / d; return (c * t1 * t1 * t1 * t1 + b);
            // exponential (2^t) easing in - с ускорением от нуля
        case EASEINEXPO: return (t == 0 ? b : (c * powf(2, (10 * (t / d - 1))) + b));
            // exponential (2^t) easing out - с торможением до нуля
        case EASEOUTEXPO: return (t == d ? (b + c) : (c * (-powf(2, -10 * t / d) + 1) + b));
        default: return 0;
    }
}
