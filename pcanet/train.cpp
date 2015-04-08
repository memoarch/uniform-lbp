#include <opencv2\opencv.hpp>
#include "PCANet.h"

#include <iostream>
#include <fstream>
#include <string>
using std::cout;
using std::endl;


int main(int argc, char** argv)
{
    const int DIR_NUM = 40; // 40;

    cv::String dir = "lfw3d_9000";
    std::vector<cv::String> files;
    glob(dir,files,true);

    std::vector<cv::Mat> InImgs;
    for (size_t i=0; i<4000; ++i)
    {
        size_t z = cv::theRNG().uniform(0,files.size());
        cv::Mat im = cv::imread(files[z],0);
        im.convertTo(im,CV_32F);
        InImgs.push_back(im);
    }

    std::vector<int> NumFilters;
    NumFilters.push_back(4);
    NumFilters.push_back(4);
    std::vector<int> blockSize;
    blockSize.push_back(28);
    blockSize.push_back(23);

    PCANet pcaNet = {
        0,
        2,
        11,
        NumFilters,
        blockSize,
        0
    };
    cv::String sets = pcaNet.settings();
    cout << "PCANet " << sets << endl;
    int64 e1 = cv::getTickCount();
    cv::Mat features = pcaNet.trainPCA(InImgs, true);
    int64 e2 = cv::getTickCount();
    double time = (e2 - e1) / cv::getTickFrequency();
    cout << " PCANet Training time: " << time << endl;

    pcaNet.save("pcanet.xml");
    cerr << features.size() << " " << features.type() << endl;
    return 0;
}
