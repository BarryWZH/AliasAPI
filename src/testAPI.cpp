#include <iostream>
#include "AliasAPI.h"
#include <map>
#include <algorithm>
using namespace std;

int main()
{
    AliasAPI ap("/home/zhwang/WZH/SneakerAPI-Cpp/data/config.yaml");
    ap.autoUpList();
    // ap.loadSlug();
    // ap.save_sku("/home/zhwang/WZH/SneakerAPI-Cpp/data/sku.txt");
    return 0;
}