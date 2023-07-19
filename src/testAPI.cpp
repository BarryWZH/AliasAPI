#include <iostream>
#include "AliasAPI.h"
#include <map>
#include <algorithm>
using namespace std;

int N = 10;

struct Data
{
    string sku_;
    string slug_;
    int numsize_;
    vector<float> vsize_;
};

void splitData()
{
    ifstream ifs("/home/zhwang/WZH/SneakerAPI-Cpp/data/data.txt");
    string sku;
    float size, price;
    int num, flag;
    vector<string> vsku;
    while (!ifs.eof())
    {
        ifs >> sku >> size >> num >> price >> flag;
        vsku.push_back(sku);
    }
    int patchsize = (vsku.size() / N)+1;
    int pn = 0;
    // for(int i=0; i<N; ++i)
    // {
    //     char filepath[100];
    //     sprintf(filepath, "/home/zhwang/WZH/SneakerAPI-Cpp/data/multi/%d.txt", i+1);
    //     ofstream ofs(filepath);
    //     for(int t=i*patchsize; t<(i+1)*patchsize; ++t){
    //         ofs << vsku[t];
    //         if(t!=(i+1)*patchsize-1) ofs << endl;
    //     }
    //     ofs.close();
    // }
    for(int i=0; i<vsku.size();++i)
    {
        char filepath[100];
        int n = i%N;
        sprintf(filepath, "/home/zhwang/WZH/SneakerAPI-Cpp/data/multi/%d.txt", n+1);
        ofstream ofs(filepath, ios::app);
        ofs << vsku[i];
        if(i < vsku.size()-N) ofs << endl;
        ofs.close();
    }
}

void mergeData()
{
    map<string, Data> mp;
    // 10个文件合并
    for(int i=1; i<=N;++i)
    {
        char filepath[100];
        sprintf(filepath, "/home/zhwang/WZH/SneakerAPI-Cpp/data/multi/slugsize%d.txt", i);
        ifstream ifs(filepath);
        if(!ifs.is_open()) continue;
        string sku, slug;
        int numsize;
        float size;
        while(!ifs.eof())
        {
            Data d;
            ifs >> sku >> slug >> numsize;
            if(ifs.eof())
            {
                ifs.close();
                break;
            }
            d.sku_ = sku;
            d.slug_ = slug;
            d.numsize_ = numsize;
            for(int i=0;i<numsize;++i)
            {
                ifs >> size;
                d.vsize_.push_back(size);
            }
            mp[sku] = d;
        }
        ifs.close();
    }

    // 原始文件
    ifstream ifs("/home/zhwang/WZH/SneakerAPI-Cpp/data/multi/merge.txt");
    string sku, slug;
    int numsize;
    float size;
    while (!ifs.eof())
    {
        Data d;
        ifs >> sku >> slug >> numsize;
        if(ifs.eof())
        {
            ifs.close();
            break;
        }
        d.sku_ = sku;
        d.slug_ = slug;
        d.numsize_ = numsize;
        for (int i = 0; i < numsize; ++i)
        {
            ifs >> size;
            d.vsize_.push_back(size);
        }
        mp[sku] = d;
        ifs.get();
    }
    ifs.close();

    ofstream ofs("/home/zhwang/WZH/SneakerAPI-Cpp/data/multi/merge.txt");
    int nn = 0;
    for(auto it:mp)
    {
        nn++;
        ofs << it.second.sku_ << " " << it.second.slug_ << " " << it.second.numsize_;
        for(int i=0; i<it.second.numsize_;++i){
            ofs << " " << it.second.vsize_[i];
        }
        if(nn != mp.size()) ofs << endl;
    }
    ofs.close();
    cout << "Merge complete!" << endl;
}

int main(int argc, char** argv)
{
    // splitData();
    if(argc != 3)
    {
        cout << "testAPI ${skufilepath} ${slugsizefilepath}\n";
        exit(1);
    }

    mergeData();

    AliasAPI ap("/home/zhwang/WZH/SneakerAPI-Cpp/data/config.yaml");
    // ap.autoUpList();
    string input = argv[1], output = argv[2];
    ap.save_sku(input, output);

    return 0;
}