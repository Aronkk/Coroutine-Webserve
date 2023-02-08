#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <iostream>
#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <functional>
#include <stdarg.h>
#include <time.h>

#include "singleton.h"
#include "util.h"
#include "thread.h"
#include "fiber.h"


// 定义一些宏指令
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getSS()

// 使用流式方式将日志级别debug的日志写入到logger
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)

// 使用流式方式将日志级别info的日志写入到logger
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

// 使用流式方式将日志级别warn的日志写入到logger
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

// 使用流式方式将日志级别error的日志写入到logger
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

// 使用流式方式将日志级别fatal的日志写入到logger
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

// 使用格式化方式将日志级别level的日志写入到logger
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

// 使用格式化方式将日志级别debug的日志写入到logger
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别info的日志写入到logger
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别warn的日志写入到logger
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别error的日志写入到logger
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

// 使用格式化方式将日志级别fatal的日志写入到logger
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)//

// 获取主日志器
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

// 获取name的日志器
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)



// 通过命名空间来避免函数重名
namespace sylar{

class Logger;
class LoggerManager; 

// 日志级别level，用枚举表示，注意用逗号隔开
class LogLevel{
public:
    enum Level {       
        UNKNOW = 0,     /// 未知级别
        DEBUG = 1,      /// DEBUG 级别     
        INFO = 2,       /// INFO 级别     
        WARN = 3,       /// WARN 级别    
        ERROR = 4,      /// ERROR 级别     
        FATAL = 5       /// FATAL 级别
    };

    static const char* ToString (LogLevel::Level Level);            //将日志级别转成文本输出
    static LogLevel::Level FromString(const std::string& str);      //将文本转换成日志级别
 };


// 每个日志生成的日志事件，定义其内容
class LogEvent{
public:
    // 定义一个共享类型的模板类智能指针，方便后续调用
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level m_level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name);

    const char* getFile() const { return m_file;}                       //返回文件名
    int32_t getLine() const { return m_line;}                           //返回行号
    uint32_t getElapse() const { return m_elapse;}                      //返回耗时
    uint32_t getThreadId() const { return m_threadId;}                  //返回线程ID
    uint32_t getFiberId() const { return m_fiberId;}                    //返回协程ID
    uint64_t getTime() const { return m_time;}                          //返回时间
    const std::string& getThreadName() const { return m_threadName;}    //返回线程名称
    std::string getContent() const { return m_ss.str();}                //返回日志内容
    std::shared_ptr<Logger> getLogger() const { return m_logger;}       //返回日志器
    LogLevel::Level getLevel() const { return m_level;}                 //返回日志级别

    std::stringstream& getSS() { return m_ss;}                          //返回日志内容字符串流
    void format(const char* fmt, ...);                                  //格式化写入日志内容
    void format(const char* fmt, va_list al);                           //格式化写入日志内容

private:    
    const char* m_file = nullptr;       /// 文件名   
    int32_t m_line = 0;                 /// 行号   
    uint32_t m_elapse = 0;              /// 程序启动开始到现在的毫秒数  
    uint32_t m_threadId = 0;            /// 线程ID  
    uint32_t m_fiberId = 0;             /// 协程ID  
    uint64_t m_time = 0;                /// 时间戳 
    std::string m_threadName;           /// 线程名称 
    std::stringstream m_ss;             /// 日志内容流  
    std::shared_ptr<Logger> m_logger;   /// 日志器
    LogLevel::Level m_level;            /// 日志等级
};


class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);                      // 构造函数, e 日志事件
    ~LogEventWrap();                                    // 析构函数

    LogEvent::ptr getEvent() const { return m_event;}   // 获取日志事件
    std::stringstream& getSS();                         // 获取日志内容流
private:
    LogEvent::ptr m_event;                              // 日志事件
};


// 日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    // 默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    //std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    // 日志格式会有多种，用子类进行定义
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){};
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();    // 初始化,解析日志模板
    bool isError() const {return m_error;}      // 是否有错误
    const std::string getPattern() const { return m_pattern;}   // 返回日志模板

private:  
    std::string m_pattern;                  /// 日志格式模板 
    std::vector<FormatItem::ptr> m_items;   /// 日志格式解析后格式
    bool m_error = false;                   /// 是否有错误
};


// 日志输出地（stdout，file）
class LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    // 因为日志的输出会有多种格式，因此将其设置为纯虚函数，由子类自行判断
    virtual ~LogAppender(){};
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level Level, const LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val;}
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;      // 日志级别   
    MutexType m_mutex;                              // Mutex
    LogFormatter::ptr m_formatter;                  // 日志格式器
    bool m_hasFormatter = false;                    // 是否有自己的日志格式器
};


// 日志器， 继承使得自己可以调用自己的智能指针
class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;
    Logger(const std::string& name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);   // 写日志（判断日志等级），下面的都是调用了这个函数
    void debug(LogEvent::ptr event);                        // 写debug级别日志
    void info(LogEvent::ptr event);                         // 写info级别日志
    void warn(LogEvent::ptr event);                         // 写warn级别日志
    void error(LogEvent::ptr event);                        // 写error级别日志
    void fatal(LogEvent::ptr event);                        // 写fatal级别日志

    void addAppender(LogAppender::ptr appender);            // 添加日志目标
    void delAppender(LogAppender::ptr appender);            // 删除日志目标
    void clearAppenders();                                  // 清空日志目标

    LogLevel::Level getLevel() const { return m_level;}     // 返回日志级别
    void setLevel(LogLevel::Level val) { m_level = val;}    // 设置日志级别
    const std::string& getName() const { return m_name;}    // 返回日志名称

    void setFormatter(LogFormatter::ptr val);               // 设置日志格式器
    void setFormatter(const std::string& val);              // 设置日志格式模板
    LogFormatter::ptr getFormatter();                       // 获取日志格式器
    std::string toYamlString();                             // 将日志器的配置转成YAML String

private:
    std::string m_name;                         // 日志名称
    LogLevel::Level m_level;                    // 日志级别
    std::list<LogAppender::ptr> m_appenders;    // Appener集合，用来存放日志信息
    LogFormatter::ptr m_formatter;              /// 日志格式器
    Logger::ptr m_root;                         /// 主日志器
    MutexType m_mutex;
};


// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
private:
};


// 输出到文件的Appender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    // 重新打开文件，成功打开返回true
    bool reopen();
private:  
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;    // 上次重新打开时间
};


// 日志管理器
class LoggerManager {
public:
    typedef Spinlock MutexType;
    LoggerManager();                                // 构造函数
    Logger::ptr getLogger(const std::string& name); // 获取日志器，name 日志器名称
    void init();                                    // 初始化
    Logger::ptr getRoot() const { return m_root;}   // 返回主日志器
    std::string toYamlString();                     // 将所有的日志器配置转成YAML String
private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;   // 日志器容器
    Logger::ptr m_root;                             // 主日志器
};

/// 日志器管理类单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;

}

#endif