#ifndef RECORD_LIST_H
#define RECORD_LIST_H

#include "record.h"

struct list_node {
    record rec;
    list_node* prev;
    list_node* next;
    list_node() : prev(nullptr), next(nullptr) {}
};

class record_list {
private:
    list_node* head_;
    list_node* tail_;
    int size_;

public:
    record_list() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~record_list() {
        list_node* cur = head_;
        while (cur) {
            list_node* tmp = cur->next;
            delete cur;
            cur = tmp;
        }
    }

    record_list(record_list&& o) noexcept
        : head_(o.head_), tail_(o.tail_), size_(o.size_) {
        o.head_ = nullptr;
        o.tail_ = nullptr;
        o.size_ = 0;
    }

    record_list& operator=(record_list&& o) noexcept {
        if (this != &o) {
            list_node* cur = head_;
            while (cur) {
                list_node* tmp = cur->next;
                delete cur;
                cur = tmp;
            }
            head_ = o.head_;
            tail_ = o.tail_;
            size_ = o.size_;
            o.head_ = nullptr;
            o.tail_ = nullptr;
            o.size_ = 0;
        }
        return *this;
    }

    record_list(const record_list&) = delete;
    record_list& operator=(const record_list&) = delete;

    list_node* push_back(record&& r) {
        list_node* nd = new list_node;
        nd->rec = std::move(r);
        nd->prev = tail_;
        nd->next = nullptr;
        if (tail_)
            tail_->next = nd;
        else
            head_ = nd;
        tail_ = nd;
        size_++;
        return nd;
    }

    void remove(list_node* nd) {
        if (nd->prev)
            nd->prev->next = nd->next;
        else
            head_ = nd->next;
        if (nd->next)
            nd->next->prev = nd->prev;
        else
            tail_ = nd->prev;
        delete nd;
        size_--;
    }

    list_node* head() const { return head_; }
    int size() const { return size_; }
};

#endif
