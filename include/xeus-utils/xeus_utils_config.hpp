/***************************************************************************
* Copyright (c) 2022, Thorsten Beier                                  
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#ifndef XEUS_UTILS_CONFIG_HPP
#define XEUS_UTILS_CONFIG_HPP

// Project version
#define XEUS_UTILS_VERSION_MAJOR 0
#define XEUS_UTILS_VERSION_MINOR 1
#define XEUS_UTILS_VERSION_PATCH 0

// Composing the version string from major, minor and patch
#define XEUS_UTILS_CONCATENATE(A, B) XEUS_UTILS_CONCATENATE_IMPL(A, B)
#define XEUS_UTILS_CONCATENATE_IMPL(A, B) A##B
#define XEUS_UTILS_STRINGIFY(a) XEUS_UTILS_STRINGIFY_IMPL(a)
#define XEUS_UTILS_STRINGIFY_IMPL(a) #a

#define XEUS_UTILS_VERSION XEUS_UTILS_STRINGIFY(XEUS_UTILS_CONCATENATE(XEUS_UTILS_VERSION_MAJOR,   \
                 XEUS_UTILS_CONCATENATE(.,XEUS_UTILS_CONCATENATE(XEUS_UTILS_VERSION_MINOR,   \
                                  XEUS_UTILS_CONCATENATE(.,XEUS_UTILS_VERSION_PATCH)))))

#ifdef _WIN32
    #ifdef XEUS_UTILS_EXPORTS
        #define XEUS_UTILS_API __declspec(dllexport)
    #else
        #define XEUS_UTILS_API __declspec(dllimport)
    #endif
#else
    #define XEUS_UTILS_API
#endif

#endif