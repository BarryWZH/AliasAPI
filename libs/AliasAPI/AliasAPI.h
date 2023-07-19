#ifndef _HEADER_ALIAS_API_H_
#define _HEADER_ALIAS_API_H_
#include <iostream>
#include <opencv2/opencv.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <xlnt/xlnt.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <algorithm>
#include <chrono>
#include <set>
#include <iomanip>
using namespace std;
using json = nlohmann::json;

class AliasAPI
{
public:

    AliasAPI(string configfile);

    ~AliasAPI();

    void autoUpList();

public:

    void readConfig(string configfilepath);

    void getData();

    void loadSlug();

    void initializeCurl();

    // 获取auth_token, access_token
    void getToken();

public:

    // API interface

    string ping();

    bool init_auth();

    bool login();

    void search_by_sku(string keyword, int page, string& slug, vector<float>& vsize, int& found);

    void get_product_detail(string slug, vector<float>& size, int& found);

    // void save_sku(string filename);

    void save_sku();

    void save_sku(string& inputfile, string& outputfile);

    void listing_product_multi(string params);

public:

    // function 

    void WriteResult();

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

    static int DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr);

    void processJson(const string& jsonStr);

public:
    cv::FileStorage fs_;
    chrono::steady_clock::time_point start_, end_;
    

    // 数据相关
    string datapath_;
    string slugpath_;
    string respath_;

    vector<string> vsku_;
    vector<float> vsize_;
    vector<int> vnum_;
    vector<float> vprice_;
    // 0: unprocessed 1: processed 2: invalid size 3: unfound
    vector<int> vuplist_;

    // url,token信息
    string token_;
    string base_url_;
    map<string, string> function_url_;
    string account_;
    string password_;

    // 调试输出信息
    bool use_debug_;
    bool use_verbose_;
    bool initializewithsavesku_;

    // curl
    CURL* curl_;
    string response_;
    struct curl_slist* headers_;

    bool init_flag_;         // 状态位

    string auth_;            // auth_
    string access_token_;    // access_token_;
    string refresh_token_;   // refresh_token

    // product_slug_id: <sku, slug>
    map<string, string> products_slug_;
    map<string, string> slug_products_;
    map<string, int> products_id_;
    map<string, set<float>> slug_size_;
    queue<pair<string, float>> processed_;    // 已经上架
    queue<pair<string, float>> unprocessed_;  // 未上架
    queue<pair<string, float>> invalid_;      // 尺码不存在的
    queue<pair<string, float>> unfound_;      // 没有找到的
    int total_num_;           // 总共要上架的数量
    
    mutex mtxslug_; // 处理product_slug

    // list
    mutex mtxsearch_;
    int numsearchthread_;
    int patchsearchsize_;

    int patchuplistsize_;

};

#endif