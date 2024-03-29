//
// Created by User on 04.06.2022.
//

#pragma once

class zRand {
public:
    enum { SEED1 = 214013L, SEED2 = 2531011L, SEED3 = 0x7fffUL };
    static int nextInt(int _b, int _e) { return (int)resolve(_b, _e); }
    static int nextInt(int _e) { return (int)resolve(0, _e); }
    static long nextLong(int _b, int _e) { return resolve(_b, _e); }
    static bool nextBoolean() { return resolve(0, 2) != 0; }
    static u32 nextColor(int a) {
        auto r(nextInt(32, 256) << 16), g(nextInt(32, 256) << 8), b(nextInt(32, 256));
        return ((a << 24) | r | g | b);
    }
protected:
    static long seed;
    static long resolve(int _b, int _e) {
        if(!seed) seed = (long)::time(nullptr);
        seed *= SEED1; seed += SEED2;
        auto tmp((seed >> 16) & SEED3);
        return _b + (tmp % (_e - _b));
    }
};
