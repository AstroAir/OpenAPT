/*
 * modloader.h
 * 
 * Copyright (C) 2023 Max Qian <lightapt.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/************************************************* 
 
Copyright: 2023 Max Qian. All rights reserved
 
Author: Max Qian

E-mail: astro_air@126.com
 
Date: 2023-3-29
 
Description: C++ and Python Modules Loader
 
**************************************************/

#ifndef _MOD_LOADER_H_
#define _MOD_LOADER_H_

#include "openapt.hpp"

#include <Python.h>

#include <vector>
#include <unordered_map>
#include <cstdio>

#if defined(WIN32)
    // Windows平台
    #define MODULE_HANDLE HMODULE
    #define LOAD_LIBRARY(p) LoadLibrary(p)
    #include <windows.h>
#elif defined(__APPLE__)
    // macOS平台
    #define MODULE_HANDLE void*
    #define LOAD_LIBRARY(p) dlopen_ext(p, RTLD_NOW, (const char*[]){ "-undefined", "dynamic_lookup", NULL })
    #include <dlfcn.h>
#else
    // Linux和其他类UNIX平台
    #define MODULE_HANDLE void*
    #define LOAD_LIBRARY(p) dlopen(p, RTLD_NOW | RTLD_GLOBAL)
    #define UNLOAD_LIBRARY(p) dlclose(p)
    #include <dlfcn.h>
    #include <dirent.h>
    #include <unistd.h>
#endif

#include <spdlog/spdlog.h>

#include "plugins/thread.hpp"
#include "nlohmann/json.hpp"

namespace OpenAPT{

    nlohmann::json iterator_modules_dir();
    
    class ModuleLoader {
        public:
            ModuleLoader();
            ~ModuleLoader();
            bool LoadModule(const std::string& path, const std::string& name);
            bool UnloadModule(const std::string& filename);
            bool LoadBinary(const char *dir_path, const char *out_path, const char *build_path, const char *lib_name);
            bool LoadPythonScript(const std::string& scriptName);
            void UnloadPythonScript(const std::string& scriptName);

            template<typename T>
            T GetFunction(const std::string& module_name, const std::string& function_name);
            /// @brief 从已加载的模块中加载函数并运行
            /// @tparam T 函数类型
            /// @tparam ...Args 函数所需参数 
            /// @tparam class_type 类类型
            /// @param module_name 模块名称
            /// @param func_name 函数名称
            /// @param thread_name 线程名称
            /// @param instance 类实例
            /// @param ...args 
            /// @return 函数是否启动成功
            template<typename T, typename class_type, typename... Args>
            typename std::enable_if<std::is_class<class_type>::value, bool>::type
            LoadAndRunFunction(const std::string& module_name, const std::string& func_name,
                                const std::string& thread_name, class_type* instance, Args... args) {
                typedef T (class_type::*MemberFunctionPtr)(Args...);
                typedef T (*FunctionPtr)(Args...);
                void* handle = GetHandle(module_name);
                auto sym_ptr = dlsym(handle, func_name.c_str());
                if (!sym_ptr) {
                    spdlog::error("Failed to load symbol {}: {}", func_name, dlerror());
                    return false;
                }
                FunctionPtr func_ptr = reinterpret_cast<FunctionPtr>(sym_ptr);
                MemberFunctionPtr member_func_ptr = nullptr;
                if constexpr (std::is_member_function_pointer_v<MemberFunctionPtr>) {
                    member_func_ptr = reinterpret_cast<MemberFunctionPtr>(sym_ptr);
                }
                m_ThreadManager.addThread(std::bind(member_func_ptr, instance, args...), std::bind(func_ptr, args...), thread_name);
                return true;
            }

            /**
             * @brief 从动态链接库中加载函数并执行，支持指定函数返回值类型和传入参数类型
             * 
             * @tparam T 函数返回值的类型
             * @tparam Args 函数传入参数的类型，可以是多个
             * @param module_name 要加载的动态链接库的名称，不含后缀
             * @param func_name 要加载的函数的名称
             * @param thread_name 新开线程的名称
             * @param args 要传入函数的参数列表
             * @return T 执行函数的返回值
             */
            template<typename T, typename... Args>
            T LoadAndRunFunction(const std::string& module_name, const std::string& func_name,
                                const std::string& thread_name, Args&&... args) {
                // 定义函数指针类型
                typedef T (*FunctionPtr)(Args...);

                // 获取动态链接库句柄
                void* handle = GetHandle(module_name);

                // 加载函数
                auto sym_ptr = dlsym(handle, func_name.c_str());
                if (!sym_ptr) {
                    spdlog::error("Failed to load symbol {}: {}", func_name, dlerror());
                }
                FunctionPtr func_ptr = reinterpret_cast<FunctionPtr>(sym_ptr);

                // 新建线程并执行函数
                m_ThreadManager.addThread(std::bind(func_ptr, std::forward<Args>(args)...), thread_name);

                // 返回函数返回值
                return static_cast<T>(0);
            }

            std::vector<std::string> getPythonFunctions(const std::string& scriptName);

            template<typename... Args>
            bool runPythonFunction(const std::string& scriptName, const std::string& functionName, Args... args);

            template<typename F, typename... Args>
            void AysncRunPythonFunction(const std::string& scriptName, const std::string& functionName, ModuleLoader& scriptLoader, F&& callback, Args&&... args);

        public:
            void* GetHandle(const std::string& name) const {
                auto it = handles_.find(name);
                if (it == handles_.end()) {
                    return nullptr;
                }
                return it->second;
            }
            
        private:
            std::unordered_map<std::string, void*> handles_;

            std::unordered_map<std::string, PyObject*> python_modules_;
    };
}

#endif

/*
Example:

    loader.loadLuaModule("module1", "module1.lua");
    loader.loadLuaModule("module2", "module2.lua");
    const auto& functionList1 = loader.getFunctionList("module1");
    const auto& functionList2 = loader.getFunctionList("module2");
    std::cout << "Functions in module1:" << std::endl;
    for (const auto& functionName : functionList1) {
        std::cout << "  " << functionName << std::endl;
    }
    std::cout << "Functions in module2:" << std::endl;
    for (const auto& functionName : functionList2) {
        std::cout << "  " << functionName << std::endl;
    }

    chai

    if (loader.load_script("myscript.chai")) {
        // 获取函数和变量列表
        const std::unordered_map<std::string, std::variant<chaiscript::Type_Info, chaiscript::Boxed_Value>>& chai_functions_ = loader.get_functions_and_variables();
        // 遍历函数和变量列表，输出每个函数和变量的名称和类型
        for (const auto& item : chai_functions_) {
            std::ostringstream oss;
            oss << "Name: " << item.first << ", Type: ";
            if (item.second.index() == 0) {
                chaiscript::Type_Info info = std::get<0>(item.second);
                oss << "Function, Return Type: " << info.bare_name();
            } else if (item.second.index() == 1) {
                chaiscript::Boxed_Value value = std::get<1>(item.second);
                oss << "Variable, Type: " << value.get_type_info().bare_name();
            }
            logger->info(oss.str());
        }
    }
*/