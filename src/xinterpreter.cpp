/***************************************************************************
* Copyright (c) 2022, Thorsten Beier                                  
*                                                                          
* Distributed under the terms of the BSD 3-Clause License.                 
*                                                                          
* The full license is in the file LICENSE, distributed with this software. 
****************************************************************************/

#include <string>
#include <vector>
#include <iostream>

#include "nlohmann/json.hpp"

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"
#include "xeus/xhelper.hpp"
#include "xeus-utils/xinterpreter.hpp"
#include "xeus-utils/xmagic.hpp"

#ifdef XEUS_UTILS_EMSCRIPTEN_WASM_BUILD
#include "xeus-utils/xfetch.hpp"
#include "xeus-utils/xzip.hpp"
#endif
#include "xeus-utils/xfilesystem.hpp"

namespace nl = nlohmann;

namespace xeus_utils
{
 
    interpreter::interpreter()
    {
        using token_iterator = typename MagicExecuter::token_iterator;
        xeus::register_interpreter(this);
        static const xinterpreter_cout xcout;

        m_magic_executor.add_callback("fubar",[&](auto & app){

            std::string url;
            app().add_option("-u,--url",url, "an url")->required();

            app.parse([&](){
                xcout<<"parsed successful url:"<<url<<"\n";
            });
        });


        m_magic_executor.add_callback("ls",[&](auto & app){
            std::string dir;
            app().add_option("-d,--dir",dir, "a directory")->required();
            app.parse([&](){
                ls(dir);
            });
        });

        m_magic_executor.add_callback("mkdir",[&](auto & app){
            std::string dir;
            app().add_option("-d,--dir",dir, "a directory")->required();
            app.parse([&](){
                std::filesystem::create_directory(dir);
            });
        });

        #ifdef XEUS_UTILS_EMSCRIPTEN_WASM_BUILD
        m_magic_executor.add_callback("fetch",[&](auto & app){
            std::string url, filename;
            app().add_option("-u,--url",url, "an url")->required();
            app().add_option("-f,--filename",filename, "an filename")->required();
            app.parse([&](){
                fetch(url, filename);
            });
        });
        m_magic_executor.add_callback("untar",[&](auto & app){
            std::string filename, dirname;
            app().add_option("-f,--filename",filename, "an filename")->required();
            app().add_option("-d,--dirname",dirname, "an dirname")->required();
            app.parse([&](){
                untar_file(filename, dirname);
            });
        });
        #endif
    }

    nl::json interpreter::execute_request_impl(int execution_counter, // Typically the cell number
                                                      const  std::string & code, // Code to execute
                                                      bool /*silent*/,
                                                      bool /*store_history*/,
                                                      nl::json /*user_expressions*/,
                                                      bool /*allow_stdin*/)
    {
        nl::json kernel_res;
        try{
            auto magic_state = m_magic_executor.dispatch(code);
        }
        catch (const std::exception& e) { 
            publish_execution_error("error","Error", std::vector<std::string>(1, std::string(e.what())));
        }
        // #ifdef XEUS_UTILS_EMSCRIPTEN_WASM_BUILD
        // using namespace std;
        // using namespace boost;
        // string s = code;
        // string separator1("");//dont let quoted arguments escape themselves
        // string separator2(" ");//split on spaces
        // string separator3("\"\'");//let it have quoted arguments

        // escaped_list_separator<char> els(separator1,separator2,separator3);
        // tokenizer<escaped_list_separator<char>> tok(s, els);

        // for(tokenizer<escaped_list_separator<char>>::iterator beg=tok.begin(); beg!=tok.end();++beg)
        // {
        //     cout << *beg << "\n";
        //     publish_stream("stdout", *beg+std::string("\n"));
        // }
        // #endif

        // if (code.compare("hello, world") == 0)
        // {
        //     publish_stream("stdout", code);
        // }

        // if (code.compare("error") == 0)
        // {
        //     publish_stream("stderr", code);
        // }

        // if (code.compare("?") == 0)
        // {
        //     std::string html_content = R"(<iframe class="xpyt-iframe-pager" src="
        //         https://xeus.readthedocs.io"></iframe>)";

        //     auto payload = nl::json::array();
        
        //     payload = nl::json::array();
        //     payload[0] = nl::json::object({
        //         {"data", {
        //             {"text/plain", "https://xeus.readthedocs.io"},
        //             {"text/html", html_content}}
        //         },
        //         {"source", "page"},
        //         {"start", 0}
        //     });

        //     return xeus::create_successful_reply(payload);
        // }

        nl::json pub_data;
        // pub_data["text/plain"] = code;

        publish_execution_result(execution_counter, 
            std::move(pub_data),
            nl::json::object()
        );

        return xeus::create_successful_reply();
    }

    void interpreter::configure_impl()
    {
        // Perform some operations
    }

    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        if (code.compare("incomplete") == 0)
        {
            return xeus::create_is_complete_reply("incomplete"/*status*/, "   "/*indent*/);
        }
        else if(code.compare("invalid") == 0)
        {
            return xeus::create_is_complete_reply("invalid"/*status*/);
        }
        else
        {
            return xeus::create_is_complete_reply("complete"/*status*/);
        }   
    }

    nl::json interpreter::complete_request_impl(const std::string&  code,
                                                     int cursor_pos)
    {
        // Code starts with 'H', it could be the following completion
        if (code[0] == 'H')
        {
       
            return xeus::create_complete_reply(
                {
                    std::string("Hello"), 
                    std::string("Hey"), 
                    std::string("Howdy")
                },          /*matches*/
                5,          /*cursor_start*/
                cursor_pos  /*cursor_end*/
            );
        }

        // No completion result
        else
        {

            return xeus::create_complete_reply(
                nl::json::array(),  /*matches*/
                cursor_pos,         /*cursor_start*/
                cursor_pos          /*cursor_end*/
            );
        }
    }

    nl::json interpreter::inspect_request_impl(const std::string& /*code*/,
                                                      int /*cursor_pos*/,
                                                      int /*detail_level*/)
    {
        
        return xeus::create_inspect_reply(true/*found*/, 
            {{std::string("text/plain"), std::string("hello!")}}, /*data*/
            {{std::string("text/plain"), std::string("hello!")}}  /*meta-data*/
        );
         
    }

    void interpreter::shutdown_request_impl() {
        std::cout << "Bye!!" << std::endl;
    }

    nl::json interpreter::kernel_info_request_impl()
    {

        const std::string  protocol_version = "5.3";
        const std::string  implementation = "xutils";
        const std::string  implementation_version = XEUS_UTILS_VERSION;
        const std::string  language_name = "utils";
        const std::string  language_version = "1.0.0";
        const std::string  language_mimetype = "text/x-utilssrc";;
        const std::string  language_file_extension = "utils";;
        const std::string  language_pygments_lexer = "";
        const std::string  language_codemirror_mode = "";
        const std::string  language_nbconvert_exporter = "";
        const std::string  banner = "xutils";const bool         debugger = false;
        
        const nl::json     help_links = nl::json::array();


        return xeus::create_info_reply(
            protocol_version,
            implementation,
            implementation_version,
            language_name,
            language_version,
            language_mimetype,
            language_file_extension,
            language_pygments_lexer,
            language_codemirror_mode,
            language_nbconvert_exporter,
            banner,
            debugger,
            help_links
        );
    }

}
