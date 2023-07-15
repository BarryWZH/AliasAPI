#include "AliasAPI.h"
using namespace std;

#define DEBUG 0

int tttt=0;

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
}

void AliasAPI::getData(){

    xlnt::workbook wb;

    try {
        wb.load(datapath_);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load workbook: " << e.what() << std::endl;
        return;
    }

    // 获取第一个工作表
    auto ws = wb.active_sheet();
    
    // 遍历行和列
    int height = ws.rows().length();
    int width = ws.columns().length();

    // 加载上架数据
    for (int i = 1; i < height; ++i)
    {
        auto cellrow = ws.rows()[i];
        // sku
        string str = cellrow[0].to_string();
        replace(str.begin(), str.end(), ' ', '-');
        vsku_.push_back(str);
        // size
        vsize_.push_back(atof(cellrow[1].to_string().c_str()));
        // num
        vnum_.push_back(atoi(cellrow[2].to_string().c_str()));
        // price * 100
        vprice_.push_back(atof(cellrow[3].to_string().c_str()) * 100);
        // uplist flag
        vuplist_.push_back(atoi(cellrow[4].to_string().c_str()));
        // sku2id
        products_id_[str] = i-1;
    }

    // 加载本地存储的slug信息和对应尺码信息
    loadSlug();

    // 更新slug信息和对应的尺码信息
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
            if(slug_size_[vsku_[i]].count(vsize_[i])){
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
    cout << "TotalNum InValud:       " << invalid_.size() << endl;
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
    }

    ifs.close();
}

void AliasAPI::initializeCurl()
{
    // 初始化curl
    curl_ = curl_easy_init();
    if (curl_)
        cout << "Init curl successful!\n";
    else
        cout << "Init curl failed!\n";

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
    if(!init_auth())
        return;
    if(!login())
        return;
    init_flag_ = true;
    cout << "Get token successful!" << endl;
}
// --------------------- API ----------------------------

string AliasAPI::ping(){
    return "";
}

bool AliasAPI::init_auth()
{
    string requesturl = base_url_ + function_url_["init_auth_url"] + "?token=" + token_;
    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        if(jsonData.contains("data")){
            auth_ = jsonData["data"]["auth"];
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

void AliasAPI::search_by_sku(string keyword, int page, string& slug, int& found)
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
            slug = jsonData["data"]["hits"][0]["slug"];
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

// void AliasAPI::save_sku(string filename)
// {
//     map<string, string> sku2slug;
//     bool isfound;
//     queue<string> q;
//     for(int i=0; i<vsku_.size(); ++i){
//         q.push(vsku_[i]);
//     }
//     int num=0;
//     int N = 10;   // 10个线程
//     vector<string> vslug(N);
//     vector<string> vkey(N);
//     vector<int64_t> vb(N);
//     vector<thread> vthread;
//     mutex mtx;
//     ofstream ofs(filename, ios::app);
//     while(!q.empty())
//     {
//         vthread.clear();
//         int num = 0;
//         while(num < N)
//         {
//             if(q.empty()) break;
//             string keyword = q.front();
//             q.pop();
//             if(sku2slug.count(keyword))
//                 continue;
//             vkey[num] = keyword;
//             vthread.push_back(thread(&AliasAPI::search_by_sku, this, keyword, 0, ref(vslug[num]), vb[num]));
//             num++;
//         }
//         for(int i=0; i<vthread.size(); ++i)
//             vthread[i].join();

//         for(int i=0; i<vthread.size();++i)
//         {
//             if(vb[i])
//             {
//                 sku2slug[vkey[i]] = vslug[i];
//                 ofs << vkey[i] << " " << vslug[i] << endl;
//             }
//             else
//                 q.push(vkey[i]);
//         }
//     }
//     ofs.close();
// }

void AliasAPI::save_sku()
{
    ofstream ofs(slugpath_, ios::app);
    int isfound = 0;
    queue<string> q;
    for(int i=0; i<vsku_.size(); ++i){
        if(products_slug_.count(vsku_[i])) continue;
        q.push(vsku_[i]);
    }
    int num=0;
    string slug;
    while(!q.empty())
    {
        cout << q.size() << endl;
        string keyword = q.front();
        q.pop();
        cout << num++ << " " << keyword;
        if(products_slug_.count(keyword)) 
        {
            // found
            cout << endl;
            continue;
        }
        search_by_sku(keyword, 0, slug, isfound);
        if(isfound)
        {
            vector<float> vsize;
            get_product_detail(slug, vsize, isfound);
            if(isfound)
            {
                cout << " " << slug << " ";
                cout << vsize.size() << " ";
                products_slug_[keyword] = slug;
                slug_products_[slug] = keyword;
                for(int  i=0; i<vsize.size(); ++i)
                {
                    slug_size_[slug].insert(vsize[i]);
                    cout << vsize[i] << " ";
                }
                cout << endl;
                ofs << keyword << " " << slug << " ";
                ofs << vsize.size() << " ";
                for(int i=0; i<vsize.size(); ++i)
                    ofs << vsize[i] << " ";
                ofs << endl;
            }
            else
            {
                q.push(keyword);
                cout << endl;
                continue;
            }
        }
        else
        {
            q.push(keyword);
            cout << endl;
        }
    }
    cout << "savesku complete!\n";
    ofs.close();
}

void AliasAPI::patch_search()
{
    while(1)
    {
        if(unprocessed_.size() > 0)
        {
            mtxsearch_.lock();
            bool found = false;
            if(numsearchthread_ <= patchsearchsize_)
                // string word = unprocessed_.front();
                unprocessed_.pop();
                
                // thread t(&AliasAPI::search_by_sku, this, std::move(word), 0, std::move(found));
            
        }
    }
}

void AliasAPI::listing_product_multi(string params)
{
    response_.clear();
    string requesturl = base_url_ + function_url_["listing_product_multi_url"] + "?token=" + token_
    + "&auth=" + auth_ + "&access_token=" + access_token_;

    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, params.length());
    
    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        // cout << response_ << endl;
        // 分别对succeeded和failed的做处理
        if(jsonData["data"].contains("succeeded"))
        {
            int size = jsonData["data"]["succeeded"].size();
            tttt+=size;
            if(size > 0)
            {
                for(int i=0;i<size;++i)
                {
                    string sku = jsonData["data"]["succeeded"][i]["product"]["sku"];
                    float size = jsonData["data"]["succeeded"][i]["size_option"]["value"];
                    replace(sku.begin(), sku.end(), ' ', '-');
                    processed_.push({sku, size});
                }
            }
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
            int size = jsonData["data"]["failed"].size();
            auto temp = jsonData["data"]["failed"];
            for(int i = 0; i<size; ++i)
            {
                string slug = temp[i]["product_id"];
                float size = temp[i]["size_option"]["value"];
                string sku = slug_products_[slug];
                unprocessed_.push({sku, size});
            }
            
        }
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

    while(!unprocessed_.empty())
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
        patch++;

        if(patch >= patchuplistsize_){
            jsonData["products"] = products;
            // cout << jsonData.dump() << endl;
            auto t1 = chrono::steady_clock::now();
            listing_product_multi(jsonData.dump());
            auto t2 = chrono::steady_clock::now();
            cout << "multiuplist: " << chrono::duration<double>(t2 - t1).count() << " s" << endl;
            patch = 0;
            products.clear();
            jsonData.clear();
        }
    }

    cout << "Successful Uplist:      " << tttt << endl;
    cout << "Processed queue size:   " << processed_.size() << endl;
    cout << "Unprocessed queue size: " << unprocessed_.size() << endl;

    cout << "TotalNum UpList:        " << total_num_ << endl;
    cout << "TotalNum InValud:       " << invalid_.size() << endl;
    cout << "TotalNum UnFound:       " << unfound_.size() << endl;

    WriteResult();

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





