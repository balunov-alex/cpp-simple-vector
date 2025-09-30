#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity)
        : capacity_(capacity)
    {
    }
    
    size_t GetCapacity() const {
        return capacity_;
    }
    
private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t reserved_capacity) {
    return ReserveProxyObj(reserved_capacity);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;
    
    explicit SimpleVector(size_t size)
        : data_ptr_(size), size_(size), capacity_(size)
    {
        std::generate(begin(), end(), []() { return Type(); });
    }

    SimpleVector(size_t size, const Type& value)
        : data_ptr_(size), size_(size), capacity_(size)
    {
        std::fill(begin(), end(), value);
    }
  
    SimpleVector(size_t size, Type&& value)
        : data_ptr_(size), size_(size), capacity_(size)
    {
        std::generate(begin(), end(), [&value]() { return std::move(value); });
    }    

    SimpleVector(std::initializer_list<Type> init)
        : data_ptr_(init.size()), size_(init.size()), capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }
    
    SimpleVector(const SimpleVector<Type>& other) {
        if (capacity_ >= other.size_) {
            std::copy(other.begin(), other.end(), begin());
        } else {
            size_t new_capacity = (capacity_ * 2 >= other.size_ ? capacity_ * 2 : other.size_);
            ArrayPtr<Type> new_ptr(new_capacity);
            std::copy(other.begin(), other.end(), new_ptr.Get());
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;   
        }
        size_ = other.size_;
    }
    
    SimpleVector(SimpleVector&& other) noexcept {
        swap(other);
    }
    
    SimpleVector(const ReserveProxyObj& other)
        : data_ptr_(other.GetCapacity()), size_(0), capacity_(other.GetCapacity())
    {  
    }
    
    SimpleVector& operator=(const SimpleVector<Type>& rhs) {
        if (this != &rhs) {
            auto rhs_copy(rhs);
            swap(rhs_copy);
        }
        return *this;
    }
    
    SimpleVector& operator=(SimpleVector<Type>&& rhs) noexcept {
        if (this != &rhs) {
            swap(rhs);
        }    
        return *this;
    }    
    
    void PopBack() noexcept {
        assert(size_ != 0);
        --size_;
    }
    
    void PushBack(const Type& value) {
        if (size_ != capacity_) {
            data_ptr_[size_] = value;
            ++size_;
        } else {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2); 
            ArrayPtr<Type> new_ptr(new_capacity);
            std::copy(begin(), end(), new_ptr.Get());
            new_ptr[size_] = value;            
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
            ++size_;            
        }
    };
    
    void PushBack(Type&& value) {
        if (size_ != capacity_) {
            data_ptr_[size_] = std::move(value);
            ++size_;
        } else {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2); 
            ArrayPtr<Type> new_ptr(new_capacity);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), new_ptr.Get());
            new_ptr[size_] = std::move(value);            
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
            ++size_;            
        }
    }
    
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        const auto n = std::distance(cbegin(), pos);
        std::copy(std::make_move_iterator(&data_ptr_[n + 1]), std::make_move_iterator(end()), &data_ptr_[n]);
        --size_;
        return &data_ptr_[n];
    }
    
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        if (size_ != capacity_) {
            const auto n = std::distance(cbegin(), pos);
            std::copy_backward(pos, end(), end() + 1);
            data_ptr_[n] = value;
            ++size_;
            return &data_ptr_[n];
        } else {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_ptr(new_capacity);
            auto pos_in_new = std::copy(begin(), pos, new_ptr.Get());
            *pos_in_new = value;
            std::copy(pos, end(), pos_in_new + 1);
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
            ++size_;
            return pos_in_new;
        }
    }
    
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        const auto n = std::distance(cbegin(), pos);
        if (size_ != capacity_) {
            std::copy_backward(std::make_move_iterator(&data_ptr_[n]), std::make_move_iterator(end()), &data_ptr_[size_ + 1]);
            data_ptr_[n] = std::move(value);
            ++size_;
            return &data_ptr_[n];
        } else {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_ptr(new_capacity);
            auto pos_in_new = std::copy(std::make_move_iterator(begin()), std::make_move_iterator(&data_ptr_[n]), new_ptr.Get());
            *pos_in_new = std::move(value);
            std::copy(std::make_move_iterator(&data_ptr_[n]), std::make_move_iterator(end()), pos_in_new + 1);
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
            ++size_;
            return pos_in_new;
        }
    }    
    
    void swap(SimpleVector& other) noexcept {
        data_ptr_.swap(other.data_ptr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_ptr(new_capacity);
            std::copy(begin(), end(), new_ptr.Get());
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
        }
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_ptr_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return data_ptr_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("At():Indexing beyond the vector");
        }
        return data_ptr_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("At():Indexing beyond the vector");
        }
        return data_ptr_[index];
    }

    void Clear() noexcept {
        size_ = 0u;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
        } else if (new_size > size_ && new_size <= capacity_) {
            std::generate(end(), begin() + new_size, []() { return Type(); });
            size_ = new_size;
        } else if (new_size > size_ && new_size > capacity_) {
            size_t new_capacity = (capacity_ * 2 >= new_size ? capacity_ * 2 : new_size);
            ArrayPtr<Type> new_ptr(new_capacity);          
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), new_ptr.Get());
            std::generate(new_ptr.Get() + size_, new_ptr.Get() + new_size, []() { return Type(); });           
            data_ptr_.swap(new_ptr);
            capacity_ = new_capacity;
            size_ = new_size;
        }
    }

    Iterator begin() noexcept {
        return data_ptr_.Get();
    }

    Iterator end() noexcept {
        return data_ptr_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return cbegin();
    }

    ConstIterator end() const noexcept {
        return cend();
    }

    ConstIterator cbegin() const noexcept {
        return data_ptr_.Get();
    }

    ConstIterator cend() const noexcept {
        return data_ptr_.Get() + size_;
    }

private:
    ArrayPtr<Type> data_ptr_;
    size_t size_ = 0u;
    size_t capacity_ = 0u;
};

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
