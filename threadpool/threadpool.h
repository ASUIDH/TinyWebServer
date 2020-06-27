#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include <cstdio>
#include <exception>
#include "../lock/lock.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool{
private:
   int m_thread_number;
   int m_max_requests;
   pthread_t *m_threads;//线程池的数组
   std::list<T*> m_workqueue;
   mutex m_queuelock;
   sem m_queuestat;
   int m_actor_model;
   connection_pool *m_connpool;

   static void *worker(void *arg);
   void run();

public:
   threadpool(int actor_model,connection_pool *connpool,
   int thread_number = 8,int max_request = 10000);
   ~threadpool();
   bool append(T *request,int state);
   bool append_p(T *request);
};

template <typename T>
threadpool<T>::threadpool(int actor_model,connection_pool *connpool,
int thread_number,int max_request):
m_actor_model(actor_model),m_thread_number(thread_number),
m_max_requests(max_request),m_connpool(connpool){
   if(thread_number <= 0||max_request <= 0)
      throw std::exception();
   m_threads = new pthread_t[m_thread_number];
   if(m_threads == NULL)
      throw std::exception();
   for(int i = 0;i < thread_number;++i){
      if(pthread_create(m_threads + i,NULL,worker,this) != 0){
         delete[] m_threads;
         throw std::exception();
      }
      if(pthread_detach(m_threads[i])){
         delete[] m_threads;
         throw std::exception();
      }
   }
}

template <typename T>
threadpool<T>::~threadpool(){
   delete[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request,int state){
   m_queuelock.lock();
   if(m_workqueue.size() >= m_max_requests){
      m_queuelock.unlock();
      return false;
   }
   request->m_state = state;
   m_workqueue.push_back(request);
   m_queuelock.unlock();
   m_queuestat.post();
   return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request){
   m_queuelock.lock();
   if(m_workqueue.size() >= m_max_requests){
      m_queuelock.unlock();
      return false;
   }
   m_workqueue.push_back(request);
   m_queuelock.unlock();
   m_queuestat.post();
   return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg){
   threadpool *pool = (threadpool*)arg;
   pool->run();
   return pool;
}

template <typename T>
void threadpool<T>::run(){
   while(true){
      m_queuestat.wait();
      m_queuelock.lock();
      if(m_workqueue.empty()){
         m_queuelock.unlock();
         continue;
      }
      T *request = m_workqueue.front();
      m_workqueue.pop_front();
      m_queuelock.unlock();
      if(!request)continue;
      if(m_actor_model == 1){
         if(request->m_state == 0){
            if(request->read_once()){
               request->improv = 1;
               connectionRAII mysqlcon(&request->mysql,m_connpool);
               request->process();
            }
            else{
               request->improv = 1;
               request->timer_flag = 1;
            }
         }
         else if(request->write()){
            request->improv = 1;
         }
         else{
            request->improv = 1;
            request->timer_flag = 1;
         }
      }
      else{
         connectionRAII mysqlcon(&request->mysql,m_connpool);
         request->process();
      }
   }
}


#endif