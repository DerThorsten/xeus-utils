#ifndef XUTILS_MAGIC_HPP
#define XUTILS_MAGIC_HPP

#include <string>
#include <map>
#include <tuple>
#include <utility>

#include <boost/tokenizer.hpp>
#include <string>
#include <boost/mp11/tuple.hpp>
#include <boost/lexical_cast.hpp>

namespace xeus_utils
{

    class XEUS_UTILS_API MagicExecuter
    {
    public:
        using separator = boost::escaped_list_separator<char>;
        using tokenizer = boost::tokenizer<separator>;
        using token_iterator = tokenizer::iterator;
        using key_type = std::string;
        using callback_type = std::function<void(token_iterator, token_iterator)>;
            
        template<class F>
        void add_callback(const std::string & name, F && f)
        {
            m_callbacks.emplace(name, f);
        }

        template<class ...Types, class F>
        void add_typed_callback(const std::string & name, F && f)
        {
            using args_tuple = std::tuple< Types ...> ;
            auto packed_f = [&](token_iterator tokens_begin, token_iterator tokens_end){

                auto n_string_args = std::distance(tokens_begin, tokens_end);
                if(n_string_args != std::tuple_size<args_tuple>::value)
                {
                    throw std::runtime_error("wrong number of arguments");
                }

                // default construct all args
                args_tuple args;


                auto some_function = [&](auto & element)
                {
                    using element_type = std::decay_t<decltype(element)>;
                    element = boost::lexical_cast<element_type>(*tokens_begin);
                    ++tokens_begin;
                };
         
                // from string args to args
                std::apply([&](auto& ...x){(..., some_function(x));}, args);

                // call the callback
                std::apply(f, args);
            };

            m_callbacks.emplace(name, packed_f);
        }

        void dispatch(const std::string & code)
        {
            std::string separator1("");//dont let quoted arguments escape themselves
            std::string separator2(" ");//split on spaces
            std::string separator3("\"\'");//let it have quoted arguments
            separator els(separator1,separator2,separator3);
            tokenizer tok(code, els);
            token_iterator begin = tok.begin();
            const std::string cmd_name = *begin;
            ++begin;
            auto find_res = m_callbacks.find(cmd_name);
            if(find_res != m_callbacks.end())
            {
                find_res->second(begin, tok.end());
            }
        }
    private:

        std::map<key_type, callback_type>  m_callbacks;
    };
}

#endif