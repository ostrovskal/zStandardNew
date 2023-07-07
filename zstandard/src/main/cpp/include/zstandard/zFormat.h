//
// Created by User on 02.07.2023.
//

#pragma once

#include <iostream>
#include <type_traits>
#include <stdexcept>
#include <cstdio>

template <typename T> typename std::enable_if<std::is_integral<T>::value, long>::type normalize_arg(T arg) { return arg; }

template <typename T> typename std::enable_if<std::is_floating_point<T>::value, double>::type normalize_arg(T arg) { return arg; }

template <typename T> typename std::enable_if<std::is_pointer<T>::value, T>::type normalize_arg(T arg) { return arg; }

const char* normalize_arg(std::string const& str) { return str.c_str(); }

#define ENFORCE(A) if (!(A)) throw std::runtime_error("Type did not match format specifier")

template <typename Param> void validate_type_parameter(char format_specifier) {
    switch(format_specifier) {
        case 'f':
            ENFORCE(std::is_floating_point<Param>::value);
            break;
        case 'd':
            ENFORCE(std::is_integral<Param>::value);
            break;
        case 's':
            constexpr bool is_valid_c_str = std::is_same<Param, const char *>::value || std::is_same<Param, char *>::value;
            ENFORCE(is_valid_c_str);
            break;
        // We are not handling the full printf specification here for simplicity
        default: throw std::runtime_error("Недопустмый спецификатор формата - возможны только f, d и s");
    }
}

void safe_printf_impl(const char *str) {
    // We already own the lock so we might as well use the unlocked version
    for(; *str && (*str != '%' || *(++str) == '%'); ++str) putchar_unlocked(*str);
    if (*str) throw std::runtime_error("Too few arguments were passed to safe_printf");
}

template <typename Param, typename... Params> void safe_printf_impl(const char *str, Param parameter, Params... parameters) {
    // We already own the lock so we might as well use the unlocked version
    for(; *str && (*str != '%' || *(++str) == '%'); ++str) putchar_unlocked(*str);
    validate_type_parameter<Param>(*str);
    // If we have made it thus far, the format specifier agrees with the type parameter
    const char format[3] = {'%', *str, '\0'};
    printf(format, parameter);
    //Process the rest of the string
    safe_printf_impl(++str, parameters...);
}

template <typename... Params> void safe_printf(const char *str, Params const&... parameters) {
    // Retain file locking
    flockfile(stdout);
    safe_printf_impl(str, normalize_arg(parameters)...);
    funlockfile(stdout);
}
