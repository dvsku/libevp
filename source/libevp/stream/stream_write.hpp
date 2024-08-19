#pragma once

#include "libevp/type_traits.hpp"

#include <string>
#include <memory>
#include <fstream>
#include <filesystem>

namespace libevp {
    class fstream_write {
    public:
        fstream_write()                     = delete;
        fstream_write(const fstream_write&) = delete;
        fstream_write(fstream_write&&)      = default;

        fstream_write(const std::filesystem::path& file) {
            m_stream = std::make_unique<std::ofstream>(file, std::ios::binary);
            if (!m_stream || !m_stream->is_open()) {
                m_stream = nullptr;
                return;
            }
        }

        fstream_write& operator=(const fstream_write&) = delete;
        fstream_write& operator=(fstream_write&&)      = default;

    public:
        size_t pos() const {
            return (size_t)m_stream->tellp();
        }

        bool is_valid() const {
            return m_stream && m_stream->is_open();
        }

        void seek(size_t offset, std::ios_base::seekdir dir = std::ios_base::cur) {
            m_stream->seekp(offset, dir);
        }

        template<typename T>
        requires arithmetic<T> || is_enum<T>
        void write(const T value) {
            internal_write(&value, sizeof(T));
        }

        void write(const std::string& str) {
            uint32_t size = (uint32_t)str.size();

            write(size);
            internal_write(str.data(), size);
        }

        void write(uint8_t* src, uint32_t size) {
            internal_write(src, size);
        }

    private:
        std::unique_ptr<std::ofstream> m_stream;

    private:
        void internal_write(const void* src, uint32_t size) {
            m_stream->write((const char*)src, (size_t)size);

            if (!(*m_stream))
                throw std::runtime_error("Failed to write requested size.");
        }
    };
}
