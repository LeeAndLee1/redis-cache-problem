/*
* 解决方案一：缓存空对象:
* 当查询一条不存在的数据时，在缓存中存储一个空对象并设置一个过期时间(设置过期时间是为了避免出现数据库中存在了数据但是缓存中仍然是空数据现象)，
* 这样可以避免所有请求全部查询数据库的情况。
*/
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

std::string queryInfoFromDB(std::string table, int id)
{
    std::string result;
    char sql[1024] = {0};
    sprintf(sql, "select * from %s where id = %d", table.c_str(), id);

    MYSQL_RES* res = g_mysql->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            json js;
            js["id"] = atoi(row[0]);
            js["name"] = row[1];
            js["age"] = atoi(row[2]);
            result = js.dump();
            mysql_free_result(res);
            return result;
        }
    }
    return result;
}

bool rewriteRedis(std::string& key, std::string& info)
{
    if (g_redis->setValue(key, info))
    {
        printf("rewrite redis success!\n");
        return true;
    }
    return false;
}

/*
* 正常情况下查询逻辑如下：
* redis未查询到 --》 查询数据库 --》数据库返回 --》 写回redis
* 缓存穿透：是指查询缓存和数据库中都不存在的数据，导致所有的查询压力全部给到了数据库。
* 当请求量很大时，这种写法会有问题，所有请求都落在数据库上，导致数据库崩溃
*/
/* ================================= problem : TestProblem ===========================================*/
int TestProblem()
{
    if (!initRedisAndDB())
        return -1;
    int id = 8;
    std::string tableName = "student";
    std::string key = tableName + "_id_" + std::to_string(id);
    std::string info;
    // 模拟客户端发送5次查询key的请求
    for (int i=0; i<5; i++)
    {
        // redis存在key,则获取value
        if (g_redis->exists(key))
        {
            g_redis->getValue(key, info);
            if (info.empty())
            {
                printf("==== select from cache , data is empty ====\n");
            }        
        }
        else
        {
            // redis不存在key，查询数据库
             printf("==== select from db ====\n");
            std::string queryDB = queryInfoFromDB(table, id);
            if (!queryDB.empty())
            {
                // 写回redis
                g_redis->setValue(key, queryDB);
                printf("get info from db success, info = %s, id = %d\n", queryDB.c_str(), id);
            }
        }
    }

    finiRedisAndDB();
    return 0;
}

/* ================================= svolve1 : TestSlove1CacheNullValue ===========================================*/
int TestSlove1CacheNullValue()
{
    if (!initRedisAndDB())
        return -1;
    int id = 8;
    std::string tableName = "student";
    std::string key = tableName + "_id_" + std::to_string(id);
    std::string info;
    for (int i=0; i<5; i++)
    {
        // redis存在key,则获取value
        if (g_redis->exists(key))
        {
            g_redis->getValue(key, info);
            if (info.empty())
            {
                printf("==== select from cache , data is empty ====\n");
            }        
        }
        else
        {
            // redis不存在key，查询数据库
             printf("==== select from db ====\n");
            std::string queryDB = queryInfoFromDB(table, id);
            // 查到该数据，写回redis；如果没查到该数据，将空值写回redis
            g_redis->setValue(key, queryDB);
            //printf("get info from db success, info = %s, id = %d\n", queryDB.c_str(), id);
        }
    }
    
    finiRedisAndDB();
    return 0;
}

/* ================================= svolve2 : bloomFilter ===========================================*/
int TestSlove1BloomFilter()
{
    if (!initRedisAndDB())
        return -1;
    
    // 先将所有的key同步到布隆过滤器中
    std::string filter = "filter1";
    std::string tableName = "student";
    for (int i=1; i<5; i++)
    {
        std::string keyTmp = tableName + "_id_" + std::to_string(i);
        g_redis->putBloomFilterRedis(filter, keyTmp);
    }

    for (int id=4; id<9; id++)
    {
        std::string key = tableName + "_id_" + std::to_string(id);
        // 布隆过滤器中是否有可能存在这个key
        bool b = g_redis->existBloomFilterRedis(filter, key);
        if(!b){
            // 如果不存在，直接返回空
            printf("==== select from bloomFilter , not find ====\n");
            continue;
        }

        // redis存在key，直接获取value
        std::string info;
        g_redis->getValue(key, info);
        if (!info.empty())
        {
            printf("==== select from cache success ====\n");
        }     
        else
        {
            // redis不存在key，查询数据库
             printf("==== select from db ====\n");
            std::string queryDB = queryInfoFromDB(table, id);
            // 查到该数据，写回redis
            if (!queryDB.empty())
            {
                g_redis->setValue(key, queryDB);
            }
        }
    }
    
    finiRedisAndDB();
    return 0;
}

int main()
{
    // TestProblem();
    // TestSlove1CacheNullValue();
    TestSlove1BloomFilter();
    return 0;
}

