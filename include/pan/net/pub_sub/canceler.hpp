#ifndef __PAN_NET_PUBSUB_CANCELER_HPP__
#define __PAN_NET_PUBSUB_CANCELER_HPP__

#include <pan/base.hpp>
#include <pan/net/protocol.hpp>
#include <pan/net/pub_sub/processor.hpp>

namespace pan { namespace net { namespace pubsub {

template <typename Session>
class canceler : public processor_base<Session> {
public:
    typedef Pango::PubSub::Cancel message_type;
    typedef std::shared_ptr<message_type> message_ptr;
    typedef std::function<void(session_ptr, message_ptr)> complete_callback_type;
    typedef std::function<void(const std::string&)> reply_callback_type;
    typedef std::tuple<message_ptr, reply_callback_type> reply_operation;
    typedef std::map<int32_t, reply_operation> confirmation_map;

    canceler(codec_type& codec, subscriber_map& subs)
        : processor_base<Session>("Pango.PubSub.Cancel", codec, subs)
    {
        using namespace std::placeholders;
        auto cb = std::bind(&canceler::on_message, this, _1, _2);
        codec.register_callback<message_type>(cb);
    }

    // for client side.
    void set_session(session_ptr session)
    {
        session_ = session;
    }

    // for server side.
    // after processing sub request, you can do some extra thing, 
    // such as store it in DB through this callback
    void register_complete_callback(complete_callback_type cb)
    {
        complete_callback_ = std::move(cb);
    }

    // for client side.
    void register_reply_callback(reply_callback_type cb)
    {
        reply_callback_ = std::move(cb);
    }

    // for client side.
    void cancel(const std::string& topic)
    {
        static int32_t number = 0;
        auto message = std::make_shared<message_type>();
        message->set_number(++number);
        message->set_topic(topic);
        if (!session_) {
            LOG_ERROR("pubsub.cancel: session is not ready yet");
            return;
        }
        codec().send(session_, message);
        confirmations_[number] = std::make_tuple(message, reply_callback_);
        // TODO: post timer queue of io_services(event loop)
        // to handle timeout...
    }

private:
    // entrance of received data(callback from elsewhere), 
    // which had been decoded to specific protobuf message.
    void on_message(session_ptr session, message_ptr message)
    {
        if (message->has_reply()) {
            on_reply(session, message);
        } else {
            on_cancel(session, message);
            reply(session, message);
        }
    }

    // reply from server
    void on_reply(session_ptr session, message_ptr message)
    {
        auto it = confirmations_.find(message->number());
        if (it != confirmations_.end()) {
            auto& callback = std::get<1>(it->second);
            if (callback) callback(message->topic());
            confirmations_.erase(message->number());
        }
    }

    // request to server
    void on_cancel(session_ptr session, message_ptr message)
    {
        auto& subers = subscribers()[message->topic()];
        auto it = subers.find(session->to_string());
        if (it != subers.end()) {
            if (complete_callback_) complete_callback_(session, message);
            subers.erase(it);
        }
    }

    // reply to client
    void reply(session_ptr session, message_ptr message)
    {
        message->set_reply(true);
        codec().send(session, message);
    }

private:
    session_ptr session_;
    confirmation_map confirmations_;
    complete_callback_type complete_callback_;
    reply_callback_type reply_callback_;

};

}}}

#endif // __PAN_NET_PUBSUB_CANCELER_HPP__