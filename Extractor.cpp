#include <vector>
using std::vector;
#include <iostream>
using std::cerr;
using std::endl;

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
using namespace cv;

#include "TextureFeature.h"




//
// this is the most simple one.
//
struct ExtractorPixels : public TextureFeature::Extractor
{
    int resw,resh;

    ExtractorPixels(int resw=0,int resh=0): resw(resw), resh(resh) {}

    // TextureFeature::Extractor
    virtual int extract(const Mat &img, Mat &features) const
    {
        if (resw>0 && resh>0)
            resize(img, features, Size(resw,resh));
        else
            features = img;
        features = features.reshape(1,1);
        return features.total() * features.elemSize();
    }
};


struct FeatureLbp
{
    int operator() (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> feature(I.size(),0);
        Mat_<uchar> img(I);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                uchar cen = img(r,c);
                v |= (img(r-1,c  ) > cen) << 0;
                v |= (img(r-1,c+1) > cen) << 1;
                v |= (img(r  ,c+1) > cen) << 2;
                v |= (img(r+1,c+1) > cen) << 3;
                v |= (img(r+1,c  ) > cen) << 4;
                v |= (img(r+1,c-1) > cen) << 5;
                v |= (img(r  ,c-1) > cen) << 6;
                v |= (img(r-1,c-1) > cen) << 7;
                feature(r,c) = v;
            }
        }
        fI = feature;
        return 256;
    }
};

//
// "Description of Interest Regions with Center-Symmetric Local Binary Patterns"
// (http://www.ee.oulu.fi/mvg/files/pdf/pdf_750.pdf).
//    (w/o threshold)
//
struct FeatureCsLbp
{
    int operator() (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> feature(I.size(),0);
        Mat_<uchar> img(I);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                v |= (img(r-1,c  ) > img(r+1,c  )) << 0;
                v |= (img(r-1,c+1) > img(r+1,c-1)) << 1;
                v |= (img(r  ,c+1) > img(r  ,c-1)) << 2;
                v |= (img(r+1,c+1) > img(r-1,c-1)) << 3;
                feature(r,c) = v;
            }
        }
        fI = feature;
        return 16;
    }
};


//
// / \
// \ /
//
struct FeatureDiamondLbp
{
    int operator() (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> feature(I.size(),0);
        Mat_<uchar> img(I);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                v |= (img(r-1,c  ) > img(r  ,c+1)) << 0;
                v |= (img(r  ,c+1) > img(r+1,c  )) << 1;
                v |= (img(r+1,c  ) > img(r  ,c-1)) << 2;
                v |= (img(r  ,c-1) > img(r-1,c  )) << 3;
                feature(r,c) = v;
            }
        }
        fI = feature;
        return 16;
    }
};


//  _ _
// |   |
// |_ _|
//
struct FeatureSquareLbp
{
    int operator() (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> feature(I.size(),0);
        Mat_<uchar> img(I);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                v |= (img(r-1,c-1) > img(r-1,c+1)) << 0;
                v |= (img(r-1,c+1) > img(r+1,c+1)) << 1;
                v |= (img(r+1,c+1) > img(r+1,c-1)) << 2;
                v |= (img(r+1,c-1) > img(r-1,c-1)) << 3;
                feature(r,c) = v;
            }
        }
        fI = feature;
        return 16;
    }
};


//
// just run around in a circle (instead of comparing to the center) ..
//
struct FeatureBGC1
{
    int operator () (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> feature(I.size(),0);
        Mat_<uchar> img(I);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                v |= (img(r-1,c  ) > img(r-1,c-1)) << 0;
                v |= (img(r-1,c+1) > img(r-1,c  )) << 1;
                v |= (img(r  ,c+1) > img(r-1,c+1)) << 2;
                v |= (img(r+1,c+1) > img(r  ,c+1)) << 3;
                v |= (img(r+1,c  ) > img(r+1,c+1)) << 4;
                v |= (img(r+1,c-1) > img(r+1,c  )) << 5;
                v |= (img(r  ,c-1) > img(r+1,c-1)) << 6;
                v |= (img(r-1,c-1) > img(r  ,c-1)) << 7;
                feature(r,c) = v;
            }
        }
        fI = feature;
        return 256;
    }
};


//
// Antonio Fernandez, Marcos X. Alvarez, Francesco Bianconi:
// "Texture description through histograms of equivalent patterns"
//
//    basically, this is just 1/2 of the full lbp-circle (4bits / 16 bins only!)
//
struct FeatureMTS
{
    int operator () (const Mat &I, Mat &fI) const
    {
        Mat_<uchar> img(I);
        Mat_<uchar> fea(I.size(), 0);
        const int m=1;
        for (int r=m; r<img.rows-m; r++)
        {
            for (int c=m; c<img.cols-m; c++)
            {
                uchar v = 0;
                uchar cen = img(r,c);
                v |= (img(r-1,c  ) > cen) << 0;
                v |= (img(r-1,c+1) > cen) << 1;
                v |= (img(r  ,c+1) > cen) << 2;
                v |= (img(r+1,c+1) > cen) << 3;
                fea(r,c) = v;
            }
        }
        fI = fea;
        return 16;
    }
};


//
// Wolf, Hassner, Taigman : "Descriptor Based Methods in the Wild"
// 3.1 Three-Patch LBP Codes
//
struct FeatureTPLbp
{
    int operator () (const Mat &img, Mat &features) const
    {
        Mat_<uchar> I(img);
        Mat_<uchar> fI(I.size(), 0);
        const int border=2;
        for (int r=border; r<I.rows-border; r++)
        {
            for (int c=border; c<I.cols-border; c++)
            {
                uchar v = 0;
                v |= ((I(r,c) - I(r  ,c-2)) > (I(r,c) - I(r-2,c  ))) * 1;
                v |= ((I(r,c) - I(r-1,c-1)) > (I(r,c) - I(r-1,c+1))) * 2;
                v |= ((I(r,c) - I(r-2,c  )) > (I(r,c) - I(r  ,c+2))) * 4;
                v |= ((I(r,c) - I(r-1,c+1)) > (I(r,c) - I(r+1,c+1))) * 8;
                v |= ((I(r,c) - I(r  ,c+2)) > (I(r,c) - I(r+1,c  ))) * 16;
                v |= ((I(r,c) - I(r+1,c+1)) > (I(r,c) - I(r+1,c-1))) * 32;
                v |= ((I(r,c) - I(r+1,c  )) > (I(r,c) - I(r  ,c-2))) * 64;
                v |= ((I(r,c) - I(r+1,c-1)) > (I(r,c) - I(r-1,c-1))) * 128;
                fI(r,c) = v;
            }
        }
        features = fI;
        return 256;
    }
};



//
// Wolf, Hassner, Taigman : "Descriptor Based Methods in the Wild"
// 3.2 Four-Patch LBP Codes (4bits / 16bins only !)
//
struct FeatureFPLbp
{
    int operator () (const Mat &img, Mat &features) const
    {
        Mat_<uchar> I(img);
        Mat_<uchar> fI(I.size(), 0);
        const int border=2;
        for (int r=border; r<I.rows-border; r++)
        {
            for (int c=border; c<I.cols-border; c++)
            {
                uchar v = 0;
                v |= ((I(r  ,c+1) - I(r+2,c+2)) > (I(r  ,c-1) - I(r-2,c-2))) * 1;
                v |= ((I(r+1,c+1) - I(r+2,c  )) > (I(r-1,c-1) - I(r-2,c  ))) * 2;
                v |= ((I(r+1,c  ) - I(r+2,c-2)) > (I(r-1,c  ) - I(r-2,c+2))) * 4;
                v |= ((I(r+1,c-1) - I(r  ,c-2)) > (I(r-1,c+1) - I(r  ,c+2))) * 8;
                fI(r,c) = v;
            }
        }
        features = fI;
        return 16;
    }
};





static void hist_patch(const Mat_<uchar> &fI, Mat &histo, int histSize=256)
{
    Mat_<float> h(1,histSize,0.0f);
    for (int i=0; i<fI.rows; i++)
    {
        for (int j=0; j<fI.cols; j++)
        {
            h( int(fI(i,j)) ) += 1.0f;
        }
    }
    histo.push_back(h.reshape(1,1));
}


struct GriddedHist
{
    int GRIDX,GRIDY;

    GriddedHist(int gridx=8, int gridy=8)
        : GRIDX(gridx)
        , GRIDY(gridy)
    {}

    void hist(const Mat &feature, Mat &histo, int histSize=256) const
    {
        histo.release();
        int sw = feature.cols/GRIDX;
        int sh = feature.rows/GRIDY;
        for (int i=0; i<GRIDX; i++)
        {
            for (int j=0; j<GRIDY; j++)
            {
                Mat patch(feature, Range(j*sh,(j+1)*sh), Range(i*sw,(i+1)*sw));
                hist_patch(patch, histo, histSize);
            }
        }
        normalize(histo.reshape(1,1),histo);
    }
};



struct PyramidGrid
{
    void hist_level(const Mat &feature, Mat &histo, int GRIDX, int GRIDY,int histSize=256) const
    {
        int sw = feature.cols/GRIDX;
        int sh = feature.rows/GRIDY;
        for (int i=0; i<GRIDX; i++)
        {
            for (int j=0; j<GRIDY; j++)
            {
                Mat patch(feature, Range(j*sh,(j+1)*sh), Range(i*sw,(i+1)*sw));
                hist_patch(patch, histo, histSize);
            }
        }
    }

    void hist(const Mat &feature, Mat &histo, int histSize=256) const
    {
        histo.release();
        int levels[] = {5,6,7,8};
        for (int i=0; i<4; i++)
        {
            hist_level(feature,histo,levels[i],levels[i],histSize);
        }
        normalize(histo.reshape(1,1),histo);
    }
};


struct OverlapGridHist : public GriddedHist
{
    int over,over2;

    OverlapGridHist(int gridx=8, int gridy=8, int over=0)
        : GriddedHist(gridx, gridy)
        , over(over)
        , over2(over*2)
    { }

    void hist(const Mat &feature, Mat &histo, int histSize=256) const
    {
        histo.release();
        int sw = (feature.cols - over2)/GRIDX;
        int sh = (feature.rows - over2)/GRIDY;
        for (int r=over; r<feature.rows-sh; r+=sh)
        {
            for (int c=over; c<feature.cols-sw; c+=sw)
            {
                Rect patch(c-over, r-over, sw+over2, sh+over2);
                hist_patch(feature(patch), histo, histSize);
            }
        }
        normalize(histo.reshape(1,1),histo);
    }
};



//
// hardcoded to funneled, 90x90 images.
//
//   the (offline) train thing uses a majority vote over rects,
//   the current impl concatenated histograms (the majority scheme seems to play nicer)
//
struct ElasticParts
{
    void hist(const Mat &feature, Mat &histo, int histSize=256) const
    {
        const int nparts = 64;
        Rect parts[nparts] =
        {
            Rect(15,23,48, 5), // 0.701167 // 1.504 // gen5
            Rect(24, 0,22,11),            Rect(24,23,55, 4),            Rect(56,21,34, 7),            Rect(24, 9,25,10),
            Rect(25,23,52, 4),            Rect( 0,52,60, 4),            Rect(40,27,35, 7),            Rect(36,59,31, 8),
            Rect( 5,24,38, 6),            Rect( 5, 0,21,11),            Rect( 4, 2,24,10),            Rect( 1,51,36, 6),
            Rect(25,29,18,13),            Rect(10, 1,26, 9),            Rect(50,27,25,10),            Rect(42,17,17,14),
            Rect( 6,26,30, 8),            Rect(34, 6,13,19),            Rect(65, 1,24,10),            Rect(20,24,37, 6),
            Rect(22,22,41, 6),            Rect(60,22,30, 7),            Rect(53,21,37, 6),            Rect(32,19,13,19),
            Rect(45,17,29, 8),            Rect(30,23,55, 4),            Rect(52,17,30, 8),            Rect(21,27,44, 5),
            Rect(39,27,38, 6),            Rect(53,12,28, 8),            Rect(22,29,21,11),            Rect(16, 6,35, 7),
            Rect(31,20,11,22),            Rect(14,24,55, 4),            Rect(37,15,13,19),            Rect(30,61,38, 6),
            Rect(76,11,14,17),            Rect(38,13,25,10),            Rect(26,30,17,14),            Rect(25,30,20,12),
            Rect( 1, 6,17,14),            Rect( 5, 8,22,11),            Rect(56,11,24,10),            Rect(69,14,20,12),
            Rect(41,20,16,15),            Rect(22,22,43, 5),            Rect(64,58,16,15),            Rect(70,42,13,19),
            Rect(39,14,15,16),            Rect(25,60,30, 8),            Rect(10,64,23,10),            Rect(26, 1,17,14),
            Rect(46,77,20,12),            Rect(56, 8,15,16),            Rect(66,55,19,13),            Rect( 8,64,28, 8),
            Rect(70,53,20,12),            Rect(62, 7,12,20),            Rect( 2,24,56, 4),            Rect(25,48,25,10),
            Rect(44,27,34, 7),            Rect(58,21,31, 8),            Rect(49,80,16,10)
        };

        histo.release();
        for (size_t k=0; k<nparts; k++)
        {
            hist_patch(feature(parts[k]), histo, histSize);
        }
        normalize(histo.reshape(1,1),histo);
    }
};



//
//
// layered base for lbph,
//  * calc features on the whole image,
//  * calculate the hist on a set of rectangles
//    (which could come from a grid, or a Rects based model).
//
template <typename Feature, typename Grid>
struct GenericExtractor : public TextureFeature::Extractor
{
    Feature ext;
    Grid grid;

    GenericExtractor(const Feature &ext, const Grid &grid)
        : grid(grid)
        , ext(ext)
    {}

    // TextureFeature::Extractor
    virtual int extract(const Mat &img, Mat &features) const
    {
        Mat fI;
        int histSize = ext(img, fI);
        grid.hist(fI, features, histSize);
        return features.total() * features.elemSize();
    }
};


template <typename Grid>
struct CombinedExtractor : public TextureFeature::Extractor
{
    FeatureCsLbp cslbp;
    FeatureDiamondLbp dialbp;
    FeatureSquareLbp sqlbp;
    Grid grid;

    CombinedExtractor(const Grid &grid)
        : grid(grid)
    {}

    // TextureFeature::Extractor
    virtual int extract(const Mat &img, Mat &features) const
    {
        Mat fI, f;
        int histSize = cslbp(img, f);
        grid.hist(f, fI, histSize);
        features.push_back(fI.reshape(1,1));

        histSize = dialbp(img, f);
        grid.hist(f, fI, histSize);
        features.push_back(fI.reshape(1,1));

        histSize = sqlbp(img, f);
        grid.hist(f, fI, histSize);
        features.push_back(fI.reshape(1,1));
        
        features = features.reshape(1,1);
        return features.total() * features.elemSize();
    }
};



//
// concat histograms from lbp-like features generated from a bank of gabor filtered images
//
template <typename Feature, typename Grid>
struct ExtractorGabor : public GenericExtractor<Feature,Grid>
{
    Size kernel_size;

    ExtractorGabor(const Feature &ext, const Grid &grid, int kernel_siz=8)
        : GenericExtractor<Feature,Grid>(ext, grid)
        , kernel_size(kernel_siz, kernel_siz)
    {}

    void gabor(const Mat &src_f, Mat &features,double sigma, double theta, double lambda, double gamma, double psi) const
    {
        Mat dest,dest8u,his;
        cv::filter2D(src_f, dest, CV_32F, getGaborKernel(kernel_size, sigma,theta, lambda, gamma, psi));
        dest.convertTo(dest8u, CV_8U);
        GenericExtractor<Feature,Grid>::extract(dest8u, his);
        features.push_back(his.reshape(1, 1));
    }

    virtual int extract(const Mat &img, Mat &features) const
    {
        Mat src_f;
        img.convertTo(src_f, CV_32F, 1.0/255.0);
        gabor(src_f, features, 8,4,90,15,0);
        gabor(src_f, features, 8,4,45,30,1);
        gabor(src_f, features, 8,4,45,45,0);
        gabor(src_f, features, 8,4,90,60,1);
        features = features.reshape(1,1);
        return features.total() * features.elemSize();
    }
};



//
// grid it into 8x8 image patches, do a dct on each,
//  concat downsampled 4x4(topleft) result to feature vector.
//
struct ExtractorDct : public TextureFeature::Extractor
{
    int grid;

    ExtractorDct() : grid(8) {}

    virtual int extract( const Mat &img, Mat &features ) const
    {
        Mat src;
        img.convertTo(src,CV_32F,1.0/255.0);
        for(int i=0; i<src.rows-grid; i+=grid)
        {
            for(int j=0; j<src.cols-grid; j+=grid)
            {
                Mat d;
                dct(src(Rect(i,j,grid,grid)),d);
                // downsampling is just a ROI operation here, still we need a clone()
                Mat e = d(Rect(0,0,grid/2,grid/2)).clone();
                features.push_back(e.reshape(1,1));
            }
        }
        features = features.reshape(1,1);
        return features.total() * features.elemSize();
    }
};



template < class Descriptor >
struct ExtractorGridFeature : public TextureFeature::Extractor
{
    int grid;

    ExtractorGridFeature(int g=10) : grid(g) {}

    virtual int extract(const Mat &img, Mat &features) const
    {
        float gw = float(img.cols) / grid;
        float gh = float(img.rows) / grid;
        vector<KeyPoint> kp;
        for (float i=gh/2; i<img.rows-gh; i+=gh)
        {
            for (float j=gw/2; j<img.cols-gw; j+=gw)
            {
                KeyPoint k(j, i, gh);
                kp.push_back(k);
            }
        }
        Ptr<Feature2D> f2d = Descriptor::create();
        f2d->compute(img, kp, features);

        features = features.reshape(1,1);
        return features.total() * features.elemSize();
    }
};

typedef ExtractorGridFeature<ORB> ExtractorORBGrid;
typedef ExtractorGridFeature<BRISK> ExtractorBRISKGrid;
typedef ExtractorGridFeature<xfeatures2d::FREAK> ExtractorFREAKGrid;
typedef ExtractorGridFeature<xfeatures2d::SIFT> ExtractorSIFTGrid;
typedef ExtractorGridFeature<xfeatures2d::BriefDescriptorExtractor> ExtractorBRIEFGrid;



//
// 'factory' functions (aka public api)
//

cv::Ptr<TextureFeature::Extractor> createExtractorPixels(int resw, int resh)
{
    return makePtr<ExtractorPixels>(resw, resh);
}

cv::Ptr<TextureFeature::Extractor> createExtractorLbp(int gx, int gy)
{
    return makePtr< GenericExtractor<FeatureLbp,GriddedHist> >(FeatureLbp(), GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticLbp()
{
    return makePtr< GenericExtractor<FeatureLbp,ElasticParts> >(FeatureLbp(), ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapLbp(int gx, int gy, int over)
{
    return makePtr< GenericExtractor<FeatureLbp,OverlapGridHist> >(FeatureLbp(), OverlapGridHist(gx, gy, over));
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidLbp()
{
    return makePtr< GenericExtractor<FeatureLbp,PyramidGrid> >(FeatureLbp(), PyramidGrid());
}

cv::Ptr<TextureFeature::Extractor> createExtractorFPLbp(int gx, int gy)
{
    return makePtr< GenericExtractor<FeatureFPLbp,GriddedHist> >(FeatureFPLbp(), GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticFpLbp()
{
    return makePtr< GenericExtractor<FeatureFPLbp,ElasticParts> >(FeatureFPLbp(), ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapFpLbp(int gx, int gy, int over)
{
    return makePtr< GenericExtractor<FeatureFPLbp,OverlapGridHist> >(FeatureFPLbp(), OverlapGridHist(gx, gy, over));
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidFpLbp()
{
    return makePtr< GenericExtractor<FeatureFPLbp,PyramidGrid> >(FeatureFPLbp(), PyramidGrid());
}

cv::Ptr<TextureFeature::Extractor> createExtractorTPLbp(int gx, int gy)
{
    return makePtr< GenericExtractor<FeatureTPLbp,GriddedHist> >(FeatureTPLbp(), GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticTpLbp()
{
    return makePtr< GenericExtractor<FeatureTPLbp,ElasticParts> >(FeatureTPLbp(), ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidTpLbp()
{
    return makePtr< GenericExtractor<FeatureTPLbp,PyramidGrid> >(FeatureTPLbp(), PyramidGrid());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapTpLbp(int gx, int gy, int over)
{
    return makePtr< GenericExtractor<FeatureTPLbp,OverlapGridHist> >(FeatureTPLbp(), OverlapGridHist(gx, gy, over));
}

cv::Ptr<TextureFeature::Extractor> createExtractorMTS(int gx, int gy)
{
    return makePtr< GenericExtractor<FeatureMTS,GriddedHist> >(FeatureMTS(), GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticMTS()
{
    return makePtr< GenericExtractor<FeatureMTS,ElasticParts> >(FeatureMTS(), ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidMTS()
{
    return makePtr< GenericExtractor<FeatureMTS,PyramidGrid> >(FeatureMTS(), PyramidGrid());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapMTS(int gx, int gy, int over)
{
    return makePtr< GenericExtractor<FeatureMTS,OverlapGridHist> >(FeatureMTS(), OverlapGridHist(gx, gy, over));
}

cv::Ptr<TextureFeature::Extractor> createExtractorBGC1(int gx, int gy)
{
    return makePtr< GenericExtractor<FeatureBGC1,GriddedHist> >(FeatureBGC1(), GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticBGC1()
{
    return makePtr< GenericExtractor<FeatureBGC1,ElasticParts> >(FeatureBGC1(), ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidBGC1()
{
    return makePtr< GenericExtractor<FeatureBGC1,PyramidGrid> >(FeatureBGC1(), PyramidGrid());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapBGC1(int gx, int gy, int over)
{
    return makePtr< GenericExtractor<FeatureBGC1,OverlapGridHist> >(FeatureBGC1(), OverlapGridHist(gx, gy, over));
}

cv::Ptr<TextureFeature::Extractor> createExtractorCombined(int gx, int gy)
{
    return makePtr< CombinedExtractor<GriddedHist> >(GriddedHist(gx, gy));
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticCombined()
{
    return makePtr< CombinedExtractor<ElasticParts> >(ElasticParts());
}
cv::Ptr<TextureFeature::Extractor> createExtractorPyramidCombined()
{
    return makePtr< CombinedExtractor<PyramidGrid> >(PyramidGrid());
}
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapCombined(int gx, int gy, int over)
{
    return makePtr< CombinedExtractor<OverlapGridHist> >(OverlapGridHist(gx, gy, over));
}


cv::Ptr<TextureFeature::Extractor> createExtractorGaborLbp(int gx, int gy, int kernel_siz)
{
    return makePtr< ExtractorGabor<FeatureLbp,GriddedHist> >(FeatureLbp(), GriddedHist(gx, gy), kernel_siz);
}
cv::Ptr<TextureFeature::Extractor> createExtractorElasticGaborLbp(int kernel_siz)
{
    return makePtr< ExtractorGabor<FeatureLbp,ElasticParts> >(FeatureLbp(), ElasticParts(), kernel_siz);
}

cv::Ptr<TextureFeature::Extractor> createExtractorDct()
{
    return makePtr<ExtractorDct>();
}

cv::Ptr<TextureFeature::Extractor> createExtractorORBGrid(int g)
{
    return makePtr<ExtractorORBGrid>(g);
}

cv::Ptr<TextureFeature::Extractor> createExtractorSIFTGrid(int g)
{
    return makePtr<ExtractorSIFTGrid>(g);
}
