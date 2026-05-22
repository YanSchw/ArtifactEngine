#pragma once
#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>
#include <ranges>
#include "Core/Assert.h"

struct TArray {

};

template <class T>
class Array {
private:
    TArray m_BaseArray;
    std::vector<T> m_Data;

public:

    Array() = default;
    Array(size_t size) : m_Data(size) { }
    Array(const std::vector<T>& vector) {
        m_Data = vector;
    }
    Array(const std::initializer_list<T>& vector) {
        m_Data = vector;
    }

    template<typename... Args>
    void Emplace(Args&&... args) {
        m_Data.emplace_back(std::forward<Args>(args)...);
    }
    void Add(T&& item) {
        m_Data.push_back(std::move(item));
    }
    void Add(const T& item) {
        m_Data.push_back(item);
    }

    void Insert(int32_t index, const T& item) {
        m_Data.insert(m_Data.begin() + index, item);
    }

    void Insert(int32_t index, T&& item) {
        m_Data.insert(m_Data.begin() + index, std::move(item));
    }

    void SwapElements(int32_t indexA, int32_t indexB) {
        std::iter_swap(m_Data.begin() + indexA, m_Data.begin() + indexB);
    }

    inline T& Get(int32_t index) {
        return m_Data[index];
    }
    inline T& FirstItem() {
        return m_Data[0];
    }
    inline T& SecondItem() {
        return m_Data[1];
    }
    inline T& LastItem() {
        return m_Data[Last()];
    }
    inline T& operator[](int32_t index) {
        return m_Data[index];
    }
    inline const T& operator[](int32_t index) const {
        return m_Data[index];
    }

    int32_t IndexOf(const T& item) const {
        auto it = std::find(m_Data.begin(), m_Data.end(), item);

        if (it != m_Data.end()) return (int32_t)(it - m_Data.begin());
        else return -1;
    }

    void RemoveAt(int32_t index) {
        AE_ASSERT(index >= 0 && index < Size());
        m_Data.erase(m_Data.begin() + index);
    }
    void Remove(const T& item) {
        const int32_t index = IndexOf(item);
        if (index != -1)
            RemoveAt(index);
    }
    inline void RemoveFirstItem() {
        RemoveAt(0);
    }
    inline void RemoveSecondItem() {
        RemoveAt(1);
    }
    inline void RemoveLastItem() {
        RemoveAt(Last());
    }

    bool Contains(const T& item) const {
        return std::find(m_Data.begin(), m_Data.end(), item) != m_Data.end();
    }

    Array<T> operator+(const Array<T>& other) const {
        Array<T> result = *this;
        result += other;
        return result;
    }
    void operator+=(const Array<T>& other) {
        m_Data.insert(
            m_Data.end(),
            other.m_Data.begin(),
            other.m_Data.end()
        );
    }

    void Clear() {
        m_Data.clear();
    }

    inline int32_t Size() const {
        return (int32_t)m_Data.size();
    }
    // Returns the last valid index [ Size() - 1 ]
    inline int32_t Last() const {
        return Size() - 1;
    }

    bool IsEmpty() const {
        return m_Data.empty();
    }

    const std::vector<T>& GetData() const {
        return m_Data;
    }

    std::vector<T>& GetData() {
        return m_Data;
    }

    void SetData(const std::vector<T>& data) {
        m_Data = data;
    }

    void SetData(std::vector<T>&& data) {
        m_Data = std::move(data);
    }

    // Ascending: a < b
    // Descending: a > b
    void Sort(const std::function<bool(const T&, const T&)>& lambda) {
        std::ranges::sort(m_Data, lambda);
    }

    // Iterators

    typename std::vector<T>::iterator begin() { return m_Data.begin(); }
    typename std::vector<T>::const_iterator begin() const { return m_Data.begin(); }
    typename std::vector<T>::const_iterator cbegin() const { return m_Data.cbegin(); }
    typename std::vector<T>::iterator end() { return m_Data.end(); }
    typename std::vector<T>::const_iterator end() const { return m_Data.end(); }
    typename std::vector<T>::const_iterator cend() const { return m_Data.cend(); }

};