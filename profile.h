#ifndef __profile_onboard__
#define __profile_onboard__

#include "opencv2/core/utility.hpp"
using namespace cv;
#include <iostream>
using namespace std;

#define get_ticks cv::getTickCount
#define get_freq  cv::getTickFrequency


struct Profile
{
    double dt(int64 t) const { return 1000.0*double(t/1000)/get_freq(); }

    cv::String name;
    int64 t; // accumulated time
    int64 c; // function calls
    double d_tc; // must cache, (getTickCount() etc will not be availabe in destructor) 
    double d_t;

    Profile(cv::String name)
        : name(name)
        , t(0)
        , c(0)
        , d_tc(0)
        , d_t(0)
    {}

    ~Profile()
    {
        fprintf(stderr, "%-24s %8lld ",name.c_str(),c);
        fprintf(stderr, "%13.6f ",d_tc);
        fprintf(stderr, "%13.6f ",d_t);
        fprintf(stderr, "%16lld",t);
        fprintf(stderr, "\n");
    }

    void tick(int64 delta)
    {
        if (delta <= 0)  return;

        c ++;
        t += delta;
        d_t  = dt(t); 
        d_tc = d_t/c;
    }


    struct Scope
    {
        Profile & p;
        int64 t;

        Scope(Profile & p)
            : p(p)
            , t(get_ticks())
        {}

        ~Scope()
        {
            p.tick(get_ticks() - t);
        }
    };
};

#define PROFILEX(s) static Profile _a_rose(s); Profile::Scope _is_a_rose_is(_a_rose);
#define PROFILE PROFILEX(__FUNCTION__)


#endif // __profile_onboard__

