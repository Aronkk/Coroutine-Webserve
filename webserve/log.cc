#include "log.h"
// #include "config.h"

namespace sylar{

class Logger;

const char* LogLevel::ToString (LogLevel::Level level){
    switch(level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e) {
}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);// 当函数的入参个数不确定时，使用va_list函数进行动态处理，增加编程的灵活性。
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr val){
    // MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if (m_formatter){
        m_hasFormatter = true;
    }else{
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    // MutexType::Lock lock(m_mutex);
    return m_formatter;
}


// 日志格式会有多种，用子类进行定义
// 获取日志内容
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

// 获取日志等级
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

// 获取耗时
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

// 获取日志名称
class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

// 获取线程ID
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

// 获取协程ID
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};


// 获取线程名
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

// 获取日志时间
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    // 定义好日志时间的格式
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);        // 系统自带的时间函数
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

// 获取日志文件名
class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

// 获取日志行号
class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

// 获取换行符
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};


class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

// 获取Tab
class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)
    ,m_threadName(thread_name)
    ,m_logger(logger)
    ,m_level(level) 
    {
}

Logger::Logger(const std::string& name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    // 定义一个最为常见的日志格式
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr val) {
    // MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for(auto& i : m_appenders) {
        // MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val) {
    // std::cout << "---" << val << std::endl;
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name = " << m_name
                  << " value = " << val << " invalid formatter"
                  << std::endl;
        return;
    }
    //m_formatter = new_val;
    setFormatter(new_val);
}

std::string Logger::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    // YAML::Node node;
    // node["name"] = m_name;
    // std::cout << "---m_name = " << m_name << std::endl;
    // if(m_level != LogLevel::UNKNOW) {
    //     node["level"] = LogLevel::ToString(m_level);
    // }
    // if(m_formatter) {
    //     node["formatter"] = m_formatter->getPattern();
    // }

    // for(auto& i : m_appenders) {
    //     node["appenders"].push_back(YAML::Load(i->toYamlString()));
    // }
    std::stringstream ss;
    // ss << node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
    // MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender){
    // MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()) {
        // MutexType::Lock l1(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender){
    // MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); it++){
        if (*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders(){
    // MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    // 只有日志等级 >= 当前日志等级的时候，将其信息输出
    if(level >= m_level) {
        auto self = shared_from_this();
        // MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL , event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename){
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event){
    if (level >= m_level){
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        // MutexType::Lock lock(m_mutex);
        // 当文件被删除时，会输出一个错误信息
        if (!(m_filestream << m_formatter->format(logger, level, event))){
            std::cout << "error" << std::endl;
        }
    }
}

std::string FileLogAppender::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    // YAML::Node node;
    // node["type"] = "FileLogAppender";
    // node["file"] = m_filename;
    // if(m_level != LogLevel::UNKNOW) {
    //     node["level"] = LogLevel::ToString(m_level);
    // }
    // if(m_hasFormatter && m_formatter) {
    //     node["formatter"] = m_formatter->getPattern();
    // }
    std::stringstream ss;
    // ss << node;
    return ss.str();
}

bool FileLogAppender::reopen(){
    // MutexType::Lock lock(m_mutex);
    if (m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    // !! 表示非0值转换成1，0值还是0
    return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event){
    if (level >= m_level){
        // MutexType::Lock lock(m_mutex);
        std::string str = m_formatter->format(logger, level, event);
        std::cout << str;
    }
}

std::string StdoutLogAppender::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    // YAML::Node node;
    // node["type"] = "StdoutLogAppender";
    // if(m_level != LogLevel::UNKNOW) {
    //     node["level"] = LogLevel::ToString(m_level);
    // }
    // if(m_hasFormatter && m_formatter) {
    //     node["formatter"] = m_formatter->getPattern();
    // }
    std::stringstream ss;
    // ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern(pattern){
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    
    std::stringstream ss;
    // 就是下面这个for循环出错
    for (auto& i : m_items){
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

// std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
//     for(auto& i : m_items) {
//         i->format(ofs, logger, level, event);
//     }
//     return ofs;
// }

// 解析日志格式
// %xxx %xxx{xxx} %%
void LogFormatter::init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        // 判断是否是日志格式 %d...
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        // 字符串中存在 % 
        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;     // 判断目前读取的字符串的状态，0/1/2
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;

        // %xxx{xxx}   -- nstr%str{fmt}的形式进行读取
        // 外层循环遍历整个pattern，直到遇到 % 
        // 内层循环从 % 下一个字符开始遍历，同时标记为状态0
        // 如果在状态0的情况下遇到 { 标记为状态1，获取 % 到 { 之间的子串    -- str
        // 如果在状态1的情况下遇到 } 标记为状态2，获取 { 到 } 之间的子串    -- fmt
        // 剩下的就是对获取的子串进行具体操作了
        // 下面是朴素做法，可以用有限状态机或者正则表达式
        
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);      // 读取 { 左边的字符，str
                    // std::cout << "*" << str << std::endl;
                    fmt_status = 1;      // string解析完毕，再解析format
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);    // { } 中间的字符，fmt
                    // std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;     // format解析完毕，状态改变
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);     // } 右边的字符，type
                }
            }
        }


        // 进行判断输出
        // fmt_status == 0/2 正常输出；   fmt_status == 1 输出格式错误
        // nstr 则是指 % 之前的字符串
        if(fmt_status == 0) {
            // 如果日志格式之前还有字符串，先放到vec中
            //std::cout<<nstr<<"  "<<str<<std::endl;
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            // std::cout << 0 << " * " << str << std::endl;
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    // 定义日志的格式，用哈希表存储，函数模板，内部实现也是宏定义
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),           // m:消息
        XX(p, LevelFormatItem),             // p:日志级别
        XX(r, ElapseFormatItem),            // r:累计毫秒数
        XX(c, NameFormatItem),              // c:日志名称
        XX(t, ThreadIdFormatItem),          // t:线程id
        XX(n, NewLineFormatItem),           // n:换行
        XX(d, DateTimeFormatItem),          // d:时间
        XX(f, FilenameFormatItem),          // f:文件名
        XX(l, LineFormatItem),              // l:行号
        XX(T, TabFormatItem),               // T:Tab
        XX(F, FiberIdFormatItem),           // F:协程id
        XX(N, ThreadNameFormatItem),        // N:线程名称
#undef XX
    };

    for(auto& i : vec) {
        // vec是tuble型，<str, fmt, type>，<0，1，2>，get是获取vec中的哪一个数据
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            // s_format_items，上面定义的获取日志的宏
            auto it = s_format_items.find(std::get<0>(i));            
            // 如果在宏中没有找到相对应的，输出错误信息
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->m_name] = m_root;
    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    // MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    // std::cout << name << std::endl;
    // return it == m_loggers.end() ? m_root : it->second;
    // if (it == m_loggers.end())
    //     return m_root;
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

// 对 LogDefine 和 string 之间的转换进行偏特化
// template<>
// class LexicalCast<std::string, LogDefine> {
// public:
//     LogDefine operator()(const std::string& v) {

//         // n 的属性：name、level、formater、appenders（type -- FileLogAppender、StdoutLogAppender）
//         // 都要判断是否存在，如果不存在就要抛出错误，存在就进行转换赋值
//         YAML::Node n = YAML::Load(v);
//         LogDefine ld;
//         if(!n["name"].IsDefined()) {
//             std::cout << "log config error: name is null, " << n
//                       << std::endl;
//             throw std::logic_error("log config name is null");
//         }
//         ld.name = n["name"].as<std::string>();

//         ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
//         if(n["formatter"].IsDefined()) {
//             ld.formatter = n["formatter"].as<std::string>();
//         }

//         if(n["appenders"].IsDefined()) {
//             //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
//             for(size_t x = 0; x < n["appenders"].size(); ++x) {
//                 auto a = n["appenders"][x];
//                 if(!a["type"].IsDefined()) {
//                     std::cout << "log config error: appender type is null, " << a
//                               << std::endl;
//                     continue;
//                 }
//                 std::string type = a["type"].as<std::string>();
//                 LogAppenderDefine lad;
//                 if(type == "FileLogAppender") {
//                     lad.type = 1;

//                     // 如果文件名没有定义，抛出错误
//                     if(!a["file"].IsDefined()) {
//                         std::cout << "log config error: fileappender file is null, " << a
//                               << std::endl;
//                         continue;
//                     }
//                     lad.file = a["file"].as<std::string>();
//                     if(a["formatter"].IsDefined()) {
//                         lad.formatter = a["formatter"].as<std::string>();
//                     }   
//                 } else if(type == "StdoutLogAppender") {
//                     lad.type = 2;
//                     if(a["formatter"].IsDefined()) {
//                         lad.formatter = a["formatter"].as<std::string>();
//                     } 
//                 } else {
//                     std::cout << "log config error: appender type is invalid, " << a
//                               << std::endl;
//                     continue;
//                 }

//                 ld.appenders.push_back(lad);
//             }
//         }
//         // std::cout << "----" << ld.name << "----" << ld.appenders.size() << std::endl;
//         return ld;
//     }
// };

// template<>
// class LexicalCast<LogDefine, std::string> {
// public:
//     std::string operator()(const LogDefine& i) {
//         YAML::Node n;
//         n["name"] = i.name;
//         if(i.level != LogLevel::UNKNOW) {
//             n["level"] = LogLevel::ToString(i.level);
//         }
//         if(!i.formatter.empty()) {
//             n["formatter"] = i.formatter;
//         }

//         // 日志 log 中的 appenders 是数组结构，包括 type 和 file
//         for(auto& a : i.appenders) {
//             YAML::Node na;
//             if(a.type == 1) {
//                 na["type"] = "FileLogAppender";
//                 na["file"] = a.file;
//             } else if(a.type == 2) {
//                 na["type"] = "StdoutLogAppender";
//             }
//             if(a.level != LogLevel::UNKNOW) {
//                 na["level"] = LogLevel::ToString(a.level);
//             }

//             if(!a.formatter.empty()) {
//                 na["formatter"] = a.formatter;
//             }
//             // na["formatter"] = i.formatter;
//             n["appenders"].push_back(na);
//         }

//         std::stringstream ss;
//         ss << n;
//         return ss.str();
//     }
// };


// sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
//     sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";

            // 对于日志的变化有三种状态：新增，修改，删除
            // 新增 -- 新的有，旧的没有
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = SYLAR_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = SYLAR_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                //std::cout << "** " << i.name << " level=" << i.level
                //<< "  " << logger << std::endl;
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        // if(!sylar::EnvMgr::GetInstance()->has("d")) {
                        //     ap.reset(new StdoutLogAppender);
                        // } else {
                        //     continue;
                        // }
                        ap.reset(new StdoutLogAppender);
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name = " << i.name << " appender type = " << a.type
                                      << " formatter = " << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                } 
            }

            // 删除 -- 旧的有，新的没有；并且真的删除，而是关闭文件或者提高日志等级不进行输出
            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

std::string LoggerManager::toYamlString() {
    // MutexType::Lock lock(m_mutex);
    // YAML::Node node;
    // // std::cout << "m_loggers.size() = " << m_loggers.size() <<std::endl;
    // for(auto& i : m_loggers) {
    //     node.push_back(YAML::Load(i.second->toYamlString()));
    // }
    std::stringstream ss;
    // ss << node;
    return ss.str();
}

void LoggerManager::init() {
}

}
