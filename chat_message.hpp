//
// Created by Corey on 3/1/2021.
//

#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#ifndef BOOSTWRITEJSON_CHAT_MESSAGE_HPP
#define BOOSTWRITEJSON_CHAT_MESSAGE_HPP


class chat_message: public std::enable_shared_from_this<chat_message> {
public:
    enum { header_length = 5 };
    enum {max_body_length = 99999 };
    std::unique_ptr<char[]> data_;

    chat_message()
            : body_length_(0)
    {
    }

    char* head() {
        return header;
    }

    std::unique_ptr<char[]>& body() {
        return data_;
    }

    std::size_t full_length() const {
        return body_length_+ header_length;
    }


    std::size_t body_length() const
    {
        return body_length_;
    }

    bool decode_header() {
        body_length_ = std::atoi(header);
        data_ = std::make_unique<char[]>(full_length() + 1);
        std::strncat(data_.get(), header, header_length);
        if (body_length_ > max_body_length)
        {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    std::string get_username() const {
        std::cout << data_.get() << std::endl;
        std::string username;
        rapidjson::Document json_doc(rapidjson::kObjectType);
        json_doc.Parse(data_.get() + header_length, body_length_);
        //json_doc.Parse(json.c_str());
        rapidjson::Value& head = json_doc["Header"];
        head.IsObject();
        rapidjson::Value::ConstMemberIterator itr = head.FindMember("To");
        if (itr != head.MemberEnd())
            username = itr->value.GetString();

        head.RemoveAllMembers();

        return username;
    }


private:
    std::size_t body_length_{};
    char header[header_length + 1]{};
};


#endif //BOOSTWRITEJSON_CHAT_MESSAGE_HPP
