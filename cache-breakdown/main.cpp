/*
* 解决方案一：缓存空对象:
* 当查询一条不存在的数据时，在缓存中存储一个空对象并设置一个过期时间(设置过期时间是为了避免出现数据库中存在了数据但是缓存中仍然是空数据现象)，
* 这样可以避免所有请求全部查询数据库的情况。
*/
#include <thread>
#include <stdio.h>
#include <mutex>
#include <string>
#include <chrono>
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

std::string queryInfoFromDB(std::string table, int id, MySQL* pMysql)
{
    std::string result;
    char sql[1024] = {0};
    sprintf(sql, "select * from %s where id = %d", table.c_str(), id);

    MYSQL_RES* res = pMysql->query(sql);
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

/* ================================= problem : TestProblem ===========================================*/
int TestProblem()
{
    if (!initRedisAndDB())
        return -1;
    int id = 4;
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
            std::string queryDB = queryInfoFromDB(table, id, g_mysql);
            if (!queryDB.empty())
            {
                // 写回redis.非常耗时，这里用注释来代表耗时
                // g_redis->setValue(key, queryDB);
                printf("get info from db success, info = %s, id = %d\n", queryDB.c_str(), id);
            }
        }
    }

    finiRedisAndDB();
    return 0;
}

/* ================================= solve1 : SetNoExpire ===========================================*/
int SetNoExpire()
{
    if (!initRedisAndDB())
        return -1;
    int id = 4;
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
            else
            {
                printf("==== select from cache , get data success ====\n");
            }        
        }
        else
        {
            // redis不存在key，查询数据库
             printf("==== select from db ====\n");
            std::string queryDB = queryInfoFromDB(table, id, g_mysql);
            if (!queryDB.empty())
            {
                // 写回redis，设置key永不过期
                g_redis->setValue(key, queryDB);
                printf("get info from db success, info = %s, id = %d\n", queryDB.c_str(), id);
            }
        }
    }

    finiRedisAndDB();
    return 0;
}


/* ================================= solve2 : UseMutex ===========================================*/
std::vector<MySQL*> mysqls;
std::vector<Redis*> redises;

void initRedisAndDBs(int connNum)
{
    for (int i=0; i<connNum; i++)
    {
        MySQL* mysql = new MySQL(ip, dbport, dbname, username, password);
        Redis* redis = new Redis(ip, redisport);
        if (mysql != nullptr && mysql->connect() && redis != nullptr && redis->connect())
        {
            mysqls.push_back(mysql);
            redises.push_back(redis);
            continue;
        }

        if (mysql != nullptr)
        {
            delete mysql;
        }
        if (redis != nullptr)
        {
            delete redis;
        }
    }
}

void finiRedisAndDBs()
{
    for (int i=0; i<mysqls.size(); i++)
    {
        delete mysqls[i];
        delete redises[i];
    }
}

void workThread(std::string redisKey, std::string lockKey, std::string lockVal, uint64_t timeout, 
            Redis* pRedis, MySQL* pMysql, std::string tableName, int id)
{
    std::string info;
    
    pRedis->getValue(redisKey, info);
    if (!info.empty())
    {
        printf("==== select from cache , get data success ====\n");
    }
    else
    {
        bool lockFlag = pRedis->lock(lockKey, lockVal, timeout);   //尝试加锁
        // 如果加锁成功，先再次查询缓存，有可能上一个线程查询并添加到缓存了
        if (lockFlag)
        {
            std::string retValue;
            pRedis->getValue(redisKey, retValue);
            if (!retValue.empty())
            {
                printf("==== select from cache, get data sucess! ====\n");
            }
            else
            {
                printf("==== select from db ====\n");
                std::string queryDB = queryInfoFromDB(table, id, pMysql);
                if (!queryDB.empty())
                {
                    // 写回redis，设置key永不过期
                    pRedis->setValue(redisKey, queryDB);
                    printf("get info from db success, info = %s, id = %d\n", queryDB.c_str(), id);
                }
            }
            pRedis->unlock(lockKey, lockVal);
        }
    }

}

/* ================================= solve2 : UseMutex ===========================================*/
int UseMutex()
{
    initRedisAndDBs(4);
    std::string redisKey = "student_id_4";
    std::string tableName = "student";
    int id = 4;

    struct timespec time = {0, 0};
    clock_gettime(CLOCK_REALTIME, &time);
    std::string lockVal = std::to_string(time.tv_nsec);
    std::string lockKey = "redis.lock";
    uint64_t timeout = 300;

    // 多个客户端同时查询 key 的情况
    std::vector<std::thread> threads(10);
    for (int i=0; i<mysqls.size(); i++)
    {
        threads[i] = std::thread(workThread, redisKey, lockKey, lockVal, timeout, redises[i], mysqls[i], tableName, id);
        // 为了模拟出第一个线程成功回写缓存后，其他线程直接访问缓存现象
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    for (int i=0; i<mysqls.size(); i++)
    {
        threads[i].join();
    }
    finiRedisAndDBs();
    return 0;
} 

int main()
{
    // TestProblem();
    // SetNoExpire();
    UseMutex();
    return 0;
}