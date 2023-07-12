
#include "zstandard/zstandard.h"
#include "zstandard/zSpline.h"

void zSplineBase::addPoint(ptf pt) {
    if(_elimColinearPoints && _points.size() > 2) {
        int N = _points.size() - 1;
        ptf p0(_points[N - 1] - _points[N - 2]);
        ptf p1(_points[N] - _points[N - 1]);
        ptf p2(pt - _points[N]);
        float delta((p2.y - p1.y) * (p1.x - p0.x) - (p1.y - p0.y) * (p2.x - p1.x));
        if(delta < Z_EPSILON) { _points.erase(_points.size() - 1, 1); }
    }
    _points += pt;
}

ptf zClassicSpline::eval(int seg, float t) {
    auto points(getPoints());
    const float ONE_OVER_SIX(1.0f / 6.0f);
    float oneMinust(1.0f - t), t3Minust(t * t * t - t);
    float oneMinust3minust(oneMinust * oneMinust * oneMinust - oneMinust);
    float deltaX(points[seg + 1].x - points[seg].x);
    float yValue(t * points[seg + 1].y + oneMinust * points[seg].y + ONE_OVER_SIX * deltaX * deltaX * (t3Minust * _xCol[seg + 1] - oneMinust3minust * _xCol[seg]));
    float xValue(t * (points[seg + 1].x - points[seg].x) + points[seg].x);
    return { xValue, yValue };
}

void zClassicSpline::resetDerived() {
    _diagElems.clear();
    _bCol.clear();
    _xCol.clear();
    _offDiagElems.clear();
}

bool zClassicSpline::computeSpline() {
    auto p(getPoints());
    _bCol.resize(p.size()); _xCol.resize(p.size());
    _diagElems.resize(p.size()); _offDiagElems.resize(p.size());
    for(int idx = 1; idx < p.size(); ++idx) _diagElems[idx] = 2 * (p[idx + 1].x - p[idx - 1].x);
    for(int idx = 0; idx < p.size(); ++idx) _offDiagElems[idx] = p[idx + 1].x - p[idx].x;
    for(int idx = 1; idx < p.size(); ++idx) {
        _bCol[idx] = 6.0f * ((p[idx + 1].y - p[idx].y) / _offDiagElems[idx] - (p[idx].y - p[idx - 1].y) / _offDiagElems[idx - 1]);
    }
    _xCol[0] = 0.0f; _xCol[p.size() - 1] = 0.0f;
    for(int idx = 1; idx < p.size() - 1; ++idx) {
        _bCol[idx + 1] = _bCol[idx + 1] - _bCol[idx] * _offDiagElems[idx] / _diagElems[idx];
        _diagElems[idx + 1] = _diagElems[idx + 1] - _offDiagElems[idx] * _offDiagElems[idx] / _diagElems[idx];
    }
    for(int idx = (int)p.size() - 2; idx > 0; --idx) _xCol[idx] = (_bCol[idx] - _offDiagElems[idx] * _xCol[idx + 1]) / _diagElems[idx];
    return true;
}

ptf zBezierSpline::eval(int seg, float t) {
    float omt(1.0f - t);
    ptf p0(getPoints()[seg]), p1(_p1Points[seg]), p2(_p2Points[seg]), p3(getPoints()[seg + 1]);
    float xVal(omt * omt * omt * p0.x + 3.0f * omt * omt * t * p1.x + 3.0f * omt * t * t * p2.x + t * t * t * p3.x);
    float yVal(omt * omt * omt * p0.y + 3.0f * omt * omt * t * p1.y + 3.0f * omt * t * t * p2.y + t * t * t * p3.y);
    return { xVal, yVal };
}

void zBezierSpline::resetDerived() {
    _p1Points.clear();
    _p2Points.clear();
}

bool zBezierSpline::computeSpline() {
    auto p(getPoints());
    int N(p.size() - 1);
    _p1Points.resize(N);
    _p2Points.resize(N);
    if(N == 0) return false;
    if(N == 1) {
        _p1Points[0] = (2.0f / 3.0f * p[0] + 1.0f / 3.0f * p[1]);
        _p2Points[0] = 2.0f * _p1Points[0] - p[0];
        return true;
    }
    zArray<ptf> a(N), b(N), c(N), r(N);
    a[0].x = 0; b[0].x = 2; c[0].x = 1;
    r[0].x = p[0].x + 2 * p[1].x;
    a[0].y = 0; b[0].y = 2; c[0].y = 1;
    r[0].y = p[0].y + 2 * p[1].y;
    for(int i = 1; i < N - 1; i++) {
        a[i].x = 1; b[i].x = 4.0f; c[i].x = 1;
        r[i].x = 4 * p[i].x + 2.0f * p[i + 1].x;
        a[i].y = 1; b[i].y = 4.0f; c[i].y = 1;
        r[i].y = 4 * p[i].y + 2.0f * p[i + 1].y;
    }
    a[N - 1].x = 2; b[N - 1].x = 7;
    c[N - 1].x = 0; r[N - 1].x = 8 * p[N - 1].x + p[N].x;
    a[N - 1].y = 2; b[N - 1].y = 7; c[N - 1].y = 0;
    r[N - 1].y = 8 * p[N - 1].y + p[N].y;
    for(int i = 1; i < N; i++) {
        float m;
        m = a[i].x / b[i - 1].x;
        b[i].x = b[i].x - m * c[i - 1].x;
        r[i].x = r[i].x - m * r[i - 1].x;
        m = a[i].y / b[i - 1].y;
        b[i].y = b[i].y - m * c[i - 1].y;
        r[i].y = r[i].y - m * r[i - 1].y;
    }
    _p1Points[N - 1].x = r[N - 1].x / b[N - 1].x;
    _p1Points[N - 1].y = r[N - 1].y / b[N - 1].y;
    for(int i = N - 2; i >= 0; --i) {
        _p1Points[i].x = (r[i].x - c[i].x * _p1Points[i + 1].x) / b[i].x;
        _p1Points[i].y = (r[i].y - c[i].y * _p1Points[i + 1].y) / b[i].y;
    }
    for(int i = 0; i < N - 1; i++) {
        _p2Points[i].x = 2.0f * p[i + 1].x - _p1Points[i + 1].x;
        _p2Points[i].y = 2.0f * p[i + 1].y - _p1Points[i + 1].y;
    }
    _p2Points[N - 1].x = 0.5f * (p[N].x + _p1Points[N - 1].x);
    _p2Points[N - 1].y = 0.5f * (p[N].y + _p1Points[N - 1].y);
    return true;
}

zArray<ptf> smoothPath(const zArray<ptf>& path, int divisions, bool bezier) {
    zArray<ptf> ret;
    if(path.size() > 1) {
        zClassicSpline cs; zBezierSpline bs;
        auto spline(bezier ? (zSplineBase*)&bs : (zSplineBase*)&cs);
        for(auto &pt: path) spline->addPoint(pt);
        spline->computeSpline();
        for(int idx = spline->getPoints().size() - 2; idx >= 0; --idx) {
            for(int division = divisions - 1; division >= 0; --division) {
                float t((float)division * 1.0f / (float)divisions);
                ret += spline->eval(idx, t);
            }
        }
        ret.insert(0, path[path.size() - 1]);
    }
    return ret;
}