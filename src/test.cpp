#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

string token = "AHAO-031c2e43651e45028cbfc5ef5e917864";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userptr)
{
    std::string text(data, size);
    std::cout << text;
    return 0;
}

void processJson(const std::string& jsonStr)
{
    try
    {
        json jsonData = json::parse(jsonStr);

        if(jsonData.contains("token") && jsonData["token"].is_string())
        {
            string token = jsonData["token"].get<string>();
            cout << "接收到的token参数: " << token << endl;
        }
        else
        {
            cout << "未找到有效的token参数!" << std::endl;
        }
    }
    catch(const std::exception& e)
    {
        cout << "解析JSON出错!" << endl;
        cout << "错误信息:" << e.what() << endl;
    }
}

// 处理响应头
size_t HeaderCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

int main()
{
    CURL* curl = curl_easy_init();
    // 设置请求参数
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "param1=value1&param2=value2");

    if (curl)
    {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, "http://goatapi.spiderx.cc:61030/api/alias/init_auth");

        // 设置请求头
        struct curl_slist *headers = NULL;
        // headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // json params;
        // params["token"] = "AHAO-031c2e43651e45028cbfc5ef5e917864";
        // string paramsStr = params.dump();
        // cout << paramsStr << endl;
        // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, paramsStr.c_str());
        // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, paramsStr.length());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, DebugCallback);

        // string headerResponse;
        // curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        // curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerResponse);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);

        if (res == CURLE_OK)
        {
            nlohmann::json json;
            cout << response << endl;

            // std::cout << headerResponse << std::endl;
            
            // 获取Content-Type头信息
            std::string contentType;
            // size_t pos = headerResponse.find("Content-Type:");
            // if (pos != std::string::npos)
            // {
            //     size_t end = headerResponse.find("\r\n", pos);
            //     if (end != std::string::npos)
            //     {
            //         contentType = headerResponse.substr(pos + 13, end - (pos + 13));
            //     }
            // }
            
            // std::cout << "Content-Type: " << contentType << std::endl;
        }
        else
        {
            std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return 1;
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}
