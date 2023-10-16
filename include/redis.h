#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <string>

class Redis
{
public:
    Redis(std::string ip, int port);
    ~Redis();

    // 连接Redis服务器
    bool connect();

    // 键是否存在
    bool exists(const std::string &key);

    // 获取键的值
    bool getValue(const std::string &key, std::string& value);

    // 设置键的值
    bool setValue(const std::string &key, const std::string& value);

    // 删除键
    void delValue(const std::string &key);

    bool lock(std::string lockKey, std::string lockVal, int expire_time);
    void unlock(std::string lockKey, std::string lockVal);



    // 设置过期时间
    bool setExpireTime(const std::string &key, unsigned int seconds);

    // 向布隆过滤器添加元素
    void putBloomFilterRedis(const std::string &bloomFilter, const std::string &key);

    // 判断布隆过滤器是否存在key
    bool  existBloomFilterRedis(const std::string &bloomFilter, const std::string &key);
private:
    redisContext* m_redisConn;
    std::string m_ip;
    int m_port; 
};

#endif