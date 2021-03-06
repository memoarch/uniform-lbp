
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/datasets/fr_lfw.hpp"
#include <vector>
using std::vector;


#define HAVE_DLIB
#ifdef HAVE_DLIB
 #include "opencv2/core/core_c.h"
 #include <dlib/image_processing.h>
 #include <dlib/opencv/cv_image.h>

struct LandMarks
{
    dlib::shape_predictor sp;
    int offset;

    LandMarks(int off=10)
        : offset(off)
    {
        dlib::deserialize("data/shape_predictor_68_face_landmarks.dat") >> sp;
    }

    inline int crop(int v) const {return (v<offset?offset:(v>90-offset?90-offset:v)); }
    void extract(const cv::Mat &img, vector<cv::Point> &kp) const
    {
        dlib::rectangle rec(0,0,img.cols,img.rows);
        dlib::full_object_detection shape = sp(dlib::cv_image<uchar>(img), rec);

        int idx[] = {17,26, 19,24, 21,22, 36,45, 39,42, 38,43, 31,35, 51,33, 48,54, 57,27, 0};
        //int idx[] = {18,25, 20,24, 21,22, 27,29, 31,35, 38,43, 51, 0};
        for(int k=0; (k<40) && (idx[k]>0); k++)
            kp.push_back(cv::Point(crop(shape.part(idx[k]).x()), crop(shape.part(idx[k]).y())));
        //dlib::point p1 = shape.part(31) + (shape.part(39) - shape.part(31)) * 0.5; // left of nose
        //dlib::point p2 = shape.part(35) + (shape.part(42) - shape.part(35)) * 0.5;
        //dlib::point p3 = shape.part(36) + (shape.part(39) - shape.part(36)) * 0.5; // left eye center
        //dlib::point p4 = shape.part(42) + (shape.part(45) - shape.part(42)) * 0.5; // right eye center
        //dlib::point p5 = shape.part(31) + (shape.part(48) - shape.part(31)) * 0.5; // betw.mouth&nose
        //dlib::point p6 = shape.part(35) + (shape.part(54) - shape.part(35)) * 0.5; // betw.mouth&nose
        //kp.push_back(cv::Point(p1.x(), p1.y()));
        //kp.push_back(cv::Point(p2.x(), p2.y()));
        //kp.push_back(cv::Point(p3.x(), p3.y()));
        //kp.push_back(cv::Point(p4.x(), p4.y()));
        //kp.push_back(cv::Point(p5.x(), p5.y()));
        //kp.push_back(cv::Point(p6.x(), p6.y()));
    }
};
#else // fixed manmade landmarks
struct LandMarks
{
    void extract(const Mat &img, vector<Point> &kp, float scale=1.0f)
    {
        //// now, try with less kp, and larger patches:
        //kp.push_back(KeyPoint(24,33,1));    kp.push_back(KeyPoint(66,33,1));
        //kp.push_back(KeyPoint(27,66,1));    kp.push_back(KeyPoint(63,66,1));

        kp.push_back(Point(15,19));    kp.push_back(Point(75,19));
        kp.push_back(Point(29,20));    kp.push_back(Point(61,20));
        kp.push_back(Point(36,24));    kp.push_back(Point(54,24));
        kp.push_back(Point(38,35));    kp.push_back(Point(52,35));
        kp.push_back(Point(30,39));    kp.push_back(Point(60,39));
        kp.push_back(Point(19,39));    kp.push_back(Point(71,39));
        kp.push_back(Point(10,38));    kp.push_back(Point(79,38));
        kp.push_back(Point(40,64));    kp.push_back(Point(50,64));
        kp.push_back(Point(31,75));    kp.push_back(Point(59,75));
        kp.push_back(Point(32,49));    kp.push_back(Point(59,49));
    }
};
#endif


using namespace cv;
#include "../profile.h"



struct Training
{
    enum 
    {
        kHalfSSdSize   = 5,
        kHalfPatchSize = 12,
        kNumPatches    = 32,
        kNumKeypoints  = 20,
        kFeatureBytes  = 256,
        kBestLatches   = kFeatureBytes * 8,
        kPicsPerGen    = 4400 / 2,
    };

    Ptr<cv::datasets::FR_lfw> dataset;
    LandMarks land;

    struct Latch    
    {
        int p[6];
        int pos,neg;

        Latch() : pos(0), neg(0) {}

        float score() const
        { 
            return float(pos + neg) / (Training::kPicsPerGen);
        }
        String str() const
        { 
            return format("[%d %d][%d %d %d %d %d %d]",pos,neg,p[0],p[1],p[2],p[3],p[4],p[5]); 
        }
        void reset()
        {
            pos = neg = 0;
        }
        void randomize(int hps)
        {
            reset();
            for (int k=0; k<6; k++)
            {
                p[k] = theRNG().uniform(-hps, hps);
            }
        }
        double corr(const Latch &ol)
        {
            PROFILE;
            double d = 0.0;
            for (int i=0; i<6; i++)
            {
                d += (p[i] - ol.p[i])*(p[i] - ol.p[i]);
            }
            return sqrt(d);
        }
    };
    vector<Latch> latches[kNumKeypoints], keep[kNumKeypoints];

    Training(const String &path)
        : dataset(cv::datasets::FR_lfw::create())
        , land(kHalfPatchSize)
    {
        theRNG().state = getTickCount();
        dataset->load(path);
        mutate();
    }

    bool calculateSums(const Latch &latch, const cv::Mat &grayImage, const Point2f &pt, int half_ssd_size)
    {
        PROFILEX("Train::calculateSums");
	    int ax = latch.p[0] + (int)(pt.x + 0.5);
	    int ay = latch.p[1] + (int)(pt.y + 0.5);

	    int bx = latch.p[2] + (int)(pt.x + 0.5);
	    int by = latch.p[3] + (int)(pt.y + 0.5);

	    int cx = latch.p[4] + (int)(pt.x + 0.5);
	    int cy = latch.p[5] + (int)(pt.y + 0.5);

	    int suma=0, sumc=0;
        int K = half_ssd_size;
        for (int iy = -K; iy <= K; iy++)
	    {
		    const uchar * Mi_a = grayImage.ptr<uchar>(ay + iy);
		    const uchar * Mi_b = grayImage.ptr<uchar>(by + iy);
		    const uchar * Mi_c = grayImage.ptr<uchar>(cy + iy);

		    for (int ix = -K; ix <= K; ix++)
		    {
			    double difa = Mi_a[ax + ix] - Mi_b[bx + ix];
			    suma += (int)((difa)*(difa));

			    double difc = Mi_c[cx + ix] - Mi_b[bx + ix];
			    sumc += (int)((difc)*(difc));
		    }
	    }
        return suma<sumc;
    }

    int pixelTest(const cv::Mat &imgA, const cv::Mat &imgB, const Point2f &pta, const Point2f &ptb, vector<Latch> &latch, int half_ssd_size, bool same)
    {
        PROFILEX("Train::pixelTest");
        int hits = 0;
        for (size_t i=0; i<latch.size(); i++)
	    {
		    bool bit1 = calculateSums(latch[i], imgA, pta, half_ssd_size);
		    bool bit2 = calculateSums(latch[i], imgB, ptb, half_ssd_size);
            bool ok = false;
            if (same)
                ok = (bit1==bit2), latch[i].pos += ok;
            else 
                ok = (bit1!=bit2), latch[i].neg += ok;
            hits += ok;
	    }
        return hits;
    }


    void gen(const String &path)
    {
        PROFILE;
        size_t cnt = dataset->getTrain().size();
        for (unsigned int i=0; i<cnt; i+=1)
        //for (unsigned int i=0; i<kPicsPerGen; i++)
        {
            size_t pic = i; //theRNG().uniform(0,cnt);
            cv::datasets::FR_lfwObj *example = static_cast<cv::datasets::FR_lfwObj *>(dataset->getTrain()[pic].get());

            Mat img1 = imread(path+example->image1, IMREAD_GRAYSCALE);
            Mat img2 = imread(path+example->image2, IMREAD_GRAYSCALE);
            Mat r1 = img1(Rect(80,80,90,90));
            Mat r2 = img2(Rect(80,80,90,90));
            vector<Point> kp1,kp2;
            land.extract(r1,kp1);
            land.extract(r1,kp2);
            int ok = 0;
            for (int j=0; j<kp1.size(); j++)
            {
                ok += pixelTest(r1, r2, kp1[j], kp2[j], latches[j], kHalfSSdSize, example->same);
            }
            cerr << i << " " << float(ok)/(kNumKeypoints*latches[0].size()) << "\r";
        }
    }

    void mutate()
    {
        PROFILE;
        for (int i=0; i<kNumKeypoints; i++)
        {
            latches[i].clear();
        }
        for (int g=0; g<kNumPatches; g++)
        for (int i=0; i<kNumKeypoints; i++)
        {
            vector<Point> pool;
            for (int y=-kHalfPatchSize; y<kHalfPatchSize; ++y)
            for (int x=-kHalfPatchSize; x<kHalfPatchSize; ++x)
                pool.push_back(Point(y,x));
            while (pool.size() >= 3)
            {
                // eat away 3 points from the pool:
                //   this avoids using the same point twice in a latch, 
                //   also keeps latches at a minimal distance of 3
                Latch la;
                for (int j=0; j<6; j+=2)
                {
                    size_t pid = theRNG().uniform(0,pool.size());
                    la.p[j]    = pool[pid].x;
                    la.p[j+1]  = pool[pid].y;
                    pool.erase(pool.begin() + pid);
                }
                latches[i].push_back(la);
            }
        }
        cerr << "latches " << latches[0].size() << endl;
        cerr << "keep    " << keep[0].size() << endl;
    }

    struct sort_latch
    {
        bool operator ()(const Latch &a,const Latch &b)
        {
            return a.score() > b.score();
        }
    };

    void prune()
    {
        PROFILE;
        //const double minD = 2;
        //int corrs = 0;
        for (size_t i=0; i<kNumKeypoints; i++)
        {
            std::sort(latches[i].begin(), latches[i].end(), sort_latch());
            keep[i].insert(keep[i].begin(),latches[i].begin(), latches[i].end());
            //for (size_t j=0; j<latches[i].size(); j++)
            //{
                //Latch &latch = latches[i][j];
                //bool too_correlated = false;
                //for (size_t k=0; k<keep[i].size(); k++) 
                //{
                //    Latch &other = keep[i][k];
                //    double d = other.corr(latch);
                //    //if ((d < minD) && (other.score()>latch.score())
                //    if (d < minD)
                //    {
                //        too_correlated = true;
                //        corrs ++;
                //        break;
                //    }
                //}
                //if (! too_correlated)
            //        keep[i].push_back(latches[i][j]);
            //}
            std::sort(keep[i].begin(), keep[i].end(), sort_latch());
            if (keep[i].size() > kBestLatches)
                keep[i].resize(kBestLatches);
        }
        //cerr << "corrs " << float(corrs)/(latches[0].size()*kNumKeypoints) << endl;
    }

    void dump()
    {
        PROFILE;
        static float last_mea = 0.0f;
        float mea = 0.0f;
        for (int i=0; i<kNumKeypoints; i++)
        {
            float score = 0.0f;
            for (int j=0; j<keep[i].size(); j++)
            {
                score += keep[i][j].score();
            }
            score/=keep[i].size();
            mea += score;
            cerr << format("%1.4f",score) << keep[i][0].str() << endl;
        }
        cerr << mea/kNumKeypoints << endl;
        cerr << (mea - last_mea)/kNumKeypoints << endl;
        last_mea = mea;
    }

    bool isTrained()
    {
        for (int i=0; i<kNumKeypoints; i++)
        {
            if(keep[i].size() < kBestLatches)
                return false;
        }
        return true;
    }

    void save(String fn, int genr=0)  
    {
        PROFILE;
        FileStorage fs(fn, FileStorage::WRITE);
        fs << "gen" << genr;
        fs << "patch" << kHalfPatchSize;
        fs << "bytes" << kFeatureBytes;
        fs << "ssd" << kHalfSSdSize;
        fs << "latches" << "[";
        for (int i=0; i<kNumKeypoints; i++)
        {
            Mat_<int> m;
            for (size_t j=0; j<kBestLatches; j++)
            {
                for (int k=0; k<6; k++)
                {
                    m.push_back(keep[i][j].p[k]);
                }
            }
            fs << "{:";
            fs << "pt" << m;
            fs << "}";
        }
        fs << "]";
        fs.release();
    }
};


int main()
{
    String path("data/lfw-deepfunneled/");
    Training trainer(path);
    for (int i=0; i<100; i++)
    {
        cerr << "--------------------------------" << endl;
        cerr << "gen " << i << endl;
        cerr << "--------------------------------" << endl;
        trainer.gen(path);
        trainer.prune();
        trainer.dump();
        if (trainer.isTrained())
            trainer.save("data/latch.xml.gz", i);
        trainer.mutate();
    }
    return 0;
}
