#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar{

// 定义一个静态线程局部变量，用来指向当前线程
// 只在作用于当前线程，不影响多线程的使用，所以 get 到的是 t_thread，而不是 m_thread
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count){
    // 初始化信号量，（信号量变量的地址；0 用在线程间，非0 用在进程间；信号量中的值）
    if (sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init erroe");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    // 对信号量加锁，调用一次对信号量的值-1，如果值为0，就阻塞
    if (sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait erroe");
    }
}

void Semaphore::notify(){
    // 对信号量解锁，唤醒wait中的信号量，调用一次对信号量的值+1
    if (sem_post(&m_semaphore)){
        throw std::logic_error("sem_post erroe");
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    // 创建线程失败
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    // 利用信号量 wait，确保线程先创建成功
    m_semaphore.wait();
}

Thread::~Thread() {
    if(m_thread) {
        // 分离一个线程。被分离的线程在终止的时候，会自动释放资源返回给系统
        // 一次只能分离一个线程，需要线程的 id
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread) {
        // 和一个已经终止的线程进行连接，回收子线程的资源
        // 这个函数是阻塞函数，调用一次只回收一个子线程的资源
        // 一般在主线程中使用，不需要返回值 - nullptr
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;  // 将传入参数转换成线程参数
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();

    // 为线程设置唯一名称，设置成内核及其接口中可见的线程名称，用于调试多线程应用程序
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    // 防止智能指针的应用被释放掉
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // 当线程 run之后，再将信号量唤醒
    thread->m_semaphore.notify();

    cb();
    return 0;
}

}