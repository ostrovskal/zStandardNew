//
// Created by User on 28.05.2023.
//

#pragma once

class zTga {
public:
    zTga(u8* _ptr, u32 _size);
    virtual ~zTga() { SAFE_DELETE(ptr); }
    // сохранить
    bool save(const zStringUTF8& path);
    // вернуть ширину
    u32 getWidth() const { return width; }
    // вернуть высоту
    u32 getHeight() const { return height; }
    // вернуть bpp
    u32 getComponent() const { return comp; }
    // вернуть цветовые данные
    u8* getImage() const { return ptr; }
protected:
    u8* ptr{nullptr};
    u32 width{0}, height{0}, comp{0};
};
