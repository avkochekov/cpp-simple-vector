#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <algorithm>

#include "array_ptr.h"

class ReserveProxyObj{
public:
    ReserveProxyObj(size_t size) : size(size) {}
    size_t size = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
    size_t capacity = 0u;
    size_t size = 0u;

    ArrayPtr<Type> array;

public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : capacity(size), size(size), array(size){
        std::fill(begin(), end(), Type());
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : capacity(size), size(size), array(size){
        std::fill(begin(), end(), value);
    }

    explicit SimpleVector(ReserveProxyObj capacity) : capacity(capacity.size), array(capacity.size){ }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : capacity(init.size()), size(init.size()), array(init.size()) {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector& other) : capacity(other.size), size(other.size), array(other.size){
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector(SimpleVector&& other) {
        swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        SimpleVector<Type> tmp(rhs.size);
        std::copy(rhs.begin(), rhs.end(), tmp.begin());

        swap(tmp);
        capacity = rhs.capacity;
        size = rhs.size;

        return *this;
    }

    void Reserve(const ReserveProxyObj& obj){
        if (obj.size < capacity){
            return;
        }
        SimpleVector<Type> tmp(obj.size);
        std::move(begin(), end(), tmp.begin());
        size_t old_size = size;
        swap(tmp);
        size = old_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        Insert(end(), item);
    }

    void PushBack(Type&& item) {
        Insert(end(), std::move(item));
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos < end());
        Iterator iter = Iterator(pos);
        Resize(size + 1);
        std::move_backward(iter, end(), end() + 1);
        *iter = value;
        ++size;
        return iter;
    }

    Iterator Insert(Iterator pos, Type&& value) {
        assert(pos >= begin() && pos < end());
        auto dist = std::distance(begin(), pos);
//        Iterator tmp_end = Iterator(end());
        Resize(std::move(size + 1));
        std::move_backward(std::make_move_iterator(begin() + dist), std::make_move_iterator(end() - 1), end());
        std::exchange(*(begin() + dist), std::move(value));
//        ++size;
        return begin() + dist;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size == 0){
            return;
        }
        --size;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        auto n = std::distance(cbegin(), pos);
        std::move(begin() + n + 1, end(), begin() + n);
        --size;
        return begin() + n;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        array.swap(other.array);
        std::swap(size, other.size);
        std::swap(capacity, other.capacity);
    }

    ~SimpleVector(){
        delete array.Release();
    }


    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size);
        return *(array.Get() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size);
        return *(array.Get() + index);
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index < size){
            return *(array.Get() + index);
        } else {
            throw std::out_of_range("out of range");
        }
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index < size){
            return *(array.Get() + index);
        } else {
            throw std::out_of_range("out of range");
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size) {
            size = new_size;
        } else {
            if (new_size < capacity){
                size = new_size;
            } else {
                auto new_capacity = std::max(new_size, capacity * 2);
                ArrayPtr<Type> new_items(new_capacity);
                std::move(begin(), end(), new_items.Get());
                std::generate(new_items.Get() + size, new_items.Get() + new_size, [](){return std::move(Type());});
                capacity = new_capacity;
                size = new_size;
                array.swap(new_items);
            }
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(array.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(array.Get() + size);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator(array.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator(array.Get() + size);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ConstIterator(array.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator(array.Get() + size);
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs == rhs) || (lhs < rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs <= rhs);
    return true;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
