#include "redis.h"
#include <iostream>
#include <cstring>
#include <stdio.h>

Redis::Redis(std::string ip, int port)
{
    this->m_ip = ip;
    this->m_port = port;
}

Redis::~Redis()
{
    if (m_redisConn != nullptr)
    {
        redisFree(m_redisConn);
    }
}

 bool Redis::connect()
 {
    m_redisConn = redisConnect(m_ip.c_str(), m_port);
    if (nullptr == m_redisConn)
    {
        std::cerr << "connect redis server fail!" <<std::endl;
        return false;
    }
    return true;
 }

bool Redis::exists(const std::string &key)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "EXISTS %s", key.c_str());
    if (reply == nullptr)
    {
        std::cerr << "EXISTS command failed!" << std::endl;
        return false;
    }
    int exists = reply->integer;
    freeReplyObject(reply);
    if (exists == 1)
    {
        return true;
    }
    return false;
}

bool Redis::getValue(const std::string &key, std::string& value)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "GET %s", key.c_str());
    if (reply == nullptr)
    {
        std::cerr << "GET command failed!" << std::endl;
        return false;
    }
    if (reply->str == nullptr)
    {
        value = "";
    }
    else
    {
        value = reply->str;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::setValue(const std::string &key, const std::string& value)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr)
    {
        std::cerr << "SET command failed!" << std::endl;
        return false;
    }
    int iRet = 0;
    if (reply->type != REDIS_REPLY_STATUS)
    {
        printf("set value fail : %s\n", reply->str);
        iRet = 0;
    }
    iRet = 1;
    freeReplyObject(reply);
    return iRet;
}

void Redis::delValue(const std::string &key)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "DEL %s", key.c_str());
    if (reply == nullptr)
    {
        std::cerr << "del key failed!" << std::endl;
        return;
    }
    int flag = reply->integer;
    if (flag != 1)
    {
        printf("del key fail : %d\n", flag);
    }
    freeReplyObject(reply);
}

bool Redis::setExpireTime(const std::string &key, unsigned int seconds)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "SETEX %s %d EXPIRE", key.c_str(), seconds);
    if (reply == nullptr)
    {
        std::cerr << "SET EXPIRE command failed!" << std::endl;
        return false;
    }
    int iRet = 0;
    if (reply->type != REDIS_REPLY_STATUS)
    {
        printf("SET EXPIRE fail : %s\n", reply->str);
        iRet = 0;
    }
    iRet = 1;
    freeReplyObject(reply);
    return iRet;
}

void Redis::putBloomFilterRedis(const std::string &bloomFilter, const std::string &key)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "bf.add %s %s", bloomFilter.c_str(), key.c_str());
    if (reply == nullptr)
    {
        std::cerr << "bf.add command failed!" << std::endl;
        return;
    }
    //printf("interger=%d, type=%d, str=%s\n", reply->integer, reply->type, reply->str);
    freeReplyObject(reply);
}

bool  Redis::existBloomFilterRedis(const std::string &bloomFilter, const std::string &key)
{
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "bf.exists %s %s", bloomFilter.c_str(), key.c_str());
    if (reply == nullptr)
    {
        std::cerr << "bf.exists command failed!" << std::endl;
        return false;
    }
    int exists = reply->integer;
    freeReplyObject(reply);
    if (exists == 1)
    {
       return true;
    }
    return false;
}

// 获取一个redis分布式锁
bool Redis::lock(std::string lockKey, std::string lockVal, int expire_time) {
    while(1) {
		// 使用SET命令设置key-value-NX-EX
        redisReply *reply = (redisReply*)redisCommand(m_redisConn, "SET %s %s NX EX %d", lockKey.c_str(), lockVal.c_str(), expire_time);
        // 判断返回结果是否为OK
        if (reply != nullptr && reply->type== REDIS_REPLY_STATUS && reply->str!=nullptr && std::string(reply->str).compare("OK")==0)
        {
            freeReplyObject(reply);
            return true;
        }
        else if (reply != nullptr)
        {
            freeReplyObject(reply);
        }
	}
    return false;
}

// 释放一个redis分布式锁
void Redis::unlock(std::string lockKey, std::string lockVal) {
    char * script = "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end";
    redisReply* reply = (redisReply*)redisCommand(m_redisConn, "EVAL %s 1 %s %s", script, lockKey.c_str(), lockVal.c_str());
	if (reply != NULL && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1)
    {
		//printf("get lock and unlock sucess.\n");
	}
	else {
		printf("get lock fail.\n");
	}
    freeReplyObject(reply);
}