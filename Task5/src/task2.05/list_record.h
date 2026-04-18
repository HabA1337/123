#ifndef LIST_RECORD_H
#define LIST_RECORD_H

#include <cstring>
#include "record.h"

class list_record;

class node_record {
    record data;
    node_record* prev;
    node_record* next;
    
    node_record(record&& rec) : data(std::move(rec)), prev(nullptr), next(nullptr) {}
    friend class list_record;
};

class list_record {
private:
    node_record* head;
    node_record* tail;
    size_t count;
    
public:
    list_record() : head(nullptr), tail(nullptr), count(0) {}
    
    ~list_record() {
        clear();
    }
    
    list_record(const list_record&) = delete;
    list_record& operator=(const list_record&) = delete;

    void push_back(record&& rec) {
        node_record* node = new node_record(std::move(rec));
        if (!head) {
            head = tail = node;
        } else {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }
        count++;
    }

    void push_front(record&& rec) {
        node_record* node = new node_record(std::move(rec));
        if (!head) {
            head = tail = node;
        } else {
            node->next = head;
            head->prev = node;
            head = node;
        }
        count++;
    }
    
    void clear() {
        node_record* current = head;
        while (current) {
            node_record* next = current->next;
            delete current;
            current = next;
        }
        head = tail = nullptr;
        count = 0;
    }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }

    class Iterator {
    private:
        node_record* current;
    public:
        Iterator(node_record* node = nullptr) : current(node) {}
        
        Iterator& operator++() {
            if (current) current = current->next;
            return *this;
        }
        
        Iterator& operator--() {
            if (current) current = current->prev;
            return *this;
        }
        
        bool operator==(const Iterator& other) const {
            return current == other.current;
        }
        
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }
        
        const record& operator*() const {
            return current->data;
        }
        
        const record* operator->() const {
            return &current->data;
        }
        
        node_record* get_node() const { return current; }
    };
    
    Iterator begin() const { return Iterator(head); }
    Iterator end() const { return Iterator(nullptr); }
    
    Iterator erase(Iterator pos) {
        node_record* node = pos.get_node();
        if (!node) return end();
        
        node_record* next_node = node->next;
        
        if (node->prev)
            node->prev->next = node->next;
        else
            head = node->next;
        
        if (node->next)
            node->next->prev = node->prev;
        else
            tail = node->prev;
        
        delete node;
        count--;
        
        return Iterator(next_node);
    }
    
    bool exists(const char* n, int p, int g) const {
        for (node_record* cur = head; cur; cur = cur->next) {
            if (cur->data.get_phone() == p &&
                cur->data.get_group() == g &&
                cur->data.get_name() && n &&
                strcmp(cur->data.get_name(), n) == 0)
                return true;
        }
        return false;
    }

    void swap(Iterator a, Iterator b) {
        node_record* node_a = a.get_node();
        node_record* node_b = b.get_node();
        
        if (!node_a || !node_b || node_a == node_b) return;
        
        record temp = std::move(node_a->data);
        node_a->data = std::move(node_b->data);
        node_b->data = std::move(temp);
    }
};

#endif // LIST_RECORD_H