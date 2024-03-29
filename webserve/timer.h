#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"

namespace sylar {

class TimerManager;

// 定时器
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    // 定时器的智能指针类型
    typedef std::shared_ptr<Timer> ptr;

    // 取消定时器
    bool cancel();

    // 刷新设置定时器的执行时间
    bool refresh();

    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms, bool from_now);
    
private:
    /**
     * @brief 构造函数
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 回调函数
     * @param[in] recurring 是否循环
     * @param[in] manager 定时器管理器
     */
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);

    /**
     * @brief 构造函数
     * @param[in] next 执行的时间戳(毫秒)
     */
    Timer(uint64_t next);

private:
    bool m_recurring = false;           // 是否循环定时器
    uint64_t m_ms = 0;                  // 执行周期
    uint64_t m_next = 0;                // 精确的执行时间
    std::function<void()> m_cb;         // 回调函数
    TimerManager* m_manager = nullptr;  // 定时器管理器

private:
    // 定时器比较仿函数，比较定时器的智能指针的大小(按执行时间排序)
    // lhs小为 true，rhs小为 false
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};


// 定时器管理器
class TimerManager {
friend class Timer;
public:
    
    typedef RWMutex RWMutexType;    // 读写锁类型
    TimerManager();                 //构造函数
    virtual ~TimerManager();        //析构函数

    /**
     * @brief 添加定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] recurring 是否循环定时器
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        ,bool recurring = false);

    /**
     * @brief 添加条件定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] weak_cond 条件，利用智能指针的引用计数来判断
     * @param[in] recurring 是否循环
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                        ,std::weak_ptr<void> weak_cond
                        ,bool recurring = false);

    // 获取到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTimer();

    // 获取需要执行的定时器的回调函数列表，是一个回调函数数组
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    // 是否有定时器
    bool hasTimer();

protected:
    // 当有新的定时器插入到定时器的首部,执行该函数
    virtual void onTimerInsertedAtFront() = 0;

    // 将定时器添加到管理器中
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

private:
    // 检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;                                // Mutex
    std::set<Timer::ptr, Timer::Comparator> m_timers;   // 定时器集合
    bool m_tickled = false;                             // 是否触发onTimerInsertedAtFront
    uint64_t m_previouseTime = 0;                       // 上次执行时间
};

}

#endif