#include "AliasAPI.h"
using namespace std;

#define DEBUG 0

int success = 0;
int unsuccess = 0;
int fail = 0;

AliasAPI::AliasAPI(string configfilepath){

    // 读取配置文件数据
    readConfig(configfilepath);

    // 初始化curl
    initializeCurl();

    // 设置状态位
    init_flag_ = false;

    // 初始化获取auth_token、access_token
    getToken();

    // 获取表格数据
    getData();
}

AliasAPI::~AliasAPI(){
    fs_.release();
    curl_slist_free_all(headers_);
}

void AliasAPI::readConfig(string configfilepath)
{
    fs_.open(configfilepath, cv::FileStorage::READ);

    // 读入参数
    fs_["token"] >> token_;
    fs_["base_url"] >> base_url_;

    string str;
    fs_["ping_url"] >> str;
    function_url_["ping_url"] = str;
    fs_["query_by_sku_url"] >> str;
    function_url_["query_by_sku_url"] = str;
    fs_["query_by_condition_url"] >> str;
    function_url_["query_by_condition_url"] = str;
    fs_["init_auth_url"] >> str;
    function_url_["init_auth_url"] = str;
    fs_["login_url"] >> str;
    function_url_["login_url"] = str;
    fs_["self_info_url"] >> str;
    function_url_["self_info_url"] = str;
    fs_["seller_info_url"] >> str;
    function_url_["seller_info_url"] = str;
    fs_["update_access_token_url"] >> str;
    function_url_["update_access_token_url"] = str;
    fs_["get_product_detail_url"] >> str;
    function_url_["get_product_detail_url"] = str;
    fs_["get_recent_orders_url"] >> str;
    function_url_["get_recent_orders_url"] = str;
    fs_["lowest_price_url"] >> str;
    function_url_["lowest_price_url"] = str;
    fs_["get_selling_info_url"] >> str;
    function_url_["get_selling_info_url"] = str;
    fs_["get_sold_info_url"] >> str;
    function_url_["get_sold_info_url"] = str;
    fs_["get_selling_products_url"] >> str;
    function_url_["get_selling_products_url"] = str;
    fs_["get_sold_products_url"] >> str;
    function_url_["get_sold_products_url"] = str;
    fs_["get_history_products_url"] >> str;
    function_url_["get_history_products_url"] = str;
    fs_["get_sold_products_detail_url"] >> str;
    function_url_["get_sold_products_detail_url"] = str;
    fs_["enable_vacation_mode_url"] >> str;
    function_url_["enable_vacation_mode_url"] = str;
    fs_["cancel_product_url"] >> str;
    function_url_["cancel_product_url"] = str;
    fs_["update_price_single_url"] >> str;
    function_url_["update_price_single_url"] = str;
    fs_["listing_product_single_step1_url"] >> str;
    function_url_["listing_product_single_step1_url"] = str;
    fs_["listing_product_single_step2_url"] >> str;
    function_url_["listing_product_single_step2_url"] = str;
    fs_["listing_product_multi_url"] >> str;
    function_url_["listing_product_multi_url"] = str;

    fs_["use_dubug"] >> use_debug_;
    fs_["use_verbose"] >> use_verbose_;

    fs_["account"] >> account_;
    fs_["password"] >> password_;

    // 读入+解析数据
    fs_["datapath"] >> datapath_;
    fs_["slugpath"] >> slugpath_;
    fs_["respath"] >> respath_;

    fs_["patchsearchsize"] >> patchsearchsize_;
    fs_["patchuplistsize"] >> patchuplistsize_;

    fs_["initializewithsavesku"] >> initializewithsavesku_;
}

void AliasAPI::getData(){

    ifstream ifs(datapath_);

    if(!ifs.is_open())
    {
        cerr << "Error Path!\n";
        exit(1);
    }

    string sku;
    float size, price;
    int num, flag;

    // 加载上架数据
    int n=0;
    while(!ifs.eof())
    {
        ifs >> sku >> size >> num >> price >> flag;
        // sku
        replace(sku.begin(), sku.end(), ' ', '-');
        vsku_.push_back(sku);
        // size
        vsize_.push_back(size);
        // num
        vnum_.push_back(num);
        // price * 100
        vprice_.push_back(price * 100);
        // uplist flag
        vuplist_.push_back(flag);
        // sku2id
        products_id_[sku] = n++;
    }

    // 加载本地存储的slug信息和对应尺码信息
    loadSlug();

    // 更新slug信息和对应的尺码信息
    if(initializewithsavesku_)
        save_sku();

    // 处理一些状态量
    total_num_ = 0;

    string keyword, slug;
    for(int i=0; i< vsku_.size(); ++i){
        for(int j=0;j<vnum_[i]; ++j)
        {
            total_num_++;
            // 没有找到这一款则添加到未找到队列
            if(!products_slug_.count(vsku_[i]))
            {
                vuplist_[i] = 3;
                unfound_.push({vsku_[i], vsize_[i]});
                continue;
            }

            // 存在该尺码则添加到代处理队列
            if(slug_size_[products_slug_[vsku_[i]]].count(vsize_[i])){
                unprocessed_.push({vsku_[i], vsize_[i]});
            }
            // 不存在该尺码则放入不合理队列
            else{
                invalid_.push({vsku_[i], vsize_[i]});
                vuplist_[i] = 2;
            }
        }
    }

    cout << "TotalNum UpList:        " << total_num_ << endl;
    cout << "TotalNum NeedProcessed: " << unprocessed_.size() << endl;
    cout << "TotalNum InValid:       " << invalid_.size() << endl;
    cout << "TotalNum UnFound:       " << unfound_.size() << endl;
 
    // 搜索线程设置为0；
    numsearchthread_ = 0;
}

void AliasAPI::loadSlug()
{
    ifstream ifs(slugpath_);
    
    if(!ifs.is_open())
    {
        ifs.close();
        return;
    }

    string sku, slug;
    int n;
    float size;
    int num=0;
    while(!ifs.eof())
    {
        ifs >> sku >> slug >> n;
        products_slug_[sku] = slug;
        slug_products_[slug] = sku;
        for(int i=0;i<n;++i)
        {
            ifs >> size;
            slug_size_[slug].insert(size);
        }
        num++;
    }

    ifs.close();

    cout << "Load " << num << " Slug!\n";
}

void AliasAPI::initializeCurl()
{
    // 初始化curl
    curl_ = curl_easy_init();
    if (curl_)
        cout << "Init curl successful!\n";
    else
        cout << "Init curl failed!\n";
    
    resetCurl();
}

void AliasAPI::resetCurl()
{
    // 清除状态
    curl_easy_reset(curl_);

    // 设置请求头
    headers_ = NULL;
    // headers = curl_slist_append(headers, "accept: application/json");
    headers_ = curl_slist_append(headers_, "Content-Type: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);

    // 设置回写函数
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, AliasAPI::WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_);

    // 设置输出信息
    if (use_verbose_)
        curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);

    // 设置debug
    if (use_debug_)
        curl_easy_setopt(curl_, CURLOPT_DEBUGFUNCTION, AliasAPI::DebugCallback);
}

void AliasAPI::getToken(){

    resetCurl();

    if(!init_auth())
    {
        cout << "init auth failed\n";
        return;
    }
    if(!login())
    {
        cout << "Login failed\n";
        return;
    }
    start_ = chrono::steady_clock::now();
    init_flag_ = true;
    cout << "Get token successful!" << endl;
}
// --------------------- API ----------------------------

string AliasAPI::ping(){
    return "";
}

bool AliasAPI::init_auth()
{
    response_.clear();
    string requesturl = base_url_ + function_url_["init_auth_url"] + "?token=" + token_;
    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    // cout << "Request: " << requesturl << endl;

    CURLcode res = curl_easy_perform(curl_);
    // cout << response_ << endl;
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        if(jsonData.contains("data")){
            auth_ = jsonData["data"]["auth"];
            // cout << auth_ << endl;
            return true;
        }
        else
            return false;
    }
    
    return false;
}

bool AliasAPI::login()
{   
    response_.clear();
    // baseurl + token + auth + account + password
    string requesturl = base_url_ + function_url_["login_url"] + "?token=" + token_ 
    + "&auth=" + auth_ + "&account=" + account_ + "&password=" + password_;

    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());
    // cout << "request: " << requesturl << endl;

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        if(jsonData.contains("data")){
            access_token_ = jsonData["data"]["auth_token"]["access_token"];
            refresh_token_ = jsonData["data"]["auth_token"]["refresh_token"];
            return true;
        }
        else
            return false;
    }
    
    return false;
}

void AliasAPI::search_by_sku(string keyword, int page, string& slug, vector<float>& vsize, int& found)
{
    response_.clear();
    string requesturl = base_url_ + function_url_["query_by_sku_url"] + "?token=" + token_
    + "&keyword=" + keyword + "&page=" + to_string(page);

    // cout << requesturl << endl;
    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());
    // curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &temp);

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        // 查找到
        if(jsonData["data"]["hits"].size() > 0){
            // products_slug_[keyword] = jsonData["data"]["hits"][0]["slug"];
            // cout << products_slug_[keyword] << endl;
            auto temp = jsonData["data"]["hits"][0];
            int size = temp["size_range"].size();
            for(int i=0; i<size; ++i)
            {
                float s = temp["size_range"][i];
                if(s==0) continue;
                vsize.push_back(s);
            }
            slug = temp["slug"];
            found = 1;
        }
        else
            found = 0;
    }
}

void AliasAPI::get_product_detail(string slug, vector<float>& vsize, int& found)
{
    response_.clear();
    vsize.clear();
    string requesturl = base_url_ + function_url_["get_product_detail_url"] + "?token=" + token_
    + "&auth=" + auth_ + "&access_token=" + access_token_ + "&slug=" + slug;

    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        // cout << jsonData << endl;
        // 查找到
        if(jsonData["data"].contains("product")){
            auto temp = jsonData["data"]["product"]["size_options"];
            int size = temp.size();
            for(int i=0; i<size; ++i)
            {
                if(temp[i].contains("value"))
                {
                    float value = temp[i]["value"];
                    // slug_size_[slug].insert(value);
                    vsize.push_back(value);
                }
            }
            found = 1;
        }
        else
            found = 0;
    }
    
}

void AliasAPI::save_sku()
{
    vector<string> vsku;
    string str;

    ofstream ofs(slugpath_, ios::app);
    int isfound = 0;
    queue<string> q;
    for(int i=0; i<vsku.size(); ++i){
        if(products_slug_.count(vsku[i])) continue;
        q.push(vsku[i]);
    }
    int num=0;
    string slug;
    while (!q.empty() && init_flag_)
    {
        end_ = chrono::steady_clock::now();
        double span = chrono::duration<double>(end_ - start_).count();
        cout << "Time cost: " << setw(7) << span << " s\t" << "Left: " << q.size() << endl;
        if (span > 3600)
        {
            init_flag_ = false;
            getToken();
            start_ = chrono::steady_clock::now();
        }
        string keyword = q.front();
        q.pop();
        cout << num++ << " " << keyword;
        if (products_slug_.count(keyword))
        {
            // found
            cout << " already save" << endl;
            continue;
        }
        vector<float> vsize;
        search_by_sku(keyword, 0, slug, vsize, isfound);
        if (isfound)
        {
            cout << " " << slug << " ";
            cout << vsize.size() << " ";
            products_slug_[keyword] = slug;
            slug_products_[slug] = keyword;
            for (int i = 0; i < vsize.size(); ++i)
            {
                slug_size_[slug].insert(vsize[i]);
                cout << vsize[i] << " ";
            }
            cout << endl;
            ofs << keyword << " " << slug << " ";
            ofs << vsize.size() << " ";
            for (int i = 0; i < vsize.size(); ++i)
                ofs << vsize[i] << " ";
            ofs << endl;
        }
        else
        {
            q.push(keyword);
            cout << endl;
        }
    }
    ofs.close();
}

void AliasAPI::save_sku(string& inputfile, string& outputfile)
{
    vector<string> vsku;
    string str;
    ifstream ifs(inputfile);
    while(!ifs.eof())
    {
        ifs >> str;
        vsku.push_back(str);
    }

    ofstream ofs(outputfile, ios::app);
    int isfound = 0;
    queue<string> q;
    for(int i=0; i<vsku.size(); ++i){
        if(products_slug_.count(vsku[i])) continue;
        q.push(vsku[i]);
    }
    int num=0;
    string slug;
    while (!q.empty() && init_flag_)
    {
        end_ = chrono::steady_clock::now();
        double span = chrono::duration<double>(end_ - start_).count();
        cout << "Time cost: " << setw(7) << span << " s\t" << "Left: " << q.size() << endl;
        if (span > 3600)
        {
            init_flag_ = false;
            getToken();
            start_ = chrono::steady_clock::now();
        }
        string keyword = q.front();
        q.pop();
        cout << num++ << " " << keyword;
        if (products_slug_.count(keyword))
        {
            // found
            cout << " already save" << endl;
            continue;
        }
        vector<float> vsize;
        search_by_sku(keyword, 0, slug, vsize, isfound);
        if (isfound)
        {
            cout << " " << slug << " ";
            cout << vsize.size() << " ";
            products_slug_[keyword] = slug;
            slug_products_[slug] = keyword;
            for (int i = 0; i < vsize.size(); ++i)
            {
                slug_size_[slug].insert(vsize[i]);
                cout << vsize[i] << " ";
            }
            cout << endl;
            ofs << keyword << " " << slug << " ";
            ofs << vsize.size() << " ";
            for (int i = 0; i < vsize.size(); ++i)
                ofs << vsize[i] << " ";
            ofs << endl;
        }
        else
        {
            q.push(keyword);
            cout << endl;
        }
    }
    ofs.close();
}

void AliasAPI::listing_product_multi(string params, vector<string>& vsku)
{
    response_.clear();
    string requesturl = base_url_ + function_url_["listing_product_multi_url"] + "?token=" + token_
    + "&auth=" + auth_ + "&access_token=" + access_token_;

    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, params.length());
    
    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    int successsize = 0;
    int failsize = 0;

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        // cout << response_ << endl;
        // 分别对succeeded和failed的做处理
        if(jsonData["data"].contains("succeeded"))
        {
            successsize = jsonData["data"]["succeeded"].size();
            success+=successsize;
            cout << "uplist: " << successsize << endl;
            if(successsize > 0)
            {
                string path = respath_ + "/successful.txt";
                ofstream ofs(path, ios::app);
                for(int i=0;i<successsize;++i)
                {
                    string sku = jsonData["data"]["succeeded"][i]["product"]["sku"];
                    ofs << sku << endl;
                    float s = jsonData["data"]["succeeded"][i]["size_option"]["value"];
                    replace(sku.begin(), sku.end(), ' ', '-');
                    processed_.push({sku, s});
                }
                ofs.close();
            }
        }
        else{
            string path = respath_ + "/unseccessful.txt";
            ofstream ofs(path, ios::app);
            for(auto it:vsku)
                ofs << it << endl;
            ofs.close();
            cout << "No successful!";
            unsuccess += vsku.size();
        }

        // add failure
        if(jsonData["data"].contains("failed"))
        {
            if(DEBUG)
            {
                ofstream ofs("/home/zhwang/WZH/SneakerAPI-Cpp/1.txt", ios::app);
                ofs << "输入" << endl;
                ofs << params << endl;
                ofs << "回包" << endl;
                ofs << jsonData << endl;
                ofs.close();
            }
            failsize = jsonData["data"]["failed"].size();
            fail += failsize;
            if(failsize > 0)
            {
                string path = respath_ + "/fail.txt";
                ofstream ofs(path, ios::app);
                auto temp = jsonData["data"]["failed"];
                cout << "failed: " << failsize << endl;
                for (int i = 0; i < failsize; ++i)
                {
                    string slug = temp[i]["product_id"];
                    float s = temp[i]["size_option"]["value"];
                    string sku = slug_products_[slug];
                    cout << sku << " " << s << endl;
                    ofs << sku << endl;
                    unprocessed_.push({sku, s});
                }
                ofs.close();
            }
        }
        else{
            // cout << "No fail\n";
        }
    }   
    else
    {
        cout << "res failed!\n";
    }
}

void AliasAPI::autoUpList()
{
    if(!init_flag_)
    {
        cout << "Unintialized system!" << endl;
        return;
    }

    int patch = 0;
    int t=0;

    json jsonData;
    json products;
    string slug;
    vector<string> patchsku;

    while(!unprocessed_.empty() && init_flag_)
    {
        // cout << "Unprocessed: " << unprocessed_.size() << endl;
        string keyword = unprocessed_.front().first;
        float size = unprocessed_.front().second;
        int id = products_id_[keyword];
        unprocessed_.pop();

        string slug = products_slug_[keyword];
        // cout << keyword << " " << slug << endl;
       
        json temp;
        temp["sku"] = keyword;
        temp["product_slug"] = slug;
        temp["price_cents"] = vprice_[id];
        temp["size_us"] = vsize_[id];

        products.push_back(temp);
        patchsku.push_back(keyword);
        patch++;

        if(patch >= patchuplistsize_){

            end_ = chrono::steady_clock::now();
            double span = chrono::duration<double>(end_ - start_).count();
            cout << "Time cost: " << setw(7) << span << " s\t" << "Left" << setw(7) << unprocessed_.size() << endl;
            if (span > 3500)
            {
                init_flag_ = false;
                getToken();
                start_ = chrono::steady_clock::now();
            }

            jsonData["products"] = products;
            // cout << jsonData.dump() << endl;
            auto t1 = chrono::steady_clock::now();
            listing_product_multi(jsonData.dump(), patchsku);
            auto t2 = chrono::steady_clock::now();
            cout << "multiuplist: " << chrono::duration<double>(t2 - t1).count() << " s" << endl;
            patch = 0;
            products.clear();
            jsonData.clear();
            patchsku.clear();
        }
    }

    cout << "Successful Uplist:      " << success << endl;
    cout << "UnSuccessful Uplist:    " << unsuccess << endl;
    cout << "Fail Uplist:            " << fail << endl;
    cout << "Processed queue size:   " << processed_.size() << endl;
    cout << "Unprocessed queue size: " << unprocessed_.size() << endl;

    cout << "TotalNum UpList:        " << total_num_ << endl;
    cout << "TotalNum InValid:       " << invalid_.size() << endl;
    cout << "TotalNum UnFound:       " << unfound_.size() << endl;

    // WriteResult();
}

// --------------------- function -----------------------

void AliasAPI::WriteResult()
{
    string processedpath = respath_ + "processed.txt";
    string unprocessedpath = respath_ + "unprocessed.txt";
    string invalidpath = respath_ + "invalid.txt";
    string unfoundpath = respath_ + "unfound.txt";

    // processed
    ofstream ofs(processedpath);
    for(int i=0; i<processed_.size(); ++i)
    {
        auto temp = processed_.front();
        processed_.pop();
        ofs << temp.first << " " << temp.second << endl;
    }
    ofs.close();

    // unprocessed
    ofs.open(unprocessedpath);
    for(int i=0; i<unprocessed_.size(); ++i)
    {
        auto temp = unprocessed_.front();
        unprocessed_.pop();
        ofs << temp.first << " " << temp.second << endl;
    }

    // invalid
    ofs.open(invalidpath);
    for(int i=0; i<invalid_.size(); ++i)
    {
        auto temp = invalid_.front();
        invalid_.pop();
        ofs << temp.first << " " << temp.second << endl;
    }

    // unfound
    ofs.open(unfoundpath);
    for(int i=0; i<unfound_.size(); ++i)
    {
        auto temp = unfound_.front();
        unfound_.pop();
        ofs << temp.first << " " << temp.second << endl;
    }

}

size_t AliasAPI::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int AliasAPI::DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr)
{
    std::string text(data, size);
    std::cout << text;
    return 0;
}

void AliasAPI::processJson(const string& jsonStr)
{
    json jsonData = json::parse(jsonStr);
    for(json::iterator it = jsonData.begin(); it!= jsonData.end(); ++it){
        cout << it.key() << " " << it.value() << endl;
    }
}





