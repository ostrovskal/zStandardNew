//
// Created by User on 26.04.2023.
//

#pragma once

class zInterpolator {
public:
    enum INTERPOLATOR_TYPE {
        NONE, LINEAR, EASEINQUAD, EASEOUTQUAD, EASEINOUTQUAD, EASEINCUBIC, EASEOUTCUBIC,
        EASEINOUTCUBIC, EASEINQUART, EASEINEXPO, EASEOUTEXPO
    };
    struct InterpolatorParams {
        INTERPOLATOR_TYPE type;
        float dest; int frames;
    };
    // конструктор
    zInterpolator() { }
    // деструктор
    ~zInterpolator() { clear(); }
    // инициализация
    void init(float _start, bool _loop) { start = _start; loop = _loop; }
    // добавление анимации
    void add(float _dest, INTERPOLATOR_TYPE _type, int _frames) { params += { _type, _dest, _frames }; }
    // выполнить кадр анимации
    bool update(float &p);
    // очистить
    void clear() { params.clear(); index = 0; }
    // проверка на наличие анимаций
    bool isEmpty() const { return params.isEmpty(); }
    // текущий кадр/количество кадров
    int frame{0}, frames{0};
private:
    // начальное значение/конечное значение/самое первое значение
    float vStart{0.0f}, vDest{0.0f}, start{0.0f};
    // индекс анимации/признак зацикливания
    int index{0}; bool loop{false};
    // тип интерполятора
    INTERPOLATOR_TYPE type{NONE};
    // массив анимаций
    zArray<InterpolatorParams> params;
    // установка анимации
    bool set();
    // вычисление интерполятора
    static float formula(INTERPOLATOR_TYPE type, float t, float b, float d, float c);
};
