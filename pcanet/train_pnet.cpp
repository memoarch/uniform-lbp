#include "opencv2/opencv.hpp"
using namespace cv;

#include <cstdio>
#include <iostream>
using namespace std;

#include "opencv2/core/core.hpp"
#include "opencv2/ml/ml.hpp"

#include <cstdio>
#include <vector>
#include <iostream>

using namespace std;
using namespace cv;
using namespace cv::ml;

#include "../texturefeature.h"
#include "PNet.cpp"
using TextureFeature::PNet;


#include "opencv2/datasets/fr_lfw.hpp"

struct TrainParam
{
    float v,m,M;
    TrainParam(float v, float m, float M) : v(v), m(m), M(M) {}
    void mutate(float learn)
    {
        v = theRNG().uniform(v-learn, v+learn);
        if (v<m) v=m;
        if (v>M) v=M;
    }
    void crossbreed(const TrainParam &sup, float learn)
    {
        v *= (1.0f - learn);
        v += sup.v * learn;
    }
};

//#define SINGLE_LS

struct TrainParams
{
    vector<TrainParam> tp;
    float accuracy;
    TrainParams()
        : accuracy(0.5f)
    {
        reset();
    }
    void reset()
    {
        tp.clear();
        tp.push_back(TrainParam(4.0f,2.0f,16.0f)); // grid
#ifdef SINGLE_LS
        tp.push_back(TrainParam(12.0f,4.0f,32.0f)); // ps
#endif // SINGLE_LS
        tp.push_back(TrainParam(4.0f,2.0f,6.0f)); // layer1
        tp.push_back(TrainParam(4.0f,2.0f,6.0f)); // layer2
        for (int i=0; i<4*20; i++)
        {
#ifndef SINGLE_LS
            tp.push_back(TrainParam(12.0f, 4.0f, 32.0f)); // ps
#endif //SINGLE_LS
            tp.push_back(TrainParam(0.1f,-0.2f,3.0f)); // ps
            tp.push_back(TrainParam(0.27f,-0.3f,4.0f)); // ps
        }
        mutate(9.0f);
    }
    void mutate(float learn)
    {
        for (size_t i=0; i<tp.size(); i++)
        {
            tp[i].mutate(learn);
        }
    }
    void crossbreed(const TrainParams &sup, float learn)
    {
        for (size_t i=0; i<tp.size(); i++)
        {
            tp[i].crossbreed(sup.tp[i], learn);
        }
    }
    String str()
    {
#ifdef SINGLE_LS
        return format("%2d %2d %2d %2d %3.3f", int(tp[0].v), int(tp[1].v),int(tp[2].v),int(tp[3].v), accuracy);
#else 
        return format("%2d %2d %2d %3.3f", int(tp[0].v), int(tp[1].v), int(tp[2].v), accuracy);
#endif // SINGLE_LS
    }
    String str2()
    {
        String s = format("%3.3f\n", accuracy);
#ifdef SINGLE_LS
        s += format("hgrid=%d; hsize=%d;\n", int(tp[0].v),(1 <<(int(tp[3].v))));
        int nf = int(tp[1].v);
        int i=4;
        for (int j=0; j<int(tp[2].v); j++, i+=2)
            s += format("s1.addFilter(%d, %3.3f, %3.3f);\n", nf, tp[i].v, tp[i+1].v);
        for (int j=0; j<int(tp[3].v); j++, i+=2)
            s += format("s2.addFilter(%d, %3.3f, %3.3f);\n", nf, tp[i].v, tp[i+1].v);
#else 
        s += format("hgrid=%d; hsize=%d;\n", int(tp[0].v),(1 <<(int(tp[2].v))));
        int i=3;
        for (int j=0; j<int(tp[1].v); j++, i+=3)
            s += format("s1.addFilter(%d, %3.3f, %3.3f);\n", int(tp[i].v), tp[i+1].v, tp[i+2].v);
        for (int j=0; j<int(tp[2].v); j++, i+=3)
            s += format("s2.addFilter(%d, %3.3f, %3.3f);\n", int(tp[i].v), tp[i+1].v, tp[i+2].v);
#endif // SINGLE_LS

        return s;
    }
};
struct Training
{
    enum { Train, Test };
    enum {NTP=7};
    PNet cnn;
    TrainParams tp[NTP];
    TrainParams work;
    Ptr<cv::ml::SVM> svm;
    Mat feat;
    Mat labl;
    Ptr<cv::datasets::FR_lfw> dataset;

    int skip;
    int skipTest;
    int wi;
    char tm;
    float aworst, abest, accuracy;

    String path;

    Training(String path)
        : svm(ml::SVM::create())
        , dataset(cv::datasets::FR_lfw::create())
        , skip(25)
        , skipTest(3)
        , path(path)
        , tm('w')
    {
        dataset->load(path);
    }
    ~Training()
    {
        cerr << endl << "final" << endl;
        dump();
    }

    void dump()
    {
        for (int i=0; i<NTP; i++)
        {
            cerr << tp[i].str2() << endl;
        }
    }
    int setWorst()
    {
        float acc = 100000;
        float mac = -1;
        int bi=0;
        wi=0;
        for (size_t i=0; i<NTP; i++)
        {
            if (acc > tp[i].accuracy)
            {
                aworst = acc = tp[i].accuracy;
                wi=i;
            }
            if (mac < tp[i].accuracy)
            {
                abest = mac = tp[i].accuracy;
                bi=i;
            }
        }
        tm = 'w';
        if (theRNG().uniform(0,8) < 2)
        {
            wi = bi;
            tm = 'b';
        }
        work = tp[wi];

        //t = tp[bi];
        //cerr << "worst " << wi << " -> " << bi << " : " << work.accuracy << " " << accuracy << endl;
        if (work.accuracy<=aworst)
        {
           work.reset();
           tm = 'r';
        } 
        //else
        //if (work.accuracy  accuracy) // we got worse
        //{
        //   // work.reset();
        //    int ri = theRNG().uniform(0,NTP);
        //    //cerr << "cross " << wi << " -> " << ri << endl;
        //    work.crossbreed(tp[ri], (1.0f - work.accuracy));
        //    tm = 'c';
        //}
        float learn = 0.2 * (1.0f-work.accuracy) * (1.0f-work.accuracy);
        work.mutate(learn);

        //cnn = PNet(int(work.tp[0].v), int(work.tp[1].v));
#ifdef SINGLE_LS
        cnn = PNet(int(work.tp[0].v), (1 << (int(work.tp[3].v))));
        int fs = int(work.tp[1].v);
        int n1 = int(work.tp[2].v);
        int n2 = int(work.tp[3].v);
        int f=4;
        PNet::Stage &s1 = cnn.stages[0];
        for (int i=0; i<n1; i++)
        {
            s1.addFilter(fs, work.tp[f].v, work.tp[f+1].v);
            f += 2;
        }
        PNet::Stage &s2 = cnn.stages[1];
        for (int i=0; i<n2; i++)
        {
            s2.addFilter(fs, work.tp[f].v, work.tp[f+1].v);
            f += 2;
        }
#else
        cnn = PNet(int(work.tp[0].v), (1 << (int(work.tp[2].v))));
        int n1 = int(work.tp[1].v);
        int n2 = int(work.tp[2].v);
        int f=3;
        PNet::Stage &s1 = cnn.stages[0];
        for (int i=0; i<n1; i++)
        {
            s1.addFilter(int(work.tp[f].v), work.tp[f+1].v, work.tp[f+2].v);
            f += 3;
        }
        PNet::Stage &s2 = cnn.stages[1];
        for (int i=0; i<n2; i++)
        {
            s2.addFilter(int(work.tp[f].v), work.tp[f+1].v, work.tp[f+2].v);
            f += 3;
        }
#endif // SINGLE_LS

        Mat fils = cnn.filterVis();
        namedWindow("krn",0);
        imshow("krn",fils);
        waitKey(10);
        return wi;
    }
    
    Mat distance_mat(const Mat &a, const Mat &b) const
    {
        Mat d = a-b; multiply(d,d,d,1,CV_32F); cv::sqrt(d,d);
        return d;
    }

    bool same(const Mat &a, const Mat &b) const
    {
        Mat res;
        try {
            svm->predict(distance_mat(a,b), res);
        } catch(...) {}
        int r = res.at<int>(0);
        return  r > 0;
    }

    int process(const  Mat & image, Mat &res)
    {
        Rect crop(80,80,90,90);
        Mat gray; 
        if (image.type() != CV_8U)
            cvtColor(image, gray, COLOR_BGR2GRAY);
        else gray=image;
        Mat i2 = gray(crop);

        
        return cnn.extract(i2, res);;
    }

    void addTrain(const  Mat &a, const  Mat &b, bool same)
    {
        int fbytes;
        Mat pa,pb;
        fbytes = process(a,pa);
        fbytes = process(b,pb);
        Mat features = distance_mat(pa,pb);
        feat.push_back(features);
        labl.push_back(same?1:-1);
        cerr << labl.rows << " " << same << " " << fbytes << '\r';
    }

    void train()
    {
        load();
        svm->clear();
        svm->train(feat, ml::ROW_SAMPLE, labl);
        feat.release();
        labl.release();
    }


    bool predict(const Mat &a, const Mat &b)
    {
        Mat pa,pb;
        process(a,pa);
        process(b,pb);
        return same(pa,pb);
    }
    unsigned int load()
    {
        static int off=0;
        for (unsigned int i=off; i<dataset->getTrain().size(); i+=skip)
        {
            cv::datasets::FR_lfwObj *example = static_cast<cv::datasets::FR_lfwObj *>(dataset->getTrain()[i].get());

            Mat img1 = imread(path+example->image1, IMREAD_GRAYSCALE);
            Mat img2 = imread(path+example->image2, IMREAD_GRAYSCALE);
            addTrain(img1, img2, example->same);
        }
        off++;
        off %= dataset->getTrain().size();
        return dataset->getNumSplits();
    }

    double test()
    {
        int n = 0;
        int numSplits = dataset->getNumSplits();
        static int off=0, offt=0;
        accuracy = 0.0;
        for (int j=offt; j<numSplits; j+=skipTest)
        {
            int nt = 0;
            int64 t0 = getTickCount(); 
            unsigned int incorrect = 0, correct = {0};
            vector < Ptr<cv::datasets::Object> > &curr = dataset->getTest(j);
            for (unsigned int i=off; i<curr.size(); i+=skip/2)
            {
                cv::datasets::FR_lfwObj *example = static_cast<cv::datasets::FR_lfwObj *>(curr[i].get());

                Mat img1 = imread(path+example->image1, IMREAD_GRAYSCALE);
                Mat img2 = imread(path+example->image2, IMREAD_GRAYSCALE);
                bool same = predict(img1,img2);
                if (same == example->same)
                    correct++;
                else
                    incorrect++;
                nt ++;
            }
            off++;
            off %= 17;
            double acc = double(correct)/(nt);
            accuracy += acc;
            int64 t1 = getTickCount(); 
            printf("%4u %3d/%-3d  %3.4f %3.4f                      \r", j,correct,incorrect,acc, ((t1-t0)/getTickFrequency()));
            n ++;

            if ( acc <= aworst ) break;
        }
        offt++;
        offt %= 3;
        accuracy /= n;
        work.accuracy = accuracy;
        bool ok = work.accuracy > tp[wi].accuracy;
        cerr << format("%2d %c %s",wi,tm,(ok?"* ":"- ")) << work.str() << "\t(" << aworst << " " << abest << ")" << endl;
        if (ok)
            tp[wi] = work;
        return tp[wi].accuracy;
    }
};


int main()
{
    theRNG().state = getTickCount();
    Training tr("lfw-deepfunneled/");
    while ( 1 )
    {
        int k = waitKey(10);
        if (k == 27) break;
        if (k == ' ') tr.dump();
        tr.setWorst();
        tr.train();
        tr.test();
    }

    return 0;
}
