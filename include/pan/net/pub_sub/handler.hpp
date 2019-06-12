#ifndef __PAN_NET_PUBSUB_HANDLER_HPP__
#define __PAN_NET_PUBSUB_HANDLER_HPP__

#include <memory>
#include <functional>
#include <pan/net/protocol.hpp>
#include <pan/net/protobuf.hpp>
#include <pan/net/pub_sub/pubsub.pb.h>
#include <pan/net/pub_sub/storage_base.hpp>
#include <pan/net/pub_sub/cancel.hpp>
#include <pan/net/pub_sub/history.hpp>
#include <pan/net/pub_sub/notify.hpp>
#include <pan/net/pub_sub/publish.hpp>
#include <pan/net/pub_sub/subscribe.hpp>

namespace pan { namespace net { namespace pubsub {

template <typename Storage>
class handler : public protocol::datagram_handler_base<handler<Storage>> {
    typedef protobuf::codec<session_type> codec_type;
    typedef processor<session_type, Pango::PubSub::Cancel> cancel_type;
    typedef processor<session_type, Pango::PubSub::History> history_type;
    typedef processor<session_type, Pango::PubSub::Topic> notify_type;
    typedef processor<session_type, Pango::PubSub::Publish> publish_type;
    typedef processor<session_type, Pango::PubSub::Subscribe> subscribe_type;
public:
    typedef Storage storage_type;
    typedef typename notify_type::topic_callback_type topic_callback_type;
    typedef std::map<std::string, std::list<session_ptr>> subscriber_map;
    //
    //typedef std::shared_ptr<Pango::PubSub::Topic> topic_ptr;
    //typedef std::list<topic_ptr> topic_list;
    //typedef std::map<topic_key, topic_list> topic_map;
    
    handler() 
        : codec_()
        , subscribers_()
        , cancel_(pool_, codec_, subscribers_)
        , history_(pool_, codec_)
        , notify_(pool_, codec_)
        , publish_(pool_, codec_, subscribers_)
        , subscribe_(pool_, codec_, subscribers_)
    {
        
    }

    void publish(const std::string& topic, const std::string& content)
    {
        publish_.publish(topic, content);
    }

    void subscribe(const std::string& topic)
    {
        subscribe_.subscribe(topic);
    }

    void cancel(const std::string& topic)
    {
        cancel_.cancel(topic);
    }

    void register_topic_callback(topic_callback_type cb)
    {
        notify_.register_topic_callback(std::move(cb));
    }
    
protected:
    void on_datagram(session_ptr session, datagram_ptr datagram)
    {
        // protobuf::codec is responsible to dispatch protobuf message 
        // to cancel/history/notify/publish/subscribe message.
        codec_.on_message(session, *datagram);
    }
    
protected:
    codec_type codec_;
    subscriber_map subscribers_;
    // message_queue_type mq_;
    cancel_type cancel_;
    history_type history_;
    notify_type notify_;
    publish_type publish_;
    subscribe_type subscribe_;

};

}}}

#endif // __PAN_NET_PUBSUB_HANDLER_HPP__