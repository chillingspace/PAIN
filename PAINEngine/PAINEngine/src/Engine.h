#pragma once

#ifdef MYENGINE_EXPORTS
#define MYENGINE_API __declspec(dllexport)
#else
#define MYENGINE_API __declspec(dllimport)
#endif

class MYENGINE_API Engine {
public:
    void Hello();
};
