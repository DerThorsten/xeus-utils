/***************************************************************************
* Copyright (c) 2022, Thorsten Beier                                       *                                                       *
* Copyright (c) 2022, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/


// tools for the jupyterlite kernels
#ifndef XUTILS_XFETCH_HPP
#define XUTILS_XFETCH_HPP

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>

#include <iostream>
#include <fstream>  
#include <sstream>

#include "xeus/xinterpreter.hpp"

// this might end up in a dedicated library at some point
namespace xeus_utils
{
    namespace detail
    {
        struct fetch_user_data
        {
            std::string filename;
            bool done;
        };
    }

    // download a file from url and store in fileystem
    // progress is reported to jupyters `stdout` stream 
    void fetch(const std::string url, const std::string filename)
    {
        std::stringstream ss;
        ss<<"Start downloading from URL "<<url<<"\n";
        auto & interpreter = xeus::get_interpreter();
        interpreter.publish_stream("stdout", ss.str());

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;\
        auto  userData = new detail::fetch_user_data{
            std::string(filename),
            false
        };
        attr.userData = userData;

        attr.onsuccess = [](emscripten_fetch_t *fetch){
            auto & interpreter = xeus::get_interpreter();

            {
                std::stringstream s;
                s<<"Finished downloading "<<fetch->numBytes<<" bytes from URL "<<fetch->url<<"\n";
                interpreter.publish_stream("stdout", s.str());
            }

            auto userData = reinterpret_cast<detail::fetch_user_data *>(fetch->userData);
            auto filename = userData->filename;
            std::ofstream myFile;
            {
                std::stringstream s;
                s<<"Writing downloaded data to file "<<filename<<"\n";
                interpreter.publish_stream("stdout", s.str());
            }

            myFile.open(filename, std::ios::out | std::ios::binary);
            myFile.write (fetch->data, fetch->numBytes);
            myFile.close();
            interpreter.publish_stream("stdout","Finished writing file\n");
            userData->done = true;
            emscripten_fetch_close(fetch);
        };
        attr.onerror = [](emscripten_fetch_t *fetch){
            std::stringstream s;
            s<<"Downloading "<<fetch->url<<"failed , HTTP failure status code:"<<fetch->status<<"\n";
            auto & interpreter = xeus::get_interpreter();
            std::cout<<s.str()<<"\n";
            interpreter.publish_stream("stdout", s.str());
            auto userData = reinterpret_cast<detail::fetch_user_data *>(fetch->userData);
            userData->done = true;
            emscripten_fetch_close(fetch);
        };
        attr.onprogress = [](emscripten_fetch_t *fetch){
            std::stringstream s;
            auto & interpreter = xeus::get_interpreter();

            if (fetch->totalBytes) {
                s<<"Downloading "<<fetch->url<<" "<<std::setprecision(2)<<fetch->dataOffset * 100.0 / fetch->totalBytes<<"% complete\n";
            } else 
            {
                s<<"Downloading "<<fetch->url<<" "<<fetch->dataOffset + fetch->numBytes<<"bytes complete\n";
            }
            std::cout<<s.str()<<"\n";
            interpreter.publish_stream("stdout", s.str());
        };
        emscripten_fetch(&attr, url.c_str());

        while(!userData->done)
        {
            emscripten_sleep(100);
        }
        delete userData;
    }

#if 0
    // make a directory an IDBFS filesystem
    EM_JS(void, async_ems_init_idbfs,  (const char* path), {
        return Asyncify.handleSleep(function (wakeUp) {
            var jpath = UTF8ToString(path);
            FS.mkdir( jpath); 
            FS.mount(IDBFS,{}, jpath);
            FS.syncfs(true, function(err) {
                assert(!err);
                wakeUp();
            });
        });
    });

    // write / flush files from in mem filesystem to IDB
    EM_JS(void, async_ems_sync_db,  (), {
        return Asyncify.handleSleep(function (wakeUp) {;
            FS.syncfs(false, function(err) {
                assert(!err);
                wakeUp();
            });
        });
    });



    EM_JS(emscripten::EM_VAL, async_open_from_indexed_db,  (const char* db_name, const char* key), {
        return Asyncify.handleSleep(function (wakeUp) {
            var jdb_name = UTF8ToString(db_name);
            var jkey = UTF8ToString(key);
            console.log("jdb_name",jdb_name,"jkey",jkey);
            var IDBOpenDBRequest = self.indexedDB.open(jdb_name);
            IDBOpenDBRequest.onerror = function(event) {
                console.log("IDBOpenDBRequest.onerror");
                wakeUp(Emval.toHandle("error1"));
            };
            IDBOpenDBRequest.onsuccess = function(event) {
                console.log("IDBOpenDBRequest.onsuccess");
                var db = IDBOpenDBRequest.result;
                console.log("IDBTransaction");
                var IDBTransaction = db.transaction("files", "readonly");
                console.log("IDBObjectStore");
                var IDBObjectStore = IDBTransaction.objectStore("files");
                console.log("IDBRequest");
                var IDBRequest = IDBObjectStore.get(jkey, "key");
                IDBRequest.onerror = function(event) {
                    console.log("IDBRequest.onerror");
                    wakeUp(Emval.toHandle("error"));
                };
                IDBRequest.onsuccess = function(event) {
                    console.log("IDBRequest.onsuccess");
                    var res = IDBRequest.result;
                    console.log("res",  res);
                    wakeUp(Emval.toHandle(res));
                };
            };
        });
    });


    // make a directory an IDBFS filesystem
    void ems_init_idbfs(const std::string & path)
    {
        async_ems_init_idbfs(path.c_str());
    }

    // write / flush files from in mem filesystem to IDB
    void ems_sync_db()
    {   
        async_ems_sync_db();
    }


    // load from an indexed
    void open_from_indexed_db2(const std::string & db_name2, const std::string & db_key, const std::string & path)
    {

        std::cout<<"db_name2: "<<db_name2<<"\n";
        std::cout<<"db_key: "<<db_key<<"\n";
        std::cout<<"path: "<<path<<"\n";

        std::cout<<"db_name2 "<<db_name2<<" db_key "<<db_key<<" path "<<path<<"\n";
        auto & interpreter = xeus::get_interpreter();
        

        std::cout<<"db_name2 "<<db_name2<<" db_key "<<db_key<<" path "<<path<<"\n";
        emscripten::val db_res =  emscripten::val::take_ownership(async_open_from_indexed_db(db_name2.c_str(), db_key.c_str()));

        const std::string db_res_type = db_res.typeOf().as<std::string>();
        if(db_res_type == "undefined")
        {
            interpreter.publish_execution_error("error","Error", std::vector<std::string>(1, std::string("cannot key in db")));
            return;
        }
        else
        {
           const std::string  content_format =  db_res["format"].as<std::string>();
           const std::string  content_type =  db_res["content"].typeOf().as<std::string>();
           std::cout<<"content_format "<<content_format<<" content_type "<<content_type<<"\n";
           if(content_format == "text" && content_type == "string")
           {
                const std::string content_as_string = db_res["content"].as<std::string>();
                std::cout<<"content_as_string "<<content_as_string<<"\n";
                interpreter.publish_stream("stdout", content_as_string);
           }
        }
        std::cout<<"db_res_type: "<<db_res_type<<"\n";
    }
#endif
}
#endif // XUTILS_XFETCH_HPP
