#include <unordered_map>
#include <set>
#include <queue>
#include <cstdint>
#include <chrono>
#include <optional>
#include <string>
#include <span>
#include <tuple>
#include <vector>

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
            return a.expire_time > b.expire_time; // min-heap
        }
    };

    std::unordered_map<Key, Entry> base_storage_;
    std::set<Key> sorted_storage_;
    std::priority_queue<ExpireRecord, std::vector<ExpireRecord>, ExpireCompare> ttl_controller_;

public:
    explicit KVStorage(std::span<std::tuple<std::string, std::string, uint32_t>> data) {
        for (const auto& record : data) {
            Key key = std::get<0>(record);
            Value value = std::get<1>(record);
            TTL ttl = std::get<2>(record);

            TimePoint expire_time = (ttl == 0)? Clock::time_point::max() : Clock::now() + std::chrono::seconds(ttl);

            base_storage_[key] = {value, expire_time};
            sorted_storage_.insert(key);

            if (ttl > 0) {
                ttl_controller_.push({expire_time, key});
            }
        }
    }

    ~KVStorage() = default;

    void set(std::string key, std::string value, uint32_t ttl) {
        TimePoint expire_time = (ttl == 0)
            ? Clock::time_point::max()
            : Clock::now() + std::chrono::seconds(ttl);

        base_storage_[key] = {value, expire_time};
        sorted_storage_.insert(key);

        if (ttl > 0) {
            ttl_controller_.push({expire_time, key});
        }
    }

    bool remove(std::string_view key) {
        auto it = base_storage_.find(std::string(key));
        if (it == base_storage_.end()) {
            return false;
        }
        base_storage_.erase(it);
        sorted_storage_.erase(std::string(key));
        return true;
    }

    std::optional<std::string> get(std::string_view key) const {
        auto it = base_storage_.find(std::string(key));
        if (it != base_storage_.end()) {
            const auto& entry = it->second;
            if (entry.expire_time > Clock::now()) {
                return entry.value;
            }
        }
        return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> getManySorted(std::string_view key, size_t count) {
        std::vector<std::pair<std::string, std::string>> result;
        result.reserve(count);

        auto it = sorted_storage_.lower_bound(std::string(key));
        while (it != sorted_storage_.end() && count-- > 0) {
            const auto& k = *it;
            const auto& entry = base_storage_.at(k);
            if (entry.expire_time > Clock::now()) {
                result.emplace_back(k, entry.value);
            }
            ++it;
        }
        return result;
    }

    std::optional<std::pair<std::string, std::string>> removeOneExpiredEntry() {
        while (!ttl_controller_.empty()) {
            auto ref = ttl_controller_.top();
            ttl_controller_.pop();

            auto it = base_storage_.find(ref.key);
            if (it != base_storage_.end() &&
                Clock::now() >= ref.expire_time &&
                it->second.expire_time == ref.expire_time) 
            {
                auto value = it->second.value;
                remove(ref.key);
                return std::make_pair(ref.key, value);
            }
        }
        return std::nullopt;
    }
};
