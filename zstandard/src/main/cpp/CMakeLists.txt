
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
        zTga.cpp
        zCloudMail.cpp
        zCloudYandex.cpp
        zCloudDropbox.cpp
        zHttpRequest.cpp
        zSpline.cpp
)

target_include_directories(zstandard PUBLIC include)

target_link_libraries(zstandard log z)
