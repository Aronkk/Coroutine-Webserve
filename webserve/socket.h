#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "address.h"
#include "noncopyable.h"

namespace sylar {

// Socket封装类
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    // Socket类型
    enum Type {
        TCP = SOCK_STREAM,      // TCP类型
        UDP = SOCK_DGRAM        // UDP类型
    };

    // Socket协议簇
    enum Family {  
        IPv4 = AF_INET,     // IPv4 socket
        IPv6 = AF_INET6,    // IPv6 socket
        UNIX = AF_UNIX,     // Unix socket
    };

    // 创建TCP Socket(满足地址类型)
    static Socket::ptr CreateTCP(sylar::Address::ptr address);
    // 创建UDP Socket(满足地址类型)
    static Socket::ptr CreateUDP(sylar::Address::ptr address);

    // 创建IPv4的TCP Socket
    static Socket::ptr CreateTCPSocket();
    // 创建IPv4的UDP Socket
    static Socket::ptr CreateUDPSocket();

    // 创建IPv6的TCP Socket
    static Socket::ptr CreateTCPSocket6();
    // 创建IPv6的UDP Socket
    static Socket::ptr CreateUDPSocket6();

    // 创建Unix的TCP Socket
    static Socket::ptr CreateUnixTCPSocket();
    // 创建Unix的UDP Socket
    static Socket::ptr CreateUnixUDPSocket();

    /**
     * @brief Socket构造函数
     * @param[in] family 协议簇
     * @param[in] type 类型
     * @param[in] protocol 协议
     */
    Socket(int family, int type, int protocol = 0);

    virtual ~Socket();

    // 获取发送超时时间(毫秒)
    int64_t getSendTimeout();
    // 设置发送超时时间(毫秒)
    void setSendTimeout(int64_t v);

    // 获取接受超时时间(毫秒)
    int64_t getRecvTimeout();
    // 设置接受超时时间(毫秒)
    void setRecvTimeout(int64_t v);

    // 获取sockopt @see getsockopt
    bool getOption(int level, int option, void* result, socklen_t* len);
    // 获取sockopt模板
    template<class T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    // 设置sockopt
    bool setOption(int level, int option, const void* result, socklen_t len);
    // 设置sockopt模板
    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    /**
     * @brief 接收connect链接
     * @return 成功返回新连接的socket,失败返回nullptr
     * @pre Socket必须 bind , listen  成功
     */
    virtual Socket::ptr accept();

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 连接地址
     * @param[in] addr 目标地址
     * @param[in] timeout_ms 超时时间(毫秒)
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听socket
     * @param[in] backlog 未完成连接队列的最大长度
     * @result 返回监听是否成功
     * @pre 必须先 bind 成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    // 关闭socket
    virtual bool close();

    /**
     * @brief 发送数据，使用 send 函数
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int send(const void* buffer, size_t length, int flags = 0);

    /**
     * @brief 发送数据 使用 sendmsg 函数
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的长度
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     * @param[in] to 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接受数据 使用 recv 函数
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(void* buffer, size_t length, int flags = 0);

    /**
     * @brief 接受数据 使用 recvmsg 函数
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buffers 接收数据的内存(iovec数组)
     * @param[in] length 接收数据的内存大小(iovec数组长度)
     * @param[out] from 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    //获取远端地址
    Address::ptr getRemoteAddress();
    // 获取本地地址
    Address::ptr getLocalAddress();

    // 获取协议簇
    int getFamily() const { return m_family;}
    // 获取类型
    int getType() const { return m_type;}
    // 获取协议
    int getProtocol() const { return m_protocol;}

    // 返回是否连接
    bool isConnected() const { return m_isConnected;}
    // 是否有效(m_sock != -1)
    bool isValid() const;
    // 返回Socket错误
    int getError();

    // 输出信息到流中
    virtual std::ostream& dump(std::ostream& os) const;
    virtual std::string toString() const;
    // 返回socket句柄
    int getSocket() const { return m_sock;}

    // 取消读
    bool cancelRead();
    // 取消写
    bool cancelWrite();
    // 取消accept
    bool cancelAccept();
    // 取消所有事件
    bool cancelAll();

protected:
    // 初始化socket
    void initSock();
    // 创建socket
    void newSock();
    // 初始化sock
    virtual bool init(int sock);
protected:
    int m_sock;                         // socket句柄
    int m_family;                       // 协议簇
    int m_type;                         // 类型
    int m_protocol;                     // 协议
    bool m_isConnected;                 // 是否连接
    Address::ptr m_localAddress;        // 本地地址
    Address::ptr m_remoteAddress;       // 远端地址
};


// class SSLSocket : public Socket {
// public:
//     typedef std::shared_ptr<SSLSocket> ptr;

//     static SSLSocket::ptr CreateTCP(sylar::Address::ptr address);
//     static SSLSocket::ptr CreateTCPSocket();
//     static SSLSocket::ptr CreateTCPSocket6();

//     SSLSocket(int family, int type, int protocol = 0);
//     virtual Socket::ptr accept() override;
//     virtual bool bind(const Address::ptr addr) override;
//     virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
//     virtual bool listen(int backlog = SOMAXCONN) override;
//     virtual bool close() override;
//     virtual int send(const void* buffer, size_t length, int flags = 0) override;
//     virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
//     virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
//     virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
//     virtual int recv(void* buffer, size_t length, int flags = 0) override;
//     virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
//     virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
//     virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

//     bool loadCertificates(const std::string& cert_file, const std::string& key_file);
//     virtual std::ostream& dump(std::ostream& os) const override;
// protected:
//     virtual bool init(int sock) override;
// private:
//     std::shared_ptr<SSL_CTX> m_ctx;
//     std::shared_ptr<SSL> m_ssl;
// };

/**
 * @brief 流式输出socket
 * @param[in, out] os 输出流
 * @param[in] sock Socket类
 */
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif