#include "timer.h"

//template<class F>
//Timer::Timer(F func) : m_func(func)
//{
//  m_bexit.store(false);
//  m_bimmediately_run.store(false);
//
//}

Timer::~Timer()
{}

void Timer::Start(unsigned msec, bool immediatelyrun, unsigned execnum)
{
  cout << __FUNCTION__ << endl;
  // 间隔时间为0或默认无效值，直接返回
  if (msec == 0)
  if (!msec)
  {
    return;
  }
  m_bexit.store(false);
  m_imsec = msec;
  m_bimmediately_run.store(immediatelyrun);
  m_thread = std::thread(std::bind(&Timer::Run,this));
}

void Timer::Stop()
{
  cout << __FUNCTION__ << endl;
  m_bexit.store(true);
  m_cond.notify_all(); // 唤醒线程

  if (m_thread.joinable())
  {
    m_thread.join();
  }

}

void Timer::SetExit(bool b_exit)
{
  m_bexit.store(b_exit);
}

void Timer::Run()
{
  if (m_bimmediately_run.load()) // 立即执行判断
  {
    if (m_func)
    {
      m_func();
    }
  }

  if (!m_bexit.load())
  {
    {
      // 锁放在花括号内部，减小锁的粒度
      std::unique_lock<std::mutex> locker(m_mutex);

      // 如果是被唤醒的，需要判断先条件确定是不是虚假唤醒
      // wait_for是等待第三个参数满足条件，当不满足时，超时后继续往下执行
      m_cond.wait_for(locker, std::chrono::milliseconds(m_imsec), [this]() { return m_bexit.load(); });
    }

    if (m_bexit.load()) // 再校验下退出标识
    {
      return;
    }

    if (m_func)
    {
      m_func();

    }
    std::cout<<"--------------  exit  -------------"<<std::endl;
  }
}

