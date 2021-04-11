//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <unordered_map>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<std::shared_ptr<chat_message>> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
    virtual ~chat_participant() = default;
    virtual void deliver(const std::string& recipient, const std::shared_ptr<chat_message>& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(const std::string& username, const chat_participant_ptr& participant)
    {
        participants_.emplace(username, participant);
        //for (const auto& msg: recent_msgs_)
        //    participant->deliver(msg);
    }

    void leave(const chat_participant_ptr& participant)
    {
        for (auto it = participants_.begin(); it != participants_.end(); )
        {
            if (it->second == participant) {
                participants_.erase(it++);
            }
            else {
                ++it;
            }
        }

    }

    void deliver(const std::string& recipient, const std::shared_ptr<chat_message>& msg)
    {
        if (participants_.find(recipient) != participants_.end()) {
            recent_msgs_.push_back(msg);
            while (recent_msgs_.size() > max_recent_msgs)
                recent_msgs_.pop_front();

            auto it = participants_.find(recipient);
            it->second->deliver(recipient, msg);
        }
    }

private:
    std::unordered_map<std::string, chat_participant_ptr> participants_;
    enum { max_recent_msgs = 100 };
    chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
        : public chat_participant,
          public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room)
            : socket_(std::move(socket)),
              room_(room)
    {
    }

    void start()
    {
        read_username();
        do_read_header();

    }

    void deliver(const std::string& recipient, const std::shared_ptr<chat_message>& msg) override
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            do_write();
        }
    }

    void read_username() {
        boost::asio::async_read_until(socket_, buf, "\n", [this]
                (boost::system::error_code ec, std::size_t size) {
            if(!ec) {
                handle_username(ec, size);
            }
        });
    }

    void handle_username(boost::system::error_code, std::size_t size) {
        std::stringstream message;

        message << std::istream(&buf).rdbuf();
        buf.consume(size);
        std::cout << message.str();
        std::string username = message.str();
        message.clear();
        int pos = username.find('\n');
        username = username.substr(0,pos);
        room_.join(username, shared_from_this());
    }

private:
    void do_read_header()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_->head(), chat_message::header_length),
                                [this, self](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec && read_msg_->decode_header())
                                    {
                                        do_read_body();
                                    }
                                    else
                                    {
                                        room_.leave(shared_from_this());
                                    }
                                });
    }

    void do_read_body()
    {
        auto self(shared_from_this());
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_->data_.get() + chat_message::header_length, read_msg_->body_length()),
                                [this, self](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        std::string test = read_msg_->get_username();
                                        //std::string receiver = read_msg_.get_recipient();
                                        room_.deliver(test, read_msg_);
                                        do_read_header();
                                    }
                                    else
                                    {
                                        room_.leave(shared_from_this());
                                    }
                                });
    }

    void do_write()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front()->data_.get(),
                                                     write_msgs_.front()->full_length()),
                                 [this, self](boost::system::error_code ec, std::size_t /*length*/)
                                 {
                                     if (!ec)
                                     {
                                         write_msgs_.pop_front();
                                         if (!write_msgs_.empty())
                                         {
                                             do_write();

                                         }
                                     }
                                     else
                                     {
                                         room_.leave(shared_from_this());
                                     }
                                 });
    }

    tcp::socket socket_;
    chat_room& room_;
    std::shared_ptr<chat_message> read_msg_ = std::make_shared<chat_message>();
    //chat_message read_msg_;
    chat_message_queue write_msgs_;
    boost::asio::streambuf buf;
};

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server(boost::asio::io_context& io_context,
                const tcp::endpoint& endpoint)
            : acceptor_(io_context, endpoint)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::make_shared<chat_session>(std::move(socket), room_)->start();
                    }

                    do_accept();
                });
    }

    tcp::acceptor acceptor_;
    chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
            return 1;
        }

        boost::asio::io_context io_context;

        std::list<chat_server> servers;
        for (int i = 1; i < argc; ++i)
        {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.emplace_back(io_context, endpoint);
        }

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
