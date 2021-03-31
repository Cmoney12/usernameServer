//
// Created by Corey on 3/1/2021.
//

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifndef BOOSTWRITEJSON_CHAT_MESSAGE_HPP
#define BOOSTWRITEJSON_CHAT_MESSAGE_HPP

namespace pt = boost::property_tree;

class chat_message {
public:
    enum {HEADER_SIZE = 4 };

    chat_message(): body_length_(0)
    {
    }

    [[nodiscard]] const char* data() const
    {
        return data_;
    }

    char* data()
    {
        return data_;
    }

    [[nodiscard]] std::size_t length() const
    {
        return HEADER_SIZE + body_length_;
    }

    [[nodiscard]] const char* body() const
    {
        return data_ + HEADER_SIZE;
    }

    char* body()
    {
        return data_ + HEADER_SIZE;
    }

    [[nodiscard]] std::size_t body_length() const
    {
        return body_length_;
    }

    void display_json() const {
        pt::write_json(std::cout, root);
    }

    void body_length(std::size_t new_length)
    {
        body_length_ = new_length;
        if (body_length_ > MAXIMUM_MESSAGE_SIZE)
            body_length_ = MAXIMUM_MESSAGE_SIZE;
    }


    std::string write_json() {
        std::ostringstream oss;
        std::string message;
        pt::write_json(oss, root);
        body_length_ = oss.str().size();
        if (body_length_ <= MAXIMUM_MESSAGE_SIZE) {
            std::string str_size = std::to_string(body_length_);

            if (str_size.length() < HEADER_SIZE) {
                std::string zeros = (str_size.size() - HEADER_SIZE, "0");
                str_size = zeros + str_size;
                message = str_size + oss.str();
            }
        }
        return message;
    }

    std::string get_recipient() {
        std::string data_str(data_);
        data_str = data_str.substr(chat_message::HEADER_SIZE * 2, body_length_);
        int pos = (data_str).find('{');
        std::string json = data_str.substr(pos);

        std::istringstream is(json);
        pt::read_json(is, root);
        recipient = root.get<std::string>("Header.To");
        return recipient;
    }

    void read_json() {
        std::istringstream is(data_);
        pt::read_json(is, root);
        std::cout << root.get<std::string>("Header.To") << std::endl;
    }


    bool decode_header()
    {
        char header[HEADER_SIZE + 1] = "";
        std::strncat(header, data_, HEADER_SIZE);
        body_length_ = std::atoi(header);
        if (body_length_ > MAXIMUM_MESSAGE_SIZE)
        {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    std::string get_username() {

    }

    void create_tree(const std::string& receiver, const std::string& deliverer,
                     const std::string& body ="") {
        root.put("Header.To", receiver);
        root.put("Header.From", deliverer);
        //root.put("Header.Type", type_info);
        root.put("Contents.Body", body);
    }

private:
    std::size_t body_length_{};
    enum { MAXIMUM_MESSAGE_SIZE = 700 };
    pt::ptree root;
    std::string recipient;
    char data_[MAXIMUM_MESSAGE_SIZE + 4]{};
};


#endif //BOOSTWRITEJSON_CHAT_MESSAGE_HPP
