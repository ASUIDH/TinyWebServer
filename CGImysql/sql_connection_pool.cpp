#include <mysql/mysql.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"
#include "../log/log.h"

connection_pool::connection_pool(){
    m_curconn = 0;
    m_freeconn = 0;
}
/*??????*/
connection_pool *connection_pool::GetInstance(){
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(string url,string User,string Password,string DataBaseName,int port,int maxconn,int close_log){
    m_url = url;
    m_port = port;
    m_user = User;
    m_password = Password;
    m_databaseName = DataBaseName;
    m_close_log = close_log;

    for(int i = 0;i < maxconn; i++){
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con ==NULL){
            LOG_ERROR("MYSQL Error");
            exit(1);
        }
        con = mysql_real_connect(con,url.c_str(),User.c_str(),Password.c_str(),DataBaseName.c_str(),port,NULL,0);

        if(con == NULL){
            LOG_ERROR("MYSQL Error");
            exit(1);
        }
        connlist.push_back(con);
        +m_freeconn;

        reserve = sem(m_freeconn);

        m_maxconn = m_freeconn;

    }
}

MYSQL *connection_pool::GetConnection(){
    MYSQL *con = NULL;

    if(connlist.size() == 0)
    return NULL;

    reserve.wait();

    lock.lock();

    con = connlist.front();
    connlist.pop_front();

    --m_freeconn;
    ++m_curconn;
    lock.unlock();

    return con;
}

bool connection_pool::ReleaseConnection(MYSQL *con){
    if(con == NULL)
    return false;

    lock.lock();

    connlist.push_back(con);
    ++m_freeconn;
    --m_curconn;

    lock.unlock();
    reserve.post();
    return true;
}

void connection_pool::DestroyPool(){
    lock.lock();
    if(connlist.size() > 0){
        list<MYSQL*>::iterator iter;
        for(iter = connlist.begin();iter != connlist.end();++iter){
            MYSQL *con = *iter;
            mysql_close(con);
        }
        m_curconn = 0;
        m_freeconn = 0;
        connlist.clear();
    }
    lock.unlock();
}

int connection_pool::GetFreeConn(){
    return m_freeconn;
}

connection_pool::~connection_pool(){
    DestroyPool();
}


connectionRAII::connectionRAII(MYSQL **SQL,connection_pool *connpool){
    *SQL = connpool->GetConnection();

    conRAII = *SQL;
    poolRAII = connpool;
}

connectionRAII::~connectionRAII(){
    poolRAII->ReleaseConnection(conRAII);
}