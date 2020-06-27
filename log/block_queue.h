#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/lock.h"

template <typename T>
class block_queue{
public:
    block_queue(int max_size){
        if(max_size <= 0){
            exit(-1);
        }

        m_maxsize = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    void clear(){
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    ~block_queue(){
        m_mutex.lock();
        if(m_array != NULL){
            delete [] m_array;
        }
        m_mutex.unlock();
    }

    bool full(){
        m_mutex.lock();
        if(m_size >= m_maxsize){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty(){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    bool back(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    int size(){
        int ans = 0;
        m_mutex.lock();
        ans = m_size;
        m_mutex.unlock();
        return ans;
    }

    int max_size(){
        int ans = 0;
        m_mutex.lock();
        ans = m_maxsize;
        m_mutex.unlock();
        return ans;
    }

    bool push(const T &item){
        m_mutex.lock();
        if(m_size >= m_maxsize){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_maxsize;
        m_array[m_back] = item;

        m_size++;

        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    bool pop(T &item){
        m_mutex.lock();
        while(m_size <= 0){
            if(!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_maxsize;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    bool pop(T &item,int ms_timeout){
        //超时处理
    }

private:
    mutex m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_maxsize;
    int m_front;
    int m_back;
};

#endif