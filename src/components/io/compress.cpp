/*
 * compress.cpp
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

Date: 2023-3-31

Description: Compressor using ZLib

**************************************************/

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <string>
#include <zlib.h>

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

constexpr int CHUNK = 16384;

namespace OpenAPT::File
{

    bool compress_file(const std::string &file_name, const std::string &output_folder)
    {
        std::filesystem::path input_path(file_name);
        if (!std::filesystem::exists(input_path))
        {
            spdlog::error("Input file {} does not exist.", file_name);
            return false;
        }

        std::filesystem::path output_path = std::filesystem::path(output_folder) / input_path.filename().concat(".gz");
        gzFile out = gzopen(output_path.string().c_str(), "wb");
        if (!out)
        {
            spdlog::error("Failed to create compressed file {}", output_path.string());
            return false;
        }

        std::ifstream in(file_name, std::ios::binary);
        if (!in)
        {
            spdlog::error("Failed to open input file {}", file_name);
            gzclose(out);
            return false;
        }

        char buf[CHUNK];
        while (!in.eof())
        {
            in.read(buf, CHUNK);
            int bytesRead = static_cast<int>(in.gcount());

            if (gzwrite(out, buf, bytesRead) != bytesRead)
            {
                spdlog::error("Failed to compress file {}", file_name);
                in.close();
                gzclose(out);
                return false;
            }
        }

        in.close();
        gzclose(out);
        spdlog::info("Compressed file {} -> {}", file_name, output_path.string());
        return true;
    }

    bool decompress_file(const std::string &file_name, const std::string &output_folder)
    {
        std::filesystem::path input_path(file_name);
        if (!std::filesystem::exists(input_path))
        {
            spdlog::error("Input file {} does not exist.", file_name);
            return false;
        }

        std::filesystem::path output_path = std::filesystem::path(output_folder) / input_path.filename().stem().concat(".out");
        FILE *out = fopen(output_path.string().c_str(), "wb");
        if (!out)
        {
            spdlog::error("Failed to create decompressed file {}", output_path.string());
            return false;
        }

        gzFile in = gzopen(file_name.c_str(), "rb");
        if (!in)
        {
            spdlog::error("Failed to open compressed file {}", file_name);
            fclose(out);
            return false;
        }

        char buf[CHUNK];
        int bytesRead;
        while ((bytesRead = gzread(in, buf, CHUNK)) > 0)
        {
            if (fwrite(buf, 1, bytesRead, out) != static_cast<size_t>(bytesRead))
            {
                spdlog::error("Failed to decompress file {}", file_name);
                fclose(out);
                gzclose(in);
                return false;
            }
        }

        fclose(out);
        gzclose(in);
        spdlog::info("Decompressed file {} -> {}", file_name, output_path.string());
        return true;
    }

    bool compress_folder(const char *folder_name)
    {
        // Size of the read/write buffer
        char outfile_name[256];
        sprintf(outfile_name, "%s.gz", folder_name);
        gzFile out = gzopen(outfile_name, "wb");
        if (!out)
        {
            spdlog::error("Failed to create compressed file {}", outfile_name);
            return false;
        }
#ifdef _WIN32
        HANDLE hFind;
        // File handle
        WIN32_FIND_DATAA findData;
        char searchPath[256];
        sprintf(searchPath, "%s\\*", folder_name);
        hFind = FindFirstFileA(searchPath, &findData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            spdlog::error("Failed to open folder {}", folder_name);
            gzclose(out);
            return false;
        }
        do
        {
            // Ignore "." and ".." directories
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
                continue;
            // Construct the file path
            char file_name[256];
            sprintf(file_name, "%s%c%s", folder_name, PATH_SEPARATOR, findData.cFileName);
            // If it's a directory, recursively call this function
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                bool res = compress_folder(file_name);
                if (!res)
                {
                    FindClose(hFind);
                    gzclose(out);
                    return false;
                }
                continue;
            }
            // Otherwise, it's a file, compress it
            if (access(file_name, F_OK) == -1)
            {
                // If the file doesn't exist
                continue;
            }
            FILE *in = fopen(file_name, "rb");
            if (!in)
            {
                spdlog::warn("Failed to open file {}", file_name);
                continue;
            }
            char buf[CHUNK];
            int len;
            while ((len = fread(buf, 1, CHUNK, in)) > 0)
            {
                if (gzwrite(out, buf, len) != len)
                {
                    fclose(in);
                    gzclose(out);
                    spdlog::error("Failed to compress file {}", file_name);
                    return false;
                }
            }
            fclose(in);
            spdlog::info("Compressed file {}", file_name);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
#else
        DIR *dir;
        struct dirent *entry;
        dir = opendir(folder_name);
        if (!dir)
        {
            spdlog::error("Failed to open folder {}", folder_name);
            gzclose(out);
            return false;
        }
        while ((entry = readdir(dir)) != NULL)
        {
            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            // Construct the file path
            char file_name[512];
            int ret = snprintf(file_name, sizeof(file_name), "%s/%s", folder_name, entry->d_name);
            if (ret < 0 || ret >= sizeof(file_name))
            {
                // Check for truncation or other errors in snprintf()
                spdlog::error("Failed to compress file {} because the output was truncated or an error occurred in snprintf()", entry->d_name);
                closedir(dir);
                gzclose(out);
                return false;
            }
            // If it's a directory, recursively call this function
            struct stat st;
            if (stat(file_name, &st) == 0 && S_ISDIR(st.st_mode))
            {
                bool res = compress_folder(file_name);
                if (!res)
                {
                    closedir(dir);
                    gzclose(out);
                    return false;
                }
                continue;
            }
            // Otherwise, it's a file, compress it
            if (access(file_name, F_OK) == -1)
            {
                // If the file doesn't exist
                continue;
            }
            FILE *in = fopen(file_name, "rb");
            if (!in)
            {
                spdlog::warn("Failed to open file {}", file_name);
                continue;
            }
            char buf[CHUNK];
            int len;
            while ((len = fread(buf, 1, CHUNK, in)) > 0)
            {
                if (gzwrite(out, buf, len) != len)
                {
                    fclose(in);
                    gzclose(out);
                    spdlog::error("Failed to compress file {}", file_name);
                    return false;
                }
            }
            fclose(in);
            spdlog::info("Compressed file {}", file_name);
        }
        closedir(dir);
#endif
        gzclose(out);
        spdlog::info("Compressed folder {} -> {}", folder_name, outfile_name);
        return true;
    }

    bool extract_zip(const std::string &zip_file, const std::string &destination_folder)
    {
        std::ifstream file(zip_file, std::ios::binary);
        if (!file)
        {
            spdlog::error("Failed to open ZIP file: {}", zip_file);
            return false;
        }

        std::vector<char> buffer(8192);

        z_stream stream{};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = 0;
        stream.next_in = Z_NULL;

        int ret = inflateInit2(&stream, 15 + 16); // 解压缩方法使用 gzip 格式
        if (ret != Z_OK)
        {
            spdlog::error("Failed to initialize zlib: {}", ret);
            return false;
        }

        while (true)
        {
            file.read(buffer.data(), buffer.size());
            stream.avail_in = static_cast<uInt>(file.gcount());
            stream.next_in = reinterpret_cast<Bytef *>(buffer.data());

            if (stream.avail_in == 0)
                break;

            do
            {
                std::vector<char> output_buffer(8192);

                stream.avail_out = static_cast<uInt>(output_buffer.size());
                stream.next_out = reinterpret_cast<Bytef *>(output_buffer.data());

                ret = inflate(&stream, Z_NO_FLUSH);
                if (ret == Z_STREAM_ERROR)
                {
                    spdlog::error("Failed to inflate data: {}", ret);
                    inflateEnd(&stream);
                    return false;
                }

                std::size_t output_size = output_buffer.size() - stream.avail_out;
                if (output_size > 0)
                {
                    std::string entry_path = destination_folder + "/" + std::to_string(stream.total_in) + ".txt";
                    std::ofstream output_file(entry_path, std::ios::binary | std::ios::app);
                    output_file.write(output_buffer.data(), output_size);
                    output_file.close();
                }
            } while (stream.avail_out == 0);
        }

        inflateEnd(&stream);
        return true;
    }

    bool create_zip(const std::string &source_folder, const std::string &zip_file)
    {
        std::ofstream file(zip_file, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            spdlog::error("Failed to create ZIP file: {}", zip_file);
            return false;
        }

        std::vector<char> buffer(8192);

        z_stream stream{};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        int ret = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY); // 压缩方法使用 gzip 格式
        if (ret != Z_OK)
        {
            spdlog::error("Failed to initialize zlib: {}", ret);
            return false;
        }

        std::vector<std::string> file_paths;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(source_folder))
        {
            if (std::filesystem::is_regular_file(entry))
            {
                file_paths.push_back(entry.path().string());
            }
        }

        for (const auto &file_path : file_paths)
        {
            std::ifstream input_file(file_path, std::ios::binary);
            if (!input_file)
            {
                spdlog::error("Failed to open file: {}", file_path);
                continue;
            }

            while (input_file.good())
            {
                input_file.read(buffer.data(), buffer.size());
                stream.avail_in = static_cast<uInt>(input_file.gcount());
                stream.next_in = reinterpret_cast<Bytef *>(buffer.data());

                do
                {
                    std::vector<char> output_buffer(8192);

                    stream.avail_out = static_cast<uInt>(output_buffer.size());
                    stream.next_out = reinterpret_cast<Bytef *>(output_buffer.data());

                    ret = deflate(&stream, Z_FINISH);
                    if (ret == Z_STREAM_ERROR)
                    {
                        spdlog::error("Failed to deflate data: {}", ret);
                        deflateEnd(&stream);
                        return false;
                    }

                    std::size_t output_size = output_buffer.size() - stream.avail_out;
                    if (output_size > 0)
                    {
                        file.write(output_buffer.data(), output_size);
                    }
                } while (stream.avail_out == 0);
            }

            input_file.close();
        }

        deflateEnd(&stream);
        return true;
    }

}
