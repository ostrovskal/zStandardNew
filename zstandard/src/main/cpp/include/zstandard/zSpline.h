
#pragma once

#include "zArray.h"
#include "zTypes.h"

class zSplineBase {
public:
    zSplineBase() {  }
    const zArray<ptf>& getPoints() { return _points; }
    virtual ptf eval(int seg, float t) = 0;
    virtual bool computeSpline() = 0;
    void reset() { _points.clear(); resetDerived(); }
    void addPoint(cptf& pt) { _points += pt;  }
protected:
    virtual void resetDerived() = 0;
private:
    zArray<ptf> _points;
};

class zClassicSpline : public zSplineBase {
private:
    zArray<float> _xCol, _bCol, _diagElems, _offDiagElems;
public:
    zClassicSpline() { }
    virtual ptf eval(int seg, float t) override;
    virtual void resetDerived() override;
    virtual bool computeSpline() override;
};

class zBezierSpline : public zSplineBase {
private:
    zArray<ptf> _p1Points, _p2Points;
public:
    zBezierSpline() { }
    virtual ptf eval(int seg, float t) override;
    virtual void resetDerived() override;
    virtual bool computeSpline() override;
};

void smoothPath(zArray<ptf>& path, int divisorts, bool bezier);