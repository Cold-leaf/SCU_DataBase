#include <cassert>
#include <functional>
#include <list>
#include <bitset>
#include <iostream>

#include "hash/extendible_hash.h"
#include "page/page.h"

namespace scudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
    template <typename K, typename V>
    ExtendibleHash<K, V>::ExtendibleHash(size_t size)
            : bucket_size_(size), bucket_count_(0), pair_count_(0), global_depth(0)
    {
        bucket_.emplace_back(new Bucket(0, 0));         // add a Bucket(0,0) (id=0,local_depth=0)
        bucket_count_ = 1;
    }

/*
 * helper function to calculate the hashing address of input key
 * std::hash<>: assumption already has specialization for type K
 * namespace std have standard specializations for basic types.
 */
    template <typename K, typename V>
    size_t ExtendibleHash<K, V>::HashKey(const K &key)
    {
        return std::hash<K>()(key);
    }


/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetGlobalDepth() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return global_depth;
    }

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(bucket_[bucket_id])
        {
            return bucket_[bucket_id]->depth;   // return local depth
        }
        return -1;
    }

/*
 * helper function to return current number of bucket in hash table
 */
    template <typename K, typename V>
    int ExtendibleHash<K, V>::GetNumBuckets() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return bucket_count_;
    }


/*
 * lookup function to find value associate with input key
 */
    template <typename K, typename V>
    bool ExtendibleHash<K, V>::Find(const K &key, V &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t position = HashKey(key) & ((1 << global_depth) - 1);     // 保留key哈希值的低位（对应桶号）

        if(bucket_[position])
        {
            if(bucket_[position]->data.find(key) != bucket_[position]->data.end())
            {
                value = bucket_[position]->data[key];                  // 依据key把值给value
                return true;
            }
        }
        return false;
    }

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
    template <typename K, typename V>
    bool ExtendibleHash<K, V>::Remove(const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t position = HashKey(key) & ((1 << global_depth) - 1);
        size_t cnt = 0;

        if(bucket_[position])
        {
            auto tmp_bucket = bucket_[position];
            cnt = tmp_bucket->data.erase(key);    // 删除成功返回1
            pair_count_ -= cnt;                    // 成功减一
        }
        return cnt != 0;               // 成功该式为1
    }


/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
    template <typename K, typename V>
    void ExtendibleHash<K, V>::Insert(const K &key, const V &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t bucket_id = HashKey(key) & ((1 << global_depth) - 1);

        // 找到插入的位置，如果为空则新建一个桶
        if(bucket_[bucket_id] == nullptr)
        {
            bucket_[bucket_id] = std::make_shared<Bucket>(bucket_id, global_depth);
            ++bucket_count_;
        }
        auto bucket = bucket_[bucket_id];

        // 如果该位置有值，则覆盖
        if(bucket->data.find(key) != bucket->data.end())
        {
            bucket->data[key] = value;
            return;
        }

        // 插入键值对
        bucket->data.insert({key, value});
        ++pair_count_;

        // 需要分裂桶以及重新分配
        if(bucket->data.size() > bucket_size_)
        {
            // 先记录旧的下标和全局深度
            auto old_index = bucket->id;
            auto old_depth = bucket->depth;

            std::shared_ptr<Bucket> new_bucket = split(bucket);

            // 溢出了就改回原来的深度
            if(new_bucket == nullptr)
            {
                bucket->depth = old_depth;
                return;
            }

            // 若插入的桶的局部深度大于全局深度，则要扩展桶数组
            if(bucket->depth > global_depth)
            {
                auto size = bucket_.size();
                auto factor = (1 << (bucket->depth - global_depth));    // 扩展倍数

                global_depth = bucket->depth;
                bucket_.resize(bucket_.size()*factor);

                // 修改和添加要插入的桶和新建的桶
                bucket_[bucket->id] = bucket;
                bucket_[new_bucket->id] = new_bucket;

                // 遍历桶数组
                for(size_t i = 0; i < size; ++i)
                {
                    if(bucket_[i])
                    {
                        if(i < bucket_[i]->id)
                        {
                            bucket_[i].reset();
                        }
                        else
                        {
                            auto step = 1 << bucket_[i]->depth;
                            for(size_t j = i + step; j < bucket_.size(); j += step)     // 多出的桶索引连接到暂未分裂的桶（忽略高位，只看目前深度的低位，下同）
                            {
                                bucket_[j] = bucket_[i];
                            }
                        }
                    }
                }
            }
            else
            {
                for (size_t i = old_index; i < bucket_.size(); i += (1 << old_depth))
                {
                    bucket_[i].reset();
                }

                bucket_[bucket->id] = bucket;
                bucket_[new_bucket->id] = new_bucket;

                auto step = 1 << bucket->depth;
                for (size_t i = bucket->id + step; i < bucket_.size(); i += step)
                {
                    bucket_[i] = bucket;
                }
                for (size_t i = new_bucket->id + step; i < bucket_.size(); i += step)
                {
                    bucket_[i] = new_bucket;
                }
            }
        }
    }

// 分裂新桶 并重新分配桶内元素
    template <typename K, typename V>
    std::shared_ptr<typename ExtendibleHash<K, V>::Bucket>
    ExtendibleHash<K, V>::split(std::shared_ptr<Bucket> &b)
    {
        // 创建一个新桶
        auto res = std::make_shared<Bucket>(0, b->depth);

        while(res->data.empty())
        {
            b->depth++;
            res->depth++;
            // 实现两个桶的分配
            for(auto it = b->data.begin(); it != b->data.end();)
            {
                // 两HashKey后面的式子不一样
                if (HashKey(it->first) & (1 << (b->depth - 1)))         // 看新高位是1/0    1的分入res 0的保留在b
                {
                    res->data.insert(*it);
                    res->id = HashKey(it->first) & ((1 << b->depth) - 1);       //  依据新桶深度分配id
                    it = b->data.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // 如果b桶的map为空，说明深度不够，还要进行循环
            if(b->data.empty())
            {
                b->data.swap(res->data);
                b->id = res->id;
            }
        }

        ++bucket_count_;
        return res;
    }

    template class ExtendibleHash<page_id_t, Page *>;
    template class ExtendibleHash<Page *, std::list<Page *>::iterator>;
// test purpose
    template class ExtendibleHash<int, std::string>;
    template class ExtendibleHash<int, std::list<int>::iterator>;
    template class ExtendibleHash<int, int>;
} // namespace scudb
