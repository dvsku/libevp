#pragma once

#include "libevp/type_traits.hpp"

#include <string>
#include <cstring>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace libevp {
    class fstream_read {
    public:
        fstream_read()                   = delete;
        fstream_read(const fstream_read&) = delete;
        fstream_read(fstream_read&&)      = default;

        fstream_read(const std::filesystem::path& file) {
            if (!std::filesystem::exists(file))
                return;
            
            m_stream = std::make_unique<std::ifstream>(file, std::ios::binary);
            if (!m_stream || !m_stream->is_open()) {
                m_stream = nullptr;
                return;
            }

            seek(0, std::ios_base::end);
            m_size = pos();
            seek(0, std::ios_base::beg);
        }

        fstream_read& operator=(const fstream_read&) = delete;
        fstream_read& operator=(fstream_read&&) = default;

    public:
        size_t pos() const {
            return m_pos;
        }

        size_t size() const {
            return m_size;
        }

        bool is_valid() const {
            return m_stream && m_stream->is_open();
        }

        void seek(size_t offset, std::ios_base::seekdir dir = std::ios_base::cur) {
            if (!m_stream->seekg(offset, dir))
                throw std::out_of_range("Tried to seek outside file bounds.");

            m_pos = static_cast<size_t>(m_stream->tellg());
        }

        template<typename T>
        requires arithmetic<T>
        T read() {
            T value{};
            internal_read(&value, sizeof(T));
            return value;
        }

        std::string read(uint32_t size) {
            std::string value(size, 0);
            internal_read(value.data(), size);
            return value;
        }

        void read(uint8_t* dst, uint32_t size) {
            internal_read(dst, size);
        }

    private:
        std::unique_ptr<std::ifstream> m_stream;
        size_t                         m_size = 0U;
        size_t                         m_pos  = 0U;

    private:
        void internal_read(void* dst, uint32_t size) {
            if (m_pos + size > m_size)
                throw std::out_of_range("Tried to read outside file bounds.");

            if (!m_stream->read((char*)dst, (size_t)size))
                throw std::runtime_error("Failed to read requested size.");

            m_pos = static_cast<size_t>(m_stream->tellg());
        }
    };

    class stream_read {
    public:
        stream_read()                   = delete;
        stream_read(const stream_read&) = delete;
        stream_read(stream_read&&)      = default;

        stream_read(const std::vector<uint8_t>& buffer)
            : m_buffer(buffer) {}

        stream_read& operator=(const stream_read&) = delete;
        stream_read& operator=(stream_read&&)      = default;

    public:
        size_t pos() const {
            return m_pos;
        }

        size_t size() const {
            return m_buffer.size();
        }

        void seek(size_t offset, std::ios_base::seekdir dir = std::ios_base::cur) {
            if (dir == std::ios::cur)
                m_pos += offset;
            else if (dir == std::ios::beg)
                m_pos = offset;
            else if (dir == std::ios::end)
                m_pos = m_buffer.size() - offset;

            if (m_pos > m_buffer.size())
                throw std::out_of_range("Tried to seek outside bounds.");
        }

        template<typename T>
        requires arithmetic<T>
        T read() {
            T value{};
            internal_read(&value, sizeof(T));
            return value;
        }

        std::string read(uint32_t size) {
            std::string value(size, 0);
            internal_read(value.data(), size);
            return value;
        }

        void read(uint8_t* dst, uint32_t size) {
            internal_read(dst, size);
        }

    private:
        const std::vector<uint8_t>& m_buffer;
        size_t                      m_pos = 0U;

    private:
        void internal_read(void* dst, uint32_t size) {
            if (m_pos + size > m_buffer.size())
                throw std::out_of_range("Tried to read outside bounds.");

            memcpy(dst, m_buffer.data() + m_pos, size);
            m_pos += size;
        }
    };
}
