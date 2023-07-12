
#pragma once

#include "zArray.h"
#include "zTypes.h"

class zSplineBase {
private:
    zArray<ptf> _points;
    bool _elimColinearPoints;
protected:
    virtual void resetDerived() = 0;
public:
    zSplineBase() { _elimColinearPoints = true; }
    const zArray<ptf>& getPoints() { return _points; }
    bool getElimColinearPoints() { return _elimColinearPoints; }
    void setElimColinearPoints(bool elim) { _elimColinearPoints = elim; }
    virtual ptf eval(int seg, float t) = 0;
    virtual bool computeSpline() = 0;
    void reset() { _points.clear(); resetDerived(); }
    void addPoint(ptf pt);
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

zArray<ptf> smoothPath(const zArray<ptf>& path, int divisorts, bool bezier);