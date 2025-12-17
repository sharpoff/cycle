#pragma once

#include <array>
#include <vector>
#include <list>
#include <unordered_map>
#include <map>
#include <string>
#include <set>
#include <queue>
#include <deque>

template <typename T, size_t N>
using Array = std::array<T, N>;

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using List = std::list<T>;

template <typename Key, typename T>
using UnorderedMap = std::unordered_map<Key, T>;

template <typename Key, typename T>
using OrderedMap = std::map<Key, T>;

using String = std::string;

template <typename Key>
using Set = std::set<Key>;

template <typename T>
using Queue = std::queue<T>;

template <typename T>
using Deque = std::deque<T>;