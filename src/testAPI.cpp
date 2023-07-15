#include <iostream>
#include "AliasAPI.h"
#include <map>
#include <algorithm>
using namespace std;

void save_sku(AliasAPI &ap)
{
    queue<string> q;
    for(int i=0; i<ap.vsku_.size(); ++i){
        q.push(ap.vsku_[i]);
    }

    int num = 0;
    int N = 10;

    vector<string> vslug(N);
    vector<string> vkey(N);
    vector<int> vb(N);
    vector<thread> vthread;

    ofstream ofs("/home/zhwang/WZH/SneakerAPI-Cpp/data/sku.txt", ios::app);


    while(!q.empty())
    {
        cout << "---------------- start -----------------------\n";
        vthread.clear();
        int num = 0;
        while(num < N)
        {
            if(q.empty()) break;
            string keyword = q.front();
            q.pop();
            if(ap.products_slug_.count(keyword))
            {
                cout << "Got!" << endl;
                continue;
            }
            vkey[num] = keyword;
            vthread.push_back(thread(&AliasAPI::search_by_sku, &ap, std::ref(keyword), 0, std::ref(vslug[num]), std::ref(vb[num])));
            num++;
        }
        for(int i=0; i<vthread.size(); ++i)
            vthread[i].join();
        
        for(int i=0; i<vthread.size();++i)
        {
            cout << vslug[i] << endl;
            if(vb[i]){
                ap.products_slug_[vkey[i]] = vslug[i];
                // cout << i << " " << vkey[i] << " " << vslug[i] << endl; 
                ofs << vkey[i] << " " << vslug[i] << endl;
            }
            else
            {
                // cout << i << "failed\n";
                q.push(vkey[i]);
            }
        }
    }

    ofs.close();
}

int main()
{
    AliasAPI ap("/home/zhwang/WZH/SneakerAPI-Cpp/data/config.yaml");
    ap.autoUpList();
    // ap.loadSlug();
    // save_sku(ap);
    // ap.save_sku();
    // ap.get_product_detail("tinman-elite-x-adizero-avanti-tyo-white-gold-metallic-gw1385");
    
    return 0;
}