#include <unordered_map>
#include <map>
#include <queue>
#include <cstdint>
#include <chrono>
template<typename Clock>
class KVStorage {
private:
    using Key       = std::string;
    using Value     = std::string;
    using TTL       = uint32_t;
    using TimePoint = typename Clock::time_point;
    struct Entry {
        Value value;
        TimePoint expire_time; // Clock::time_point::max() for eternal record
    };

    struct ExpireRecord {
        TimePoint expire_time;
        Key key;
    };

    struct ExpireCompare {
        bool operator()(const ExpireRecord& a, const ExpireRecord& b) const {
            return a.expire_time > b.expire_time; // min-heap for time
        }
    };

    using BaseIter = typename std::unordered_map<Key, Entry>::iterator;
    std::unordered_map<Key, Entry> base_storage_;
    std::map<Key, BaseIter> sorted_storage_;
    std::priority_queue<ExpireRecord,std::vector<ExpireRecord>,ExpireCompare> ttl_controller_;
public:
    explicit KVStorage(std::span<std::tuple<std::string, std::string, uint32_t>> data) {
        
        for(const auto record : data){
            Key key = std::get<0>(record);
            Value value = std::get<1>(record);
            TTL ttl = std::get<2>(record);
            TimePoint expire_time;
            ttl == 0 ? expire_time=Clock::time_point::max() :  expire_time =Clock::now()+std::chrono::seconds(ttl);
            base_storage_[key]= {value,expire_time};
            sorted_storage_[key]=base_storage_.find(key);
            if(ttl>0) {
                ttl_controller_.push({expire_time,key});
            }
        }
    }
    
    ~KVStorage() {}

    void set(std::string key, std::string value, uint32_t ttl) {
        TimePoint expire_time;
        if (ttl == 0) {
            expire_time = Clock::time_point::max();
        } else {
            expire_time = Clock::now() + std::chrono::seconds(ttl);
            ttl_controller_.push({expire_time, key});
        }

        base_storage_[key] = {value, expire_time};
        sorted_storage_[key] = base_storage_.find(key);

    }

    bool remove(std::string_view key) {
        auto base_storage_ptr = base_storage_.find(std::string(key));
        if (base_storage_ptr == base_storage_.end()) {
            return false;
        }
        auto sorted_storage_ptr=sorted_storage_.find(std::string(key));
        base_storage_.erase(base_storage_ptr);
        sorted_storage_.erase(sorted_storage_ptr);
        return true;
    }

    std::optional<std::string> get(std::string_view key) const {
        auto ptr = base_storage_.find(std::string(key));
        if (ptr != base_storage_.end()) {
            auto& entry = ptr->second;
            if (entry.expire_time > Clock::now()) {
                return entry.value;
            }
        }
        return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> getManySorted(std::string_view key, size_t count) {
        auto it= sorted_storage_.lower_bound(std::string(key));
        std::vector<std::pair<std::string, std::string>> result;
        while(it!=sorted_storage_.end() && count!=0){
            result.push_back({it->first,it->second->second.value});
            it++;
            count--;
        }
        return result;
    }

    std::optional<std::pair<std::string, std::string>> removeOneExpiredEntry() {
        while (!ttl_controller_.empty()) {
            auto ref = ttl_controller_.top();
            ttl_controller_.pop();
            auto ptr = base_storage_.find(ref.key);
            if (ptr != base_storage_.end()) {
                auto value = ptr->second.value;
                remove(ref.key);
                return std::make_pair(ref.key, value);
            }
        }
        return std::nullopt;
    } 
};