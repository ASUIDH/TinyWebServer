#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <cstdio>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/lock.h"

using namespace std;

class connection_pool{
public:
    MYSQL *GetConnection();
    bool ReleaseConnection(MYSQL *conn);
    int GetFreeConn();
    void DestroyPool();

    static connection_pool *GetInstance();

    void init(string url,string User,string Password,
    string DataBaseName,int port,int maxconn,int close_log);

private:
    connection_pool();
    ~connection_pool();

    int m_maxconn;
    int m_curconn;
    int m_freeconn;
    mutex lock;
    list<MYSQL *> connlist;
    sem reserve;

public:
    string m_url;
    string m_port;
    string m_user;
    string m_password;
    string m_databaseName;
    int m_close_log;//日志开关
};

class connectionRAII{

public:
    connectionRAII(MYSQL **SQL,connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};

#endif