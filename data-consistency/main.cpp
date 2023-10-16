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

/*
* 1. 创建3个线程，线程1更新数据库，线程2、3读取缓存
* 2. 线程1加锁，删除缓存
* 3. 线程1更新数据库
* 4. 线程1休眠 x 秒
* 5. 线程1再次删除缓存
*/

void updateDB(std::string redisKey, Redis* pRedis, MySQL* pMysql, std::string tableName, int id, std::string newName, uint64_t delayTime)
{
    // 不加锁，直接删除缓存
    pRedis->delValue(redisKey);
    printf("del redis first time!\n");
    // 更新数据库
    char sql[1024] = {0};
    sprintf(sql, "update student set name = '%s' where id = %d", newName.c_str(), id);
    pMysql->update(sql);
    printf("update database!\n");
    // 延迟删除缓存
    std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
    pRedis->delValue(redisKey);
    printf("del redis second time!\n");
}

void searchData(std::string redisKey, Redis* pRedis, MySQL* pMysql, std::string tableName, int id)
{
    // redis存在key,则获取value
    std::string info;
    pRedis->getValue(redisKey, info);
    if (!info.empty())
    {
        printf("==== select from redis, data : %s ====\n", info.c_str());
    }     
    else
    {
        // redis不存在key，查询数据库
        std::string queryDB = queryInfoFromDB(table, id, pMysql);
        if (!queryDB.empty())
        {
            // 写回redis
            pRedis->setValue(redisKey, queryDB);
            printf("==== search db and write to redis success, data = %s. ====\n", queryDB.c_str());
        }
    }
}

int main()
{
    initRedisAndDBs(5);

    int id = 4;
    std::string tableName = "student";
    std::string redisKey = tableName + "_id_" + std::to_string(id);
    std::string newName = "wwwwww";
    uint64_t delayTime = 60;

    std::vector<std::thread> threads(10);
    threads[0] =  std::thread(updateDB, redisKey, redises[0], mysqls[0], tableName, id, newName, delayTime);

    for (int i=1; i<mysqls.size(); i++)
    {
        threads[i] = std::thread(searchData, redisKey, redises[i], mysqls[i], tableName, id);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    for (int i=0; i<mysqls.size(); i++)
    {
        threads[i].join();
    }

    finiRedisAndDBs();
    return 0;
}