#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <functional>
 
using namespace std;
 
 
// 定时器类型
/*
   主要数据：
   1.一个线程变量，保存定时器线程
   2.一个互斥锁，配合条件变量使用
   3.一个条件变量，结合互斥锁，可以是线程不执行任务时，睡眠一段时间，在退出调用时，可以唤醒线程完成退出
   4.定时执行函数，具体的定时执行业务操作
   5.间隔时间，定时器间隔一段时间调用定时执行函数
   6.一个退出标识，标志是否退出定时线程循环
   7.立即执行标识，标识新建状态的定时线程是否立即执行一次任务，而不需等待一个间隔时间才开始执行第一次任务
*/

/**
 *
 */

class Timer
{
  public:
    template<typename F>
      Timer(F func) : m_func(func)
    {
      m_bexit.store(false);
      m_bimmediately_run.store(false);

    }

    ~Timer();

    /**
     * @brief 启动定时器
     * @param msec 每次执行的间隔，单位(ms)
     * @param immediatelyrun 是否立即运行
     * @param execnum 执行次数
     *
     */
    void Start(unsigned msec, bool immediatelyrun = false, unsigned execnum = 1);

    // 结束
    void Stop();

    void SetExit(bool b_exit);

private:
  void Run();

private: // 私有数据部分
  std::atomic<bool> m_bexit;
  std::atomic<bool> m_bimmediately_run; // 是否立即执行
  unsigned int m_imsec = 1000;	// 间隔时间
  std::function<void()> m_func;	// 执行函数
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};

