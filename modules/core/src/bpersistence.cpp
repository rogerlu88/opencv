// Copyright (c) 2016 Andrés Solís Montero <http://www.solism.ca>, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//


#include "precomp.hpp"

cv::BFileStorage::BFileStorage(const std::string &filename, int flags): _mode(flags)
{
    if (flags == BFileStorage::READ)
    {
        _fin.open(filename, std::ios::in | std::ios::binary);
        _status = _fin.is_open();
    }
    if (flags == BFileStorage::WRITE)
    {
        _fout.open(filename, std::ios::out | std::ios::binary);
        _status = _fout.is_open();
    }
}
cv::BFileStorage::~BFileStorage()
{
    release();
}
bool cv::BFileStorage::isOpened()
{
    return _status;
}
void cv::BFileStorage::release()
{
    if (!isOpened())
        return;
    if (_mode == BFileStorage::READ)
        _fin.close();
    else if (_mode == BFileStorage::WRITE)
        _fout.close();
}
