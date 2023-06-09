/*
 * compiler.cpp
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

Description: Compiler

**************************************************/

#include "compiler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <atomic>

using json = nlohmann::json;
namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
#define COMPILER "cl.exe"
#define CMD_PREFIX ""
#define CMD_SUFFIX ".dll"
#else
#include <cstdio>
#define COMPILER "g++"
#define CMD_PREFIX "lib"
#define CMD_SUFFIX ".so"
#endif

bool Compiler::CompileToSharedLibrary(const std::string &code, const std::string &moduleName, const std::string &functionName)
{
    spdlog::debug("Compiling module {}::{}...", moduleName, functionName);

    // 参数校验
    if (code.empty() || moduleName.empty() || functionName.empty())
    {
        spdlog::error("Invalid parameters.");
        return false;
    }

    // Check if the module is already compiled and cached
    auto cachedResult = cache_.find(moduleName + "::" + functionName);
    if (cachedResult != cache_.end())
    {
        spdlog::warn("Module {}::{} is already compiled, returning cached result.", moduleName, functionName);
        return true;
    }

    // Create output directory if it does not exist
    const std::string outputDir = "modules/global/";
    if (!fs::exists(outputDir))
    {
        spdlog::warn("Output directory does not exist, creating it: {}", outputDir);
        try
        {
            fs::create_directories(outputDir);
        }
        catch (const std::exception &e)
        {
            spdlog::error("Failed to create output directory: {}", e.what());
            return false;
        }
    }

    // Read compile options from JSON file
    std::string compileOptions = "-shared -fPIC -x c++ ";
    std::ifstream compileOptionFile("compile_options.json");
    if (compileOptionFile.is_open())
    {
        json compileOptionsJson;
        try
        {
            compileOptionFile >> compileOptionsJson;
            if (compileOptionsJson.contains("optimization_level") && compileOptionsJson.contains("cplus_version") && compileOptionsJson.contains("warnings"))
            {
                compileOptions = compileOptionsJson["optimization_level"].get<std::string>() + " " +
                                 compileOptionsJson["cplus_version"].get<std::string>() + " " +
                                 compileOptionsJson["warnings"].get<std::string>() + " ";
            }
            else
            {
                spdlog::error("Invalid format in compile_options.json.");
                return false;
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error reading compile_options.json: {}", e.what());
            return false;
        }
    }

    // Specify output file path
    std::string output = outputDir + moduleName + CMD_SUFFIX;

    // Syntax and semantic checking
    std::stringstream syntaxCheckCmd;
    syntaxCheckCmd << COMPILER << " -fsyntax-only -x c++ -";
    std::ostringstream syntaxCheckOutput;
    if (RunShellCommand(syntaxCheckCmd.str(), code, syntaxCheckOutput) != 0)
    {
        spdlog::error("Syntax error in C++ code: {}", syntaxCheckOutput.str());
        return false;
    }

    // Compile code
    std::ostringstream compilationOutput;
    std::string cmd = std::string(COMPILER) + " " + compileOptions + " - " + " -o " + output;
    spdlog::debug("{}", cmd);

    int exitCode = RunShellCommand(cmd, code, compilationOutput);
    if (exitCode != 0)
    {
        spdlog::error("Failed to compile C++ code: {}", compilationOutput.str());
        return false;
    }

    // Cache compiled module
    cache_[moduleName + "::" + functionName] = output;

    /*
    // Load the compiled module
    if(m_App.GetModuleLoader()->LoadModule(output, moduleName)) {
        spdlog::info("Module {}::{} compiled successfully.", moduleName, functionName);
        return true;
    } else {
        spdlog::error("Failed to load the compiled module: {}", output);
        return false;
    }
    */
    return false;
}

bool Compiler::CopyFile(const std::string &source, const std::string &destination)
{
    std::ifstream src(source, std::ios::binary);
    if (!src)
    {
        spdlog::error("Failed to open file for copy: {}", source);
        return false;
    }

    std::ofstream dst(destination, std::ios::binary);
    if (!dst)
    {
        spdlog::error("Failed to create file for copy: {}", destination);
        return false;
    }

    dst << src.rdbuf();
    return true;
}

int Compiler::RunShellCommand(const std::string &command, const std::string &input, std::string &output)
{
    int exitCode = -1;
#ifdef _WIN32
    HANDLE hStdoutRead;

    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    HANDLE hStdinRead, hStdoutWrite;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hStdinRead, &hStdoutWrite, &sa, 0))
    {
        spdlog::error("Failed to create input pipe for shell command: {}", command);
        return exitCode;
    }
    if (!SetHandleInformation(hStdoutWrite, HANDLE_FLAG_INHERIT, 0))
    {
        spdlog::error("Failed to set input handle information for shell command: {}", command);
        return exitCode;
    }
    if (!SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0))
    {
        spdlog::error("Failed to set output handle information for shell command: {}", command);
        return exitCode;
    }
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = hStdinRead;
    si.hStdOutput = hStdoutWrite;
    si.hStdError = hStdoutWrite;
    if (!CreateProcess(NULL, &command[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        spdlog::error("Failed to launch shell command: {}", command);
        CloseHandle(hStdinRead);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStdoutRead);
        CloseHandle(hStdoutWrite);
        return exitCode;
    }
    CloseHandle(hStdinRead);
    CloseHandle(hStdoutWrite);

    // Read the command output
    std::thread outputThread([&]()
                             {
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(hStdoutRead, buffer, sizeof(buffer), &bytesRead, NULL))
        {
            output.append(buffer, bytesRead);
        } });

    // Write the command input
    DWORD bytesWritten;
    if (!WriteFile(hStdinWrite, input.c_str(), input.size(), &bytesWritten, NULL))
    {
        spdlog::error("Failed to write input for shell command: {}", command);
        return exitCode;
    }
    CloseHandle(hStdinWrite);

    // Wait for the command to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&exitCode);

    // Clean up
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRead);
    outputThread.join();

#else
    FILE *pipe = popen(command.c_str(), "w");
    if (!pipe)
    {
        spdlog::error("Failed to popen shell command: {}", command);
        return exitCode;
    }

    fwrite(input.c_str(), 1, input.size(), pipe);
    fclose(pipe);

    exitCode = WEXITSTATUS(pclose(pipe));
#endif

    return exitCode;
}
