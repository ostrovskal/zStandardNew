//
// Created by User on 04.06.2022.
//

#pragma once

class zRand {
public:
    enum { SEED1 = 214013L, SEED2 = 2531011L, SEED3 = 0x7fffUL };
    zRand() { seed = (long)::time(nullptr); }
    static int nextInt(int _b, int _e) { return (int)resolve(_b, _e); }
    static int nextInt(int _e) { return (int)resolve(0, _e); }
    static long nextLong(int _b, int _e) { return resolve(_b, _e); }
    static bool nextBoolean() { return resolve(0, 2) != 0; }
protected:
    static long seed;
    static long resolve(int _b, int _e) {
        seed *= SEED1; seed += SEED2;
        auto tmp = (seed >> 16) & SEED3;
        return _b + (tmp % (_e - _b));
    }
};
