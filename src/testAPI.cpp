#include <iostream>
#include "AliasAPI.h"
#include <map>
#include <algorithm>
using namespace std;

int main()
{
    AliasAPI ap("/home/zhwang/WZH/SneakerAPI-Cpp/data/config.yaml");
    ap.autoUpList();
    return 0;
}