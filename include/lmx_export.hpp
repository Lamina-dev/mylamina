#pragma once

#ifdef LMX_DLL
    #ifdef _WIN32
        #ifdef LMC_BUILD
            #define LMC_API __declspec(dllexport)
        #else
            #define LMC_API __declspec(dllimport)
        #endif
        
        #ifdef LMVM_BUILD
            #define LMVM_API __declspec(dllexport)
        #else
            #define LMVM_API __declspec(dllimport)
        #endif
    #else
        #define LMC_API
        #define LMVM_API
    #endif
#else
    #define LMC_API
    #define LMVM_API
#endif

#define LMX_VERSION     0x00000001
#define LMX_MAGIC_NUM   0x4d4c5451
const unsigned int lmx_magic = LMX_MAGIC_NUM;
const unsigned int lmx_version = LMX_VERSION;
