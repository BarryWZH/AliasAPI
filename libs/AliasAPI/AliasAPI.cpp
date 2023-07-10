#include "AliasAPI.h"
using namespace std;

AliasAPI::AliasAPI(string configfilepath){

    // 读取配置文件数据
    readConfig(configfilepath);
    
    // 获取表格数据
    getData();

    // 初始化curl
    initializeCurl();

    // 设置状态位
    init_flag_ = false;

    // 初始化获取auth_token、access_token
    getToken();
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

    // cout << height << " " << width << endl;

    for (int i = 1; i < height; ++i)
    {
        auto cellrow = ws.rows()[i];
        // sku
        vsku_.push_back(cellrow[0].to_string());
        // size
        vsize_.push_back(atof(cellrow[1].to_string().c_str()));
        // num
        vnum_.push_back(atoi(cellrow[2].to_string().c_str()));
        // price * 100
        vprice_.push_back(atof(cellrow[3].to_string().c_str()) * 100);
        // uplist flag
        vuplist_.push_back(atoi(cellrow[4].to_string().c_str()));
    }

    // for(int i=0;i<vsku_.size();++i)
    // {
    //     cout << "sku: "   << vsku_[i] << " ";
    //     cout << "size:"   << vsize_[i] << " ";
    //     cout << "num:"    << vnum_[i] << " ";
    //     cout << "price:"  << vprice_[i] << " ";
    //     cout << "uplist:" << vuplist_[i] << endl;
    // }
    
    // 加载 unprocessed的search_slug
    total_num_ = 0;
    for(int i=0; i< vsku_.size(); ++i){
        mpsku2id_[vsku_[i]] = i;
        for(int j=0;j<vnum_[i]; ++j)
        {
            unprocessed_.push(vsku_[i]);
            total_num_++;
        }
    }
    cout << "TotalNum: " << total_num_ << endl;

    // 搜索线程设置为0；
    numsearchthread_ = 0;
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

void AliasAPI::search_by_sku(string keyword, int page, bool& found)
{
    response_.clear();
    string requesturl = base_url_ + function_url_["query_by_sku_url"] + "?token=" + token_
    + "&keyword=" + keyword + "&page=" + to_string(page);

    // cout << requesturl << endl;

    curl_easy_setopt(curl_, CURLOPT_URL, requesturl.c_str());

    CURLcode res = curl_easy_perform(curl_);
    if(res == CURLE_OK)
    {
        json jsonData = json::parse(response_);
        // 查找到
        if(jsonData["data"]["hits"].size() > 0){
            products_slug_[keyword] = jsonData["data"]["hits"][0]["slug"];
            // cout << products_slug_[keyword] << endl;
            found = true;
        }
        else
            found = false;
    }
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
                string word = unprocessed_.front();
                unprocessed_.pop();
                
                // thread t(&AliasAPI::search_by_sku, this, std::move(word), 0, std::move(found));
            
        }
    }
}

void AliasAPI::listing_product_multi(string params, map<string, int>& mpdata)
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
            if(size > 0)
            {
                for(int i=0;i<size;++i)
                {
                    string sku = jsonData["data"]["succeeded"][i]["product"]["sku"];
                    replace(sku.begin(), sku.end(), ' ', '-');
                    processed_.push(sku);
                    mpdata[sku]--;
                }
            }
        }

        // add failure
        for(auto it: mpdata)
        {
            int size = it.second;
            for(int i=0;i<size;++i)
                unprocessed_.push(it.first);
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
    bool found = false;
    int t=0;

    json jsonData;
    json products;
    map<string, int> mpdata;

    while(!unprocessed_.empty())
    {
        string keyword = unprocessed_.front();
        unprocessed_.pop();

        if(!products_slug_.count(keyword))
            search_by_sku(keyword, 0, found);
        // 依然没找到
        t = 0;
        while(!products_slug_.count(keyword))
        {
            search_by_sku(keyword, 0, found);
            if(++t > 3)
                break;
        }
        if(!found) 
        {
            unprocessed_.push(keyword);
            continue; //还没找到就跳过,将这个号放后面
        }

        // if found
        string slug = products_slug_[keyword];
        int id = mpsku2id_[keyword];
       
        json temp;
        temp["sku"] = keyword;
        temp["product_slug"] = slug;
        temp["price_cents"] = vprice_[id];
        temp["size_us"] = vsize_[id];

        mpdata[keyword]++;

        products.push_back(temp);
        patch++;

        if(patch >= patchuplistsize_){
            jsonData["products"] = products;
            // cout << jsonData.dump() << endl;
            auto t1 = chrono::steady_clock::now();
            listing_product_multi(jsonData.dump(), mpdata);
            auto t2 = chrono::steady_clock::now();
            cout << "multiuplist: " << chrono::duration<double, ratio<1, 1000>>(t2 - t1).count() << " ms" << endl;
            patch = 0;
            products.clear();
            jsonData.clear();
            mpdata.clear();
        }

        if(processed_.size() == total_num_)
        {
            cout << "Uplist all products successfully" << endl;
            break;
        }
    }
}

// --------------------- function -----------------------

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





