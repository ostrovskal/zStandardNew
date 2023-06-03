//
// Created by User on 28.05.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zTga.h"

zTga::zTga(u8* _ptr, u32 _size) {
    auto head((HEADER_TGA*)_ptr); _ptr += sizeof(HEADER_TGA);
    width = head->wWidth; height = head->wHeight; ptr = _ptr;
    comp = head->bBitesPerPixel / 8;
    for(int i = 0; i < width * height; i++) {
        switch(comp) {
        case 3: case 4: {
                auto t(_ptr[0]); _ptr[0] = _ptr[2]; _ptr[2] = t;
            }
            break;
        }
        _ptr += comp;
    }
}

bool zTga::save(const zStringUTF8& path) {
    return z_tgaSaveFile(path, ptr, width, height, comp);
}
