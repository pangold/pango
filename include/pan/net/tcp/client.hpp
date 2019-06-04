#ifndef __PAN_NET_TCP_CLIENT_HPP__
#define __PAN_NET_TCP_CLIENT_HPP__

#include <boost/asio.hpp>
#include <pan/base.hpp>
#include <pan/net/tcp/session.hpp>

namespace pan { namespace net { namespace tcp {
    
template <typename Handler>
class client : public noncopyable {
public:
    typedef Handler handler_type;
    typedef connector<handler_type> connector_type;
    typedef session<handler_type> session_type;
    typedef typename session_type::key_type key_type;
    typedef typename session_type::pointer session_ptr;
    typedef pan::map<key_type, session_ptr> pool_type;

    explicit client(const std::string& host = "localhost", uint16_t port = 8888)
        : io_context_()
        , handler_()
        , connector_(io_context_, host, std::to_string(port), handler_)
    {
        auto pred = std::bind(&client::new_session, this, std::placeholders::_1);
        connector_.register_session_callback(pred);
        thread_ = std::thread([this]() { io_context_.run(); });
    }

    virtual ~client()
    {
        if (session_) session_->stop();
        thread_.join();
    }

    void write(const std::string& data)
    {
        wait_for_session();
        session_->write(data.data(), data.size());
    }

protected:
    void new_session(session_ptr session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        session_ = session;
        cond_.notify_one();
    }

    void wait_for_session()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!session_) {
            cond_.wait(lock);
        }
    }

protected:
    boost::asio::io_context io_context_;
    handler_type handler_;
    connector_type connector_;
    session_ptr session_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

};

}}}

#endif // __PAN_NET_TCP_CLIENT_HPP__