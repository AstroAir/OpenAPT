/*
 * indiexception.hpp
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

Date: 2023-4-9

Description: INDI Exception

**************************************************/

#include <exception>
#include <string>

class ConnectionError : public std::exception
{
public:
    ConnectionError(const std::string &error_message) : error_message_(error_message) {}

    virtual const char *what() const noexcept override
    {
        return error_message_.c_str();
    }

private:
    std::string error_message_;
};

class ConnectionTimeout : public std::exception
{
public:
    ConnectionTimeout(const std::string &error_message) : error_message_(error_message) {}

    virtual const char *what() const noexcept override
    {
        return error_message_.c_str();
    }

private:
    std::string error_message_;
};

class CameraError : public std::exception
{
public:
    CameraError(const std::string &error_message) : error_message_(error_message) {}

    virtual const char *what() const noexcept override
    {
        return error_message_.c_str();
    }

private:
    std::string error_message_;
};

class TelescopeError : public std::exception
{
public:
    TelescopeError(const std::string &error_message) : error_message_(error_message) {}

    virtual const char *what() const noexcept override
    {
        return error_message_.c_str();
    }

private:
    std::string error_message_;
};
