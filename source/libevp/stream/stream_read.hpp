#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace libevp {
    class stream_read {
    public:
        stream_read()                   = delete;
        stream_read(const stream_read&) = delete;
        stream_read(stream_read&&)      = default;

        stream_read(const std::string& file) {
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

        stream_read& operator=(const stream_read&) = delete;
        stream_read& operator=(stream_read&&) = default;

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
            if (m_pos + offset > m_size)
                throw std::out_of_range("Tried to seek outside file bounds.");

            m_stream->seekg(offset, dir);
            m_pos = static_cast<size_t>(m_stream->tellg());
        }

        template<typename T>
        T read_num() {
            if (m_pos + sizeof(T) > m_size)
                throw std::out_of_range("Tried to read outside file bounds.");

            T value{};
            m_stream->read((char*)&value, sizeof(T));
            m_pos = static_cast<size_t>(m_stream->tellg());

            return value;
        }

        std::string read_str(std::size_t len) {
            if (m_pos + len > m_size)
                throw std::out_of_range("Tried to read outside file bounds.");

            std::string value(len, 0);

            m_stream->read(value.data(), len);
            m_pos = static_cast<size_t>(m_stream->tellg());

            return value;
        }

        void read_bytes(uint8_t* dst, size_t size) {
            if (m_pos + size > m_size)
                throw std::out_of_range("Tried to read outside file bounds.");

            m_stream->read((char*)dst, size);
            m_pos = static_cast<size_t>(m_stream->tellg());
        }

    private:
        std::unique_ptr<std::ifstream> m_stream;
        size_t                         m_size = 0U;
        size_t                         m_pos  = 0U;
    };
}
