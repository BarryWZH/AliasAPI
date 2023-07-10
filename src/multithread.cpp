#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
using namespace std;

mutex getdatamtx;

bool endflag = false;
vector<int> v;
vector<int> getData(vector<int>& v)
{
    if(v.size() < 10){
        endflag = true;
        return v;
    }
    vector<int> res;
    getdatamtx.lock(); 
    res.assign(v.begin(), v.begin() + 10);
    v.erase(v.begin(), v.begin()+10);
    getdatamtx.unlock();
    return res;
}

void fetchandprint(int threadId)
{
    while (!endflag)
    {
        vector<int> res = getData(v);
        cout << "Thread " << threadId << endl;
        for (auto it : res)
            cout << it << " ";
        cout << endl;
    }
}

int main()
{
    for(int i=0; i<100; ++i)
        v.push_back(i);
    
    thread thread1(fetchandprint, 1);
    thread thread2(fetchandprint, 2);

    thread1.join();
    thread2.join();

    return 0;
}