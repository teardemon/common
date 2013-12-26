#ifndef _BASE_MAILBOX_H
#define _BASE_MAILBOX_H
#include "signal.h"
#include "readerwriterqueue.h"

namespace base{
  // notify_ ���ڻ���io�߳�,ezNotifyQueue��������
  template<typename T>
  class ezNotifyQueue
  {
  public:
    fd_t GetFd(){return notify_.getfd();}
    void Send(const T& t)
    {
      pipe_.enqueue(t);
      notify_.send();
    }
    bool Recv(T& t)
    {
      notify_.recv();
      return pipe_.try_dequeue(t);
    }
  private:
    typedef moodycamel::ReaderWriterQueue<T> pipe_t;
    pipe_t pipe_;
    ezSignaler notify_;
  };
} 

#endif