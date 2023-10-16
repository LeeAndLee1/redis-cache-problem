#include <thread>
#include <stdio.h>
#include <mutex>
#include <string>
#include <chrono>
#include <time.h>
#include <cstdlib>
#include "db.h"
#include "redis.h"
#include "json.hpp"

using json = nlohmann::json;

// 数据库配置信息
std::string ip = "127.0.0.1";
unsigned int dbport = 3306;
std::string username = "root";
std::string password = "123";
std::string dbname = "redisTestDB";
std::string table = "student";
// Redis配置信息
unsigned int redisport = 6379;

MySQL* g_mysql = nullptr;
Redis* g_redis = nullptr;

bool initRedisAndDB()
{
    g_mysql = new MySQL(ip, dbport, dbname, username, password);
    if (g_mysql == nullptr || !(g_mysql->connect()))
    {
        printf("mysql init fail.\n");
        return false;
    }
    g_redis = new Redis(ip, redisport);
    if (g_redis == nullptr || !(g_redis->connect()))
    {
        printf("redis init fail.\n");
        return false;
    }
    return true;
}

void finiRedisAndDB()
{
    if (g_mysql)
        delete g_mysql;
    if (g_redis)
        delete g_redis;
}

/* ================================= svolve1 : set random expire time ===========================================*/
void setRandomExpireTime(std::string redisKey)
{
    // 为了rand()每次生成不同的数
    srand((unsigned)time(NULL));
    // 生成[a, b]范围的随机数，使用 rand()%(b-a+1) + a
    int rd = rand()%(10-1+1) + 10;  //生成一个[1, 10]内的一个随机数
    g_redis->setExpireTime(redisKey, 5 + rd);    //设置key的过期时间为 5+rd
}

/* ================================= svolve2 : not set expire time ===========================================*/
void notSetExpireTime(std::string redisKey, std::string redisVal)
{
    // set key value 命令默认不设置过期时间
    g_redis->setValue(redisKey, redisVal);
}

int main()
{
    std::string redisKey = "student_id_4";
    //setRandomExpireTime(redisKey);
    return 0;
}
