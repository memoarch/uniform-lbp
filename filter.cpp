
#include <opencv2/imgproc.hpp>
using namespace cv;

#include "texturefeature.h"

#include <iostream>
using namespace std;

using namespace TextureFeature;

namespace TextureFeatureImpl
{



//
// unroll bitstrings to 1 byte per bit
// (in the hope, to work more nicely with L2 based classifiers)
//
struct FilterBits : public Filter
{
    static void bits8(Mat &bits, uchar b)
    {
        bits.push_back(((b&1)!=0));
        bits.push_back(((b&2)!=0));
        bits.push_back(((b&4)!=0));
        bits.push_back(((b&8)!=0));
        bits.push_back(((b&16)!=0));
        bits.push_back(((b&32)!=0));
        bits.push_back(((b&64)!=0));
        bits.push_back(((b&128)!=0));
    }
    static void bits4(Mat &bits, uchar b)
    {
        bits.push_back((((b&1 )^(b&2  ))!=0));
        bits.push_back((((b&4 )^(b&8  ))!=0));
        bits.push_back((((b&16)^(b&32 ))!=0));
        bits.push_back((((b&64)^(b&128))!=0));
    }
    static void bits2(Mat &bits, uchar b)
    {
        bits.push_back(((((b&1 )^(b&2 ))^((b&4 )^(b&8  )))!=0));
        bits.push_back(((((b&16)^(b&32))^((b&64)^(b&128)))!=0));
    }
    typedef void (*bitfun)(Mat &bits, uchar b);
    bitfun bits_;

    FilterBits(int nb=8) 
    {
        switch(nb)
        {
        default:
        case 8: bits_ = bits8; break;
        case 4: bits_ = bits4; break;
        case 2: bits_ = bits2; break;
        }
    }

    virtual int filter(const Mat &feature, Mat &dest) const
    {
        CV_Assert(feature.type()==CV_8U);
        Mat bits;
        for (size_t i=0; i<feature.total(); i++)
        {
            bits_(bits, feature.at<uchar>(i));
        }
        dest = bits.reshape(1, feature.rows);
        return 0;
    }
};


struct FilterWalshHadamard : public Filter
{
    int keep;

    FilterWalshHadamard(int k=0) : keep(k) {}

    template<class T>
    void fast_had(int ndim, int lev, T *in, T *out) const
    {
        int h = lev/2;
        for (int j=0; j<ndim/lev; j++)
        {
            for(int i=0; i<h; i++)
            {
                out[i]   = in[i] + in[i+h];
                out[i+h] = in[i] - in[i+h];
            }
            out += lev;
            in  += lev;
        }
    }

    virtual int filter(const Mat &src, Mat &dest) const
    {
        Mat h; src.convertTo(h,CV_32F);
        Mat h2(h.size(), h.type());
        for (int lev=src.total(); lev>2; lev/=2)
        {
            fast_had(src.total(), lev, h.ptr<float>(), h2.ptr<float>());
            if (lev>4) cv::swap(h,h2);
        }
        if (keep>0)
            dest = h2(Rect(0,0,std::min(keep,h2.cols-1),1));
        else
            dest = h2;
        return 0;
    }
};



struct FilterDct : public Filter
{
    int keep;

    FilterDct(int k=0) : keep(k) {}

    virtual int filter(const Mat &src, Mat &dest) const
    {
        Mat h; src.convertTo(h, CV_32F);
        Mat h2(h.size(), h.type());

        dft(h, h2, DFT_ROWS); // dft instead of dct solves pow2 issue

        Mat h3 = (keep>0) ?
                 h2(Rect(0, 0, std::min(keep, h2.cols-1), 1)) :
                 h2;

        dft(h3, dest, DCT_INVERSE | DFT_SCALE | DFT_ROWS);
        return 0;
    }
};


struct FilterRandomProjection : public Filter
{
    int K;

    FilterRandomProjection(int k) : K(k) {}

    const Mat & setup(int N) const
    {
        static Mat proj; // else it can't be const ;(
        if (proj.rows==N && proj.cols==K)
            return proj;

        proj = Mat(N, K, CV_32F);

        theRNG().state = 37183927;
        randn(proj, Scalar(0.5), Scalar(0.5));
        //randu(proj, Scalar(0), Scalar(1));

        for (int i=0; i<K; i++)
        {
            normalize(proj.col(i), proj.col(i));
        }
        return proj;
    }

    virtual int filter(const Mat &src, Mat &dest) const
    {
        const Mat &proj = setup(src.cols);

        Mat s; src.convertTo(s, CV_32F);
        dest = s * proj;
        return 0;
    }
};



//
// hellinger kernel (no reduction)
//
struct FilterHellinger : public Filter
{
    virtual int filter(const Mat &src, Mat &dest) const
    {
        src.convertTo(dest, CV_32F);
        float eps = 1e-7f;
        dest /= (sum(dest)[0] + eps); // L1
        sqrt(dest,dest);
        dest /= (norm(dest) + eps); // L2
        return 0;
    }
};

//
// pow(n,p) (no reduction) (-> generalized intersection)
//
struct FilterPow : public Filter
{
    double P;
    FilterPow(double p=0.25) : P(p) {}
    virtual int filter(const Mat &src, Mat &dest) const
    {
        src.convertTo(dest, CV_32F);
        cv::pow(dest,P,dest);
        return 0;
    }
};

//struct FilterKMeans : public Filter
//{
//    int K;
//    FilterKMeans(int k=10) : K(k) {}
//    virtual int filter(const Mat &src, Mat &dest) const
//    {
//        Mat labels,cent,srcf=src.reshape(1,src.total());
//        srcf.convertTo(srcf,CV_32F);
//        kmeans(srcf, K, labels, TermCriteria(), 3, KMEANS_PP_CENTERS, dest);
//        dest=dest.reshape(1,1);
//        return 0;
//    }
//};



} // TextureFeatureImpl


namespace TextureFeature
{
using namespace TextureFeatureImpl;


Ptr<Filter> createFilter(int filt)
{
    switch(filt)
    {
        case FIL_NONE:     break; 
        case FIL_HELL:     return makePtr<FilterHellinger>(); break;
        case FIL_POW:      return makePtr<FilterPow>(); break;
        case FIL_SQRT:     return makePtr<FilterPow>(0.5f); break;
        case FIL_WHAD4:    return makePtr<FilterWalshHadamard>(4000); break;
        case FIL_WHAD8:    return makePtr<FilterWalshHadamard>(8000); break;
        case FIL_RP:       return makePtr<FilterRandomProjection>(8000); break;
        case FIL_DCT2:     return makePtr<FilterDct>(2000); break;
        case FIL_DCT4:     return makePtr<FilterDct>(4000); break;
        case FIL_DCT6:     return makePtr<FilterDct>(6000); break;
        case FIL_DCT8:     return makePtr<FilterDct>(8000); break;
        case FIL_DCT12:    return makePtr<FilterDct>(12000); break;
        case FIL_DCT16:    return makePtr<FilterDct>(16000); break;
        case FIL_DCT24:    return makePtr<FilterDct>(24000); break;
        case FIL_BITS8:    return makePtr<FilterBits>(8); break;
        case FIL_BITS4:    return makePtr<FilterBits>(4); break;
        case FIL_BITS2:    return makePtr<FilterBits>(2); break;
        //case FIL_KMEANS16: return makePtr<FilterKMeans>(16); break;
        //case FIL_KMEANS64: return makePtr<FilterKMeans>(64); break;
        //case FIL_KMEANS256:return makePtr<FilterKMeans>(256); break;
//        default: cerr << "Filter " << filt << " is not yet supported." << endl; exit(-1);
    }
    return Ptr<Filter>();
}

} // TextureFeatureImpl

