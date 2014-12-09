#ifndef __TextureFeature_onboard__
#define __TextureFeature_onboard__

#include <opencv2/opencv.hpp>
using cv::Mat;

//
// interfaces
//
namespace TextureFeature
{
    struct Extractor
    {
        virtual int extract(const Mat &img, Mat &features) const = 0;
    };

    struct Classifier // identification
    {
        virtual int train(const Mat &features, const Mat &labels) = 0;
        virtual int predict(const Mat &test, Mat &result) const = 0;
    };

    struct Verifier   // same-notSame
    {
        virtual int train(const Mat &features, const Mat &labels) = 0;
        virtual int same(const Mat &a, const Mat &b) const = 0;
    };
};



//
// supplied impementations:
//

//
// extractors
//
cv::Ptr<TextureFeature::Extractor> createExtractorPixels(int resw=0, int resh=0);
// lbp variants
cv::Ptr<TextureFeature::Extractor> createExtractorLbp(int gridx=8, int gridy=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticLbp();
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapLbp(int gx=8,int gy=8,int over=5);
// four-patch lbp variants
cv::Ptr<TextureFeature::Extractor> createExtractorFPLbp(int gx=8, int gy=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticFpLbp();
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapFpLbp(int gx=8,int gy=8,int over=5);
// three-patch lbp variants
cv::Ptr<TextureFeature::Extractor> createExtractorTPLbp(int gx=8, int gy=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticTpLbp();
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapTpLbp(int gx=8,int gy=8,int over=5);
// reverse lbp circle
cv::Ptr<TextureFeature::Extractor> createExtractorBGC1(int gx=8, int gy=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticBGC1();
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapBGC1(int gx=8, int gy=8, int over=5);
// 1/2 lbp circle
cv::Ptr<TextureFeature::Extractor> createExtractorMTS(int gx=8, int gy=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticMTS();
cv::Ptr<TextureFeature::Extractor> createExtractorOverlapMTS(int gx=8,int gy=8,int over=5);
// phase based
cv::Ptr<TextureFeature::Extractor> createExtractorGaborLbp(int gx=8, int gy=8, int kernel_size=8);
cv::Ptr<TextureFeature::Extractor> createExtractorElasticGaborLbp(int kernel_siz=8);
// dct based
cv::Ptr<TextureFeature::Extractor> createExtractorDct();
// featurre2D abuse
cv::Ptr<TextureFeature::Extractor> createExtractorORBGrid(int g=10);
cv::Ptr<TextureFeature::Extractor> createExtractorSIFTGrid(int g=10);


//
// identification task (get the closest item from a trained db)
//
cv::Ptr<TextureFeature::Classifier> createClassifierNearest(int norm_flag=cv::NORM_L2);
cv::Ptr<TextureFeature::Classifier> createClassifierHist(int flag=cv::HISTCMP_CHISQR);
cv::Ptr<TextureFeature::Classifier> createClassifierCosine();
cv::Ptr<TextureFeature::Classifier> createClassifierKNN(int n=1);                // TODO: needs a way to get to the k-1 others
cv::Ptr<TextureFeature::Classifier> createClassifierSVM(double degree = 0.5,double gamma = 0.8,double coef0 = 0,double C = 0.99, double nu = 0.002, double p = 0.5);
cv::Ptr<TextureFeature::Classifier> createClassifierSVMMulti();
// reference
cv::Ptr<TextureFeature::Classifier> createClassifierEigen();
cv::Ptr<TextureFeature::Classifier> createClassifierFisher();

//
// verification task (same / not same)
//
cv::Ptr<TextureFeature::Verifier> createVerifierNearest(int flag=cv::NORM_L2);
cv::Ptr<TextureFeature::Verifier> createVerifierHist(int flag=cv::HISTCMP_CHISQR);
cv::Ptr<TextureFeature::Verifier> createVerifierFisher(int flag=cv::NORM_L2);
cv::Ptr<TextureFeature::Verifier> createVerifierSVM(int distfunc=2, float scale=0);
cv::Ptr<TextureFeature::Verifier> createVerifierEM(int distfunc=2, float scale=0);
cv::Ptr<TextureFeature::Verifier> createVerifierLR(int distfunc=2, float scale=0);
cv::Ptr<TextureFeature::Verifier> createVerifierBoost(int distfunc=2, float scale=0);



#endif // __TextureFeature_onboard__
