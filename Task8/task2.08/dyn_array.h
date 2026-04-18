#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include <utility>

template <typename T>
class dyn_array {
private:
    T* data_;
    int size_;
    int capacity_;

    void grow(int min_cap) {
        int new_cap = capacity_ < 8 ? 8 : capacity_ * 2;
        while (new_cap < min_cap) new_cap *= 2;
        T* buf = new T[new_cap];
        for (int i = 0; i < size_; i++)
            buf[i] = std::move(data_[i]);
        delete[] data_;
        data_ = buf;
        capacity_ = new_cap;
    }

public:
    dyn_array() : data_(nullptr), size_(0), capacity_(0) {}
    ~dyn_array() { delete[] data_; }

    dyn_array(dyn_array&& o) noexcept
        : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
        o.data_ = nullptr;
        o.size_ = 0;
        o.capacity_ = 0;
    }

    dyn_array& operator=(dyn_array&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_;
            size_ = o.size_;
            capacity_ = o.capacity_;
            o.data_ = nullptr;
            o.size_ = 0;
            o.capacity_ = 0;
        }
        return *this;
    }

    dyn_array(const dyn_array&) = delete;
    dyn_array& operator=(const dyn_array&) = delete;

    void push_back(const T& val) {
        if (size_ == capacity_) grow(size_ + 1);
        data_[size_++] = val;
    }

    void push_back(T&& val) {
        if (size_ == capacity_) grow(size_ + 1);
        data_[size_++] = std::move(val);
    }

    void insert_at(int idx, const T& val) {
        if (size_ == capacity_) grow(size_ + 1);
        for (int i = size_; i > idx; i--)
            data_[i] = std::move(data_[i - 1]);
        data_[idx] = val;
        size_++;
    }

    void insert_at(int idx, T&& val) {
        if (size_ == capacity_) grow(size_ + 1);
        for (int i = size_; i > idx; i--)
            data_[i] = std::move(data_[i - 1]);
        data_[idx] = std::move(val);
        size_++;
    }

    void erase(int idx) {
        for (int i = idx; i < size_ - 1; i++)
            data_[i] = std::move(data_[i + 1]);
        size_--;
    }

    void resize(int n) {
        if (n > capacity_) grow(n);
        size_ = n;
    }

    T& operator[](int i) { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }

    T* data() { return data_; }
    const T* data() const { return data_; }
};

#endif
