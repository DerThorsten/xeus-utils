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
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include "xeus-utils/xeus_stream.hpp"

#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"



namespace xeus_utils
{



    class XEUS_UTILS_API CliAppWrapper
    {
    public:
        CliAppWrapper(

            CLI::App& app,
            std::vector<std::string> & rev_tokens
        )
        :   m_app(app),
            m_rev_tokens(rev_tokens)
        {

        }

        CLI::App& operator()()
        {
            return m_app;
        }
        template<class F>
        void parse(F && f)
        {
            bool exception_caught = true;
            try{
                m_app.parse(m_rev_tokens);
                exception_caught = false;
            }
            catch(const CLI::CallForHelp & e)
            {
                xinterpreter_cout()<<m_app.help()<<"\n";
            }
            // catch (const CLI::ParseError &e) {
            //     xinterpreter_cout()<<"error :"<<e.get_name()<<"\n"<<e.what()<<"\n";
            // }
            if(!exception_caught)
            {
                f();
            }
        }
    private:
        CLI::App& m_app;
        std::vector<std::string> & m_rev_tokens;
    };


    enum class MagicState 
    { 
        NO_MAGIC,
        MAGIC,
        ERROR
    };


    class XEUS_UTILS_API MagicExecuter
    {
    public:

        MagicExecuter(const std::string & magic_prefix = "%")
        : m_magic_prefix(magic_prefix)
        {

        }
        using separator = boost::escaped_list_separator<char>;
        using tokenizer = boost::tokenizer<separator>;
        using token_iterator = tokenizer::iterator;
        using key_type = std::string;
        using callback_type = std::function<void(CliAppWrapper&)>;
            

        template<class F>
        void add_callback(const std::string & cmd_name, F && f)
        {
            m_callbacks.emplace(m_magic_prefix + cmd_name, std::forward<F>(f));
        }

        MagicState dispatch(std::string code)
        {
            static const xinterpreter_cout xcout;
            if(!boost::algorithm::starts_with(code, m_magic_prefix))
            {
                return MagicState::NO_MAGIC;
            }

            std::vector<std::string> lines;
            boost::split(lines, code, boost::is_any_of("\n"));

            for(auto & line : lines)
            {

                boost::algorithm::trim(line);
                
                if(boost::algorithm::starts_with(line, m_magic_prefix))
                {
                    std::string separator1("");//dont let quoted arguments escape themselves
                    std::string separator2(" ");//split on spaces
                    std::string separator3("\"\'");//let it have quoted arguments
                    separator els(separator1,separator2,separator3);
                    tokenizer tok(line, els);
                    token_iterator begin = tok.begin();
                    token_iterator end = tok.end();


                    std::vector<std::string> rev_tokens(begin, end);
                    std::reverse(rev_tokens.begin(), rev_tokens.end());

                    const std::string cmd_name = rev_tokens.back();
                    rev_tokens.pop_back();

                    CLI::App app{cmd_name};
                    CliAppWrapper app_wrapper(app, rev_tokens);
                    auto find_res = m_callbacks.find(cmd_name);
                    if(find_res != m_callbacks.end())
                    {
                        xcout<<find_res->first<<":\n";
                        auto & cb = find_res->second;
                        cb(app_wrapper);
                    }
                    else
                    {
                        xcout<<"did not found "<<cmd_name<<"\n";
                    }
                }
                else
                {
                    break;
                }

            }
            return MagicState::MAGIC;
        }
        

       
    private:

        std::map<key_type, callback_type>  m_callbacks;
        std::string m_magic_prefix;
    };
}

#endif