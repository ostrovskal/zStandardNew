
cmake_minimum_required(VERSION 3.22.1)

project(zstandard VERSION 1.0.0 LANGUAGES CXX)

include_directories(D:/Dev/Include/)

add_library(zstandard STATIC
        zstandard.cpp
        zTypes.cpp
        zString.cpp
        zClass.cpp
        zFile.cpp
        zHMAC_SHA1.cpp
        zJpg.cpp
        zJSON.cpp
        zPng.cpp
        zSettings.cpp
        zUnzip.cpp
        zZip.cpp
        zXml.cpp
        zHtml.cpp
        zSHA1.cpp
        zIOfile.cpp
        zShader.cpp
        zGL.cpp
        zJniHelper.cpp
        zInterpolator.cpp
        gl3stub.cpp
        zStringUTF8.cpp
)

target_include_directories(zstandard PUBLIC include)

target_link_libraries(zstandard log z)

#cmake_minimum_required(VERSION 3.4.1)
#set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
#add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
#add_library(native-activity SHARED main.cpp)
#target_include_directories(native-activity PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
#target_link_libraries(native-activity android native_app_glue EGL GLESv1_CM log)
