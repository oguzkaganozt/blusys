#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace blusys {

template <typename T, std::size_t N>
using array = std::array<T, N>;

template <std::size_t N>
class string {
public:
    constexpr string() = default;

    constexpr explicit string(std::string_view value)
    {
        assign(value);
    }

    constexpr bool assign(std::string_view value)
    {
        if (value.size() > N) {
            return false;
        }

        len_ = value.size();
        for (std::size_t i = 0; i < len_; ++i) {
            storage_[i] = value[i];
        }
        storage_[len_] = '\0';
        return true;
    }

    constexpr bool push_back(char ch)
    {
        if (len_ >= N) {
            return false;
        }

        storage_[len_++] = ch;
        storage_[len_] = '\0';
        return true;
    }

    constexpr void clear()
    {
        len_ = 0;
        storage_[0] = '\0';
    }

    [[nodiscard]] constexpr const char *c_str() const
    {
        return storage_.data();
    }

    [[nodiscard]] constexpr std::string_view view() const
    {
        return std::string_view(storage_.data(), len_);
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return len_;
    }

    [[nodiscard]] static constexpr std::size_t capacity()
    {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return len_ == 0;
    }

    constexpr char &operator[](std::size_t index)
    {
        return storage_[index];
    }

    constexpr const char &operator[](std::size_t index) const
    {
        return storage_[index];
    }

private:
    std::array<char, N + 1> storage_{};
    std::size_t len_ = 0;
};

template <typename T, std::size_t N>
class ring_buffer {
public:
    [[nodiscard]] constexpr bool push_back(const T &value)
    {
        if (size_ == N) {
            return false;
        }

        storage_[tail_] = value;
        tail_ = (tail_ + 1) % N;
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr bool pop_front(T *out_value)
    {
        if (size_ == 0) {
            return false;
        }

        if (out_value != nullptr) {
            *out_value = storage_[head_];
        }

        head_ = (head_ + 1) % N;
        --size_;
        return true;
    }

    constexpr void clear()
    {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return size_;
    }

    [[nodiscard]] static constexpr std::size_t capacity()
    {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return size_ == 0;
    }

    [[nodiscard]] constexpr bool full() const
    {
        return size_ == N;
    }

private:
    std::array<T, N> storage_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
};

template <typename T, std::size_t N>
class static_vector {
public:
    [[nodiscard]] constexpr bool push_back(const T &value)
    {
        if (size_ == N) {
            return false;
        }

        storage_[size_++] = value;
        return true;
    }

    [[nodiscard]] constexpr bool push_back(T &&value)
    {
        if (size_ == N) {
            return false;
        }

        storage_[size_++] = std::move(value);
        return true;
    }

    constexpr void pop_back()
    {
        if (size_ > 0) {
            --size_;
        }
    }

    constexpr void clear()
    {
        size_ = 0;
    }

    [[nodiscard]] constexpr T &operator[](std::size_t index)
    {
        return storage_[index];
    }

    [[nodiscard]] constexpr const T &operator[](std::size_t index) const
    {
        return storage_[index];
    }

    [[nodiscard]] constexpr T *data()
    {
        return storage_.data();
    }

    [[nodiscard]] constexpr const T *data() const
    {
        return storage_.data();
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return size_;
    }

    [[nodiscard]] static constexpr std::size_t capacity()
    {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return size_ == 0;
    }

    [[nodiscard]] constexpr T *begin()
    {
        return storage_.data();
    }

    [[nodiscard]] constexpr const T *begin() const
    {
        return storage_.data();
    }

    [[nodiscard]] constexpr T *end()
    {
        return storage_.data() + size_;
    }

    [[nodiscard]] constexpr const T *end() const
    {
        return storage_.data() + size_;
    }

private:
    std::array<T, N> storage_{};
    std::size_t size_ = 0;
};

}  // namespace blusys
