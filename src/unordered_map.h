//
// Created by Даниил Маслов on 21.05.2023.
//

#ifndef UNORDEREDMAP_UNORDERED_MAP_H
#define UNORDEREDMAP_UNORDERED_MAP_H

#include <math.h>
#include <functional>
#include <iostream>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap;

template <typename T, typename Allocator = std::allocator<T>>
class List {
    struct _base_node {
        _base_node()
            : _prev(this), _next(this){};
        _base_node(_base_node* prev, _base_node* next)
            : _prev(prev), _next(next){};
        _base_node* _prev = this;
        _base_node* _next = this;

        void reset() {
            _prev = this;
            _next = this;
        }
    };

    struct _node : _base_node {
        // _node() : _value(T()) {};
        // _node(const T& value) : _value(value) {};
        // _node(T&& value) : _value(std::move(value)) {};
        // _node(const T& value, _base_node* prev, _base_node* next) : _base_node(prev, next), _value(value) {};
        // _node(T&& value, _base_node* prev, _base_node* next) : _base_node(prev, next), _value(std::move(value)) {};

        template <typename... Args>
        _node(Args&&... args)
            : _value(std::forward<Args>(args)...){};

        T _value;
    };

  public:
    template <bool is_const>
    class _list_iterator {
      public:
        using type = T;
        using const_type = typename std::add_const<T>::type;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = type;
        using pointer = typename std::conditional<is_const, const_type*, type*>::type;
        using reference = typename std::conditional<is_const, const_type&, type&>::type;

        using iterator = _list_iterator<is_const>;
        using iterator_reference = _list_iterator<is_const>&;
        using iterator_const_reference = const _list_iterator<is_const>&;
        using const_iterator = _list_iterator<true>;
        using const_iterator_reference = _list_iterator<true>&;
        using const_iterator_const_reference = const _list_iterator<true>&;

        friend List<T, Allocator>;
        friend _list_iterator<true>;
        friend _list_iterator<false>;

        explicit _list_iterator(_base_node* ptr)
            : _node_pointer(ptr){};

        // Sorry, const list has const field _base_node, but
        // pointers in iterator are not const, even if the deque is const
        // because of increment and decrement operators
        explicit _list_iterator(const _base_node* ptr)
            : _node_pointer(const_cast<_base_node*>(ptr)){};  // NOLINT

        template <bool is_const1>
        explicit _list_iterator(const _list_iterator<is_const1>& another)
            : _node_pointer(another._node_pointer){};

        template <bool is_const1>
        bool operator==(const _list_iterator<is_const1>& it1) const {
            return _node_pointer == it1._node_pointer;
        }

        template <bool is_const1>
        bool operator!=(const _list_iterator<is_const1>& it1) const {
            return _node_pointer != it1._node_pointer;
        }

        iterator_reference operator++() {
            _node_pointer = _node_pointer->_next;
            return *this;
        }

        iterator operator++(int) {
            iterator copy = *this;
            ++(*this);
            return copy;
        }

        iterator_reference operator--() {
            _node_pointer = _node_pointer->_prev;
            return *this;
        }

        iterator operator--(int) {
            iterator copy = *this;
            --(*this);
            return copy;
        }

        operator const_iterator() const {
            return const_iterator(_node_pointer);
        }

        ~_list_iterator() = default;

      protected:
        _base_node* _node_pointer;
    };

    using type = T;
    using const_type = typename std::add_const<T>::type;
    using reference = type&;
    using const_reference = const_type&;
    using iterator = _list_iterator<false>;
    using const_iterator = _list_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using node_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<_node>;
    using node_allocator_traits = std::allocator_traits<node_allocator>;

    template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
    friend class UnorderedMap;

    List() = default;

    explicit List(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            try {
                default_push_back();
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(size_t size, const T& value) {
        for (size_t i = 0; i < size; ++i) {
            try {
                push_back(value);
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    explicit List(const Allocator& alloc)
        : _alloc(alloc){};

    List(size_t size, const Allocator& alloc)
        : _alloc(alloc) {
        for (size_t i = 0; i < size; ++i) {
            try {
                default_push_back();
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(size_t size, const T& value, const Allocator& alloc)
        : _alloc(alloc) {
        for (size_t i = 0; i < size; ++i) {
            try {
                push_back(value);
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(const List& another)
        : _alloc(std::allocator_traits<Allocator>::select_on_container_copy_construction(another._alloc)) {
        if (another._list_size > 0) {
            try {
                _base_node* ptr = _virtual_node._next;
                while (ptr != &_virtual_node) {
                    push_back(static_cast<_node*>(ptr)->_value);
                    ptr = ptr->_next;
                }
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(const List& another, const Allocator& alloc)
        : _alloc(alloc) {
        if (another._list_size > 0) {
            try {
                _base_node* ptr = _virtual_node._next;
                while (ptr != &_virtual_node) {
                    push_back(static_cast<_node*>(ptr)->_value);
                    ptr = ptr->_next;
                }
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    List(List&& another)
        : _virtual_node(another._virtual_node._prev, another._virtual_node._next),
          _list_size(another._list_size),
          _alloc(std::move(another._alloc)) {
        another._virtual_node._next->_prev = &(_virtual_node);
        another._virtual_node._prev->_next = &(_virtual_node);
        another._virtual_node.reset();
        another._list_size = 0;
    }

    List(List&& another, const Allocator& alloc) {
        List copy;
        swap(copy);
        swap(another);
        another._alloc = alloc;
    }

    List& operator=(const List& another) {
        List copy(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value ? another._alloc : _alloc);
        for (const auto& obj : another) {
            try {
                copy.push_back(obj);
            } catch (...) {
                copy.clear();
                throw;
            }
        }
        swap(copy);
        return *this;
    }

    List& operator=(List&& another) {
        clear();
        _alloc = (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ? another._alloc : _alloc);
        _virtual_node._next = another._virtual_node._next;
        another._virtual_node._next->_prev = &(_virtual_node);
        _virtual_node._prev = another._virtual_node._prev;
        another._virtual_node._prev->_next = &(_virtual_node);
        another._virtual_node.reset();
        _list_size = another._list_size;
        another._list_size = 0;
        return *this;
    }

    Allocator get_allocator() const {
        return static_cast<Allocator>(_alloc);
    }

    size_t size() const {
        return _list_size;
    }

    void push_front(const T& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->_next = _virtual_node._next;
        _virtual_node._next->_prev = node_ptr;
        node_ptr->_prev = &_virtual_node;
        _virtual_node._next = node_ptr;
        ++_list_size;
    }

    void push_front(T&& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, std::move(value));
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->_next = _virtual_node._next;
        _virtual_node._next->_prev = node_ptr;
        node_ptr->_prev = &_virtual_node;
        _virtual_node._next = node_ptr;
        ++_list_size;
    }

    void push_back(const T& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->_prev = _virtual_node._prev;
        _virtual_node._prev->_next = node_ptr;
        node_ptr->_next = &_virtual_node;
        _virtual_node._prev = node_ptr;
        ++_list_size;
    }

    void push_back(T&& value) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, std::move(value));
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->_prev = _virtual_node._prev;
        _virtual_node._prev->_next = node_ptr;
        node_ptr->_next = &_virtual_node;
        _virtual_node._prev = node_ptr;
        ++_list_size;
    }

    void pop_front() {
        auto ptr = static_cast<_node*>(_virtual_node._next);
        _virtual_node._next = _virtual_node._next->_next;
        _virtual_node._next->_prev = &_virtual_node;
        std::allocator_traits<node_allocator>::destroy(_alloc, ptr);
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
        --_list_size;
    }

    void pop_back() {
        auto ptr = static_cast<_node*>(_virtual_node._prev);
        _virtual_node._prev = _virtual_node._prev->_prev;
        _virtual_node._prev->_next = &_virtual_node;
        std::allocator_traits<node_allocator>::destroy(_alloc, ptr);
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
        --_list_size;
    }

    void insert(const_iterator iter, const T& value) {
        if (iter._node_pointer == &_virtual_node) {
            push_back(value);
        } else {
            _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
            try {
                std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, value);
            } catch (...) {
                std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
                throw;
            }
            node_ptr->_next = iter._node_pointer;
            node_ptr->_prev = iter._node_pointer->_prev;
            iter._node_pointer->_prev->_next = node_ptr;
            iter._node_pointer->_prev = node_ptr;
            ++_list_size;
        }
    }

    void insert(const_iterator iter, T&& value) {
        if (iter._node_pointer == &_virtual_node) {
            push_back(std::move(value));
        } else {
            _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
            try {
                std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, std::move(value));
            } catch (...) {
                std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
                throw;
            }
            node_ptr->_next = iter._node_pointer;
            node_ptr->_prev = iter._node_pointer->_prev;
            iter._node_pointer->_prev->_next = node_ptr;
            iter._node_pointer->_prev = node_ptr;
            ++_list_size;
        }
    }

    void insert(const_iterator iter, _node* node) {
        ++_list_size;
        node->_next = iter._node_pointer;
        iter._node_pointer->_prev = static_cast<_base_node>(node);
        --iter;
        node->_prev = iter._node_pointer;
        iter._node_pointer->_next = static_cast<_base_node>(node);
    }

    template <typename... Args>
    void insert(const_iterator iter, Args&&... args) {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr, std::forward<Args>(args)...);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        node_ptr->_next = iter._node_pointer;
        node_ptr->_prev = iter._node_pointer->_prev;
        iter._node_pointer->_prev->_next = node_ptr;
        iter._node_pointer->_prev = node_ptr;
        ++_list_size;
    }

    void erase(const_iterator iter) {
        auto* ptr = static_cast<_node*>(iter._node_pointer);
        ptr->_prev->_next = ptr->_next;
        ptr->_next->_prev = ptr->_prev;
        std::allocator_traits<node_allocator>::destroy(_alloc, ptr);
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
        --_list_size;
    }

    void clear() {
        while (_list_size > 0) {
            pop_back();
        }
        _virtual_node.reset();
    }

    iterator begin() {
        return iterator(_virtual_node._next);
    }

    const_iterator begin() const {
        return const_iterator(_virtual_node._next);
    }

    const_iterator cbegin() const {
        return const_iterator(_virtual_node._next);
    }

    iterator end() {
        return iterator(&_virtual_node);
    }

    const_iterator end() const {
        return const_iterator(&_virtual_node);
    }

    const_iterator cend() const {
        return const_iterator(&_virtual_node);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(begin());
    }

    iterator insert_node(const_iterator iter, _node* node_ptr) {
        ++_list_size;
        node_ptr->_next = iter._node_pointer;
        node_ptr->_prev = iter._node_pointer->_prev;
        iter._node_pointer->_prev->_next = node_ptr;
        iter._node_pointer->_prev = node_ptr;
        return iterator(node_ptr);
    }

    void virtual_erase(const_iterator iter) {
        iter._node_pointer->_prev->_next = iter._node_pointer->_next;
        iter._node_pointer->_next->_prev = iter._node_pointer->_prev;
        --_list_size;
    }

    void virtual_clear() {
        _virtual_node.reset();
        _list_size = 0;
    }

    ~List() {
        while (_list_size != 0) {
            pop_back();
        }
    }

  private:
    [[no_unique_address]] node_allocator _alloc = Allocator();
    _base_node _virtual_node = _base_node();
    size_t _list_size = 0;

    void swap(List<T, Allocator>& another) noexcept {
        std::swap(_alloc, another._alloc);
        std::swap(_virtual_node, another._virtual_node);
        std::swap(_virtual_node._next->_prev, another._virtual_node._next->_prev);
        std::swap(_virtual_node._prev->_next, another._virtual_node._prev->_next);
        std::swap(_list_size, another._list_size);
    }

    void default_push_back() {
        _node* node_ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        try {
            std::allocator_traits<node_allocator>::construct(_alloc, node_ptr);
        } catch (...) {
            std::allocator_traits<node_allocator>::deallocate(_alloc, node_ptr, 1);
            throw;
        }
        _virtual_node._prev->_next = node_ptr;
        node_ptr->_prev = _virtual_node._prev;
        _virtual_node._prev = node_ptr;
        node_ptr->_next = &_virtual_node;
        ++_list_size;
    }

    _node* allocate_node() {
        _node* ptr = nullptr;
        ptr = std::allocator_traits<node_allocator>::allocate(_alloc, 1);
        return ptr;
    }

    void deallocate_node(_node* ptr) {
        std::allocator_traits<node_allocator>::deallocate(_alloc, ptr, 1);
    }
};

template <typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
class UnorderedMap {
  public:
    using NodeType = std::pair<const Key, Value>;

    struct node {
        NodeType _data;
        size_t _hash = 0;

        node() = default;

        // explicit node(const NodeType& node) : _data(node) {};

        // explicit node(NodeType&& node) : _data(std::move(node)) {};

        // explicit node(const Key& key) : _data({key, Value()}) {};

        // explicit node(Key&& key) : _data({std::move(key), Value()}) {};

        template <typename... Args>
        node(Args&&... args)
            : _data(std::forward<Args>(args)...) {}
    };

    using list = List<node, Alloc>;
    using list_node = typename list::_node;
    using list_base_node = typename list::_base_node;

    template <bool is_const>
    class unmap_iterator : public list::template _list_iterator<is_const> {
      public:
        using type = std::pair<const Key, Value>;
        using const_type = typename std::add_const<type>::type;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = type;
        using pointer = typename std::conditional<is_const, const_type*, type*>::type;
        using const_pointer = typename std::add_const<pointer>;
        using reference = typename std::conditional<is_const, const_type&, type&>::type;
        using const_reference = typename std::add_const<reference>;
        using iterator = unmap_iterator<is_const>;
        using const_iterator = unmap_iterator<true>;

        friend class UnorderedMap;

        unmap_iterator(list_base_node* ptr)
            : list::template _list_iterator<is_const>(ptr){};

        template <bool is_const1>
        unmap_iterator(const typename list::template _list_iterator<is_const1>& it)
            : list::template _list_iterator<is_const>(it) {}

        operator const_iterator() const {
            return const_iterator(get_pointer());
        }

        pointer operator->() {
            return &(static_cast<list_node*>(this->_node_pointer)->_value._data);
        }

        const_pointer operator->() const {
            return &(static_cast<list_node*>(this->_node_pointer)->_value._data);
        }

        reference operator*() {
            return static_cast<list_node*>(this->_node_pointer)->_value._data;
        }

        const_reference operator*() const {
            return static_cast<list_node*>(this->_node_pointer)->_value._data;
        }

        template <bool is_const1>
        bool operator==(unmap_iterator<is_const1> another) {
            return this->_node_pointer == another._node_pointer;
        }

        ~unmap_iterator() = default;

      private:
        size_t get_hash() const {
            return static_cast<list_node*>(this->_node_pointer)->_value._hash;
        }

        list_node* get_pointer() const {
            return static_cast<list_node*>(this->_node_pointer);
        }
    };

    using iterator = unmap_iterator<false>;
    using const_iterator = unmap_iterator<true>;
    using iters = iterator*;
    using iterator_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<iterator>;
    using iterator_alloc_traits = typename std::allocator_traits<iterator_alloc>;

    UnorderedMap() = default;
    // iterator_alloc alloc = _allocator;
    // _buckets = std::allocator_traits<iterator_alloc>::allocate(alloc, _bucket_count);
    // _buckets[0] = end();

    explicit UnorderedMap(size_t size, Alloc allocator = Alloc(), float max_load_factor = 1.0) {
        iterator_alloc alloc = allocator;
        _buckets = std::allocator_traits<iterator_alloc>::allocate(alloc, size);
        _bucket_count = size;
        _allocator = allocator;
        _max_load_factor = max_load_factor;
        for (size_t i = 0; i < size; ++i) {
            _buckets[i] = end();
        }
    };

    UnorderedMap(const UnorderedMap& another)
        : UnorderedMap(another._bucket_count, another._allocator, another._max_load_factor) {
        for (iterator it = another._elements.begin(); it != another._elements.end(); ++it) {
            insert(*it);
        }
    }

    UnorderedMap(UnorderedMap&& another) {
        _elements = std::move(another._elements);
        _buckets = another._buckets;
        _bucket_count = another._bucket_count;
        _elements_count = another._elements_count;
        _max_load_factor = another._max_load_factor;
        another._buckets = nullptr;
        another._bucket_count = 0;
        another._elements_count = 0;
        another._max_load_factor = 1.0;
    }

    UnorderedMap& operator=(const UnorderedMap& another) {
        UnorderedMap copy(another);
        default_swap(copy);
        if (std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value) {
            _allocator = another._allocator;
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& another) noexcept {
        if (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value) {
            _allocator = another._allocator;
        }
        _elements = std::move(another._elements);
        iterator_alloc iter_alloc = _allocator;
        std::allocator_traits<iterator_alloc>::deallocate(iter_alloc, _buckets, _bucket_count);
        _buckets = another._buckets;
        _bucket_count = another._bucket_count;
        _elements_count = another._elements_count;
        _max_load_factor = another._max_load_factor;
        another._buckets = nullptr;
        another._bucket_count = 0;
        another._elements_count = 0;
        another._max_load_factor = 1.0;
        return *this;
    }

    std::pair<iterator, bool> insert(const NodeType& value) {
        rehash_if_filled(1);
        return default_emplace(value);
    }

    std::pair<iterator, bool> insert(NodeType&& value) {
        rehash_if_filled(1);
        return default_emplace(std::move(value));
    }

    std::pair<iterator, bool> insert1(NodeType&& value) {
        rehash_if_filled(1);
        return default_emplace(std::move(value));
    }

    template <typename T>
    std::pair<iterator, bool> insert(T&& arg) {
        rehash_if_filled(1);
        return default_emplace(std::forward<T>(arg));
    }

    template <typename input_iterator>
    void insert(input_iterator first, input_iterator last) {
        while (first != last) {
            insert(*first);
            ++first;
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        rehash_if_filled(1);
        return default_emplace(std::forward<Args>(args)...);
    }

    Value& operator[](const Key& key) {
        iterator it = find(key);
        if (it == end()) {
            it = insert({key, Value()}).first;
        }
        return it->second;
    }

    Value& operator[](Key&& key) {
        iterator it = find(key);
        if (it == end()) {
            it = insert({std::move(key), Value()}).first;
        }
        return it->second;
    }

    Value& at(const Key& key) {
        iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("UnorderedMap::at: key not found!");
        }
        return it->second;
    }

    const Value& at(const Key& key) const {
        iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("UnorderedMap::at: key not found!");
        }
        return it->second;
    }

    iterator erase(const_iterator iter) {
        size_t hash = iter.get_hash();
        const_iterator it = iter;
        ++it;
        auto bucket_iter = static_cast<const_iterator>(_buckets[hash % _bucket_count]);
        if (static_cast<typename list::const_iterator>(bucket_iter) == static_cast<typename list::const_iterator>(iter)) {
            ++bucket_iter;
            if (bucket_iter != end() && bucket_iter.get_hash() % _bucket_count == hash % _bucket_count) {
                _buckets[hash % _bucket_count] = bucket_iter;
            } else {
                _buckets[hash % _bucket_count] = end();
            }
        }
        _elements.erase(iter);
        --_elements_count;
        return it;
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return first;
    }

    float load_factor() const noexcept {
        return (_bucket_count != 0 ? static_cast<float>(_elements_count) / _bucket_count : MAXFLOAT);
    }

    float max_load_factor() const noexcept {
        return _max_load_factor;
    }

    void max_load_factor(float max_load_factor) {
        _max_load_factor = max_load_factor;
        rehash_if_filled(0);
    }

    void reserve(size_t new_size) {
        size_t new_capacity = std::ceil(static_cast<float>(new_size) / _max_load_factor);
        rehash(new_capacity);
    }

    size_t size() const {
        return _elements_count;
    }

    iterator begin() {
        return _elements.begin();
    }

    const_iterator begin() const {
        return _elements.begin();
    }

    const_iterator cbegin() const {
        return _elements.cbegin();
    }

    iterator end() {
        return _elements.end();
    }

    iterator end() const {
        return _elements.end();
    }

    iterator cend() const {
        return _elements.cend();
    }

    iterator find(const Key& key) {
        return informed_find(key, _hasher(key));
    }

    const_iterator find(const Key& key) const {
        return informed_find(key, _hasher(key));
    }

    void swap(UnorderedMap& another) {
        default_swap(another);
        if (std::allocator_traits<Alloc>::propagate_on_container_swap::value) {
            std::swap(_allocator, another._allocator);
        }
    }

    ~UnorderedMap() {
        if (_buckets != nullptr) {
            iterator_alloc alloc = _allocator;
            std::allocator_traits<iterator_alloc>::deallocate(alloc, _buckets, _bucket_count);
        }
    }

  private:
    iters _buckets = nullptr;
    list _elements = list();
    size_t _bucket_count = 0;
    size_t _elements_count = 0;
    float _max_load_factor = 1.0;

    [[no_unique_address]] Alloc _allocator = Alloc();
    [[no_unique_address]] Hash _hasher = Hash();
    [[no_unique_address]] Equal _equalizer = Equal();

    const_iterator informed_find(const Key& key, size_t hash) const {
        if (_bucket_count == 0) {
            return end();
        }
        iterator it = _buckets[hash % _bucket_count];
        if (it == end()) {
            return it;
        }
        while (it != _elements.end() && it.get_hash() == hash) {
            if (_equalizer(key, it->first)) {
                return it;
            }
            ++it;
        }
        return _elements.end();
    }

    void rehash_if_filled(size_t additional_elements) {
        if (potential_load_factor(_elements_count + additional_elements) >= _max_load_factor) {
            size_t new_size = 2 * std::ceil((_elements_count + additional_elements) / _max_load_factor);
            rehash(new_size);
        }
    }

    void rehash(size_t new_size) {
        std::vector<list_node*> v;
        for (iterator it = begin(); it != end(); ++it) {
            v.push_back(it.get_pointer());
        }
        _elements.virtual_clear();
        _elements_count = 0;
        iterator_alloc iter_alloc = _allocator;
        iters new_buckets = nullptr;
        try {
            new_buckets = std::allocator_traits<iterator_alloc>::allocate(iter_alloc, new_size);
        } catch (...) {
            if (!v.empty()) {
                _elements._virtual_node._next = v[0];
                _elements._virtual_node._prev = v[v.size() - 1];
                _elements_count = v.size();
            }
            throw;
        }
        if (_buckets != nullptr) {
            std::allocator_traits<iterator_alloc>::deallocate(iter_alloc, _buckets, _bucket_count);
        }
        _buckets = new_buckets;
        new_buckets = nullptr;
        _bucket_count = new_size;
        for (size_t i = 0; i < _bucket_count; ++i) {
            _buckets[i] = end();
        }
        for (size_t i = 0; i < v.size(); ++i) {
            insert_node(v[i]);
            v[i] = nullptr;
        }
    }

    float potential_load_factor(size_t new_element_count) const {
        return (_bucket_count != 0 ? static_cast<float>(new_element_count) / _bucket_count : MAXFLOAT);
    }

    template <typename... Args>
    std::pair<iterator, bool> default_emplace(Args&&... args) noexcept {
        using pair_allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<NodeType>;
        list_node* ptr = nullptr;
        pair_allocator pair_alloc = _allocator;
        try {
            ptr = _elements.allocate_node();
            std::allocator_traits<pair_allocator>::construct(pair_alloc, &ptr->_value._data, std::forward<Args>(args)...);
            size_t hash = _hasher(ptr->_value._data.first);
            ptr->_value._hash = hash;
            ptr->_next = ptr;
            ptr->_prev = ptr;
            iterator it = informed_find(ptr->_value._data.first, hash);
            if (it != end()) {
                _elements.insert_node(begin(), ptr);
                _elements.erase(begin());
                return {it, false};
            }
            if (_buckets[hash % _bucket_count] == end()) {
                _buckets[hash % _bucket_count] = begin();
            }
            _elements.insert_node(_buckets[hash % _bucket_count], ptr);
            --_buckets[hash % _bucket_count];
            ++_elements_count;
            return {_buckets[hash % _bucket_count], true};
        } catch (...) {
            _elements.deallocate_node(ptr);
            return {end(), false};
        }
    }

    void default_swap(UnorderedMap& another) {
        _elements.swap(another._elements);
        std::swap(_elements_count, another._elements_count);
        std::swap(_buckets, another._buckets);
        std::swap(_bucket_count, another._bucket_count);
        std::swap(_max_load_factor, another._max_load_factor);
    }

    void insert_node(list_node* ptr) {
        ++_elements_count;
        size_t hash = ptr->_value._hash;
        if (_buckets[hash % _bucket_count] == end()) {
            _buckets[hash % _bucket_count] = begin();
        }
        _elements.insert_node(_buckets[hash % _bucket_count], ptr);
        --_buckets[hash % _bucket_count];
    }
};

#endif  //UNORDEREDMAP_UNORDERED_MAP_H