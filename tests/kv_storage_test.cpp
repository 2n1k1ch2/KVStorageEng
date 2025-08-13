#include <gtest/gtest.h>
#include "kvstorage.hpp"
#include <chrono>
#include <span>
struct MockClock {
    using time_point = std::chrono::steady_clock::time_point;
    using duration   = std::chrono::steady_clock::duration;
    using rep        = std::chrono::steady_clock::rep;
    using period     = std::chrono::steady_clock::period;

    static time_point now() { return start_time + offset; }

    static time_point start_time;
    static std::chrono::seconds offset;
};
MockClock::time_point MockClock::start_time = std::chrono::steady_clock::now();
std::chrono::seconds MockClock::offset{0};

TEST(KVStorageTest, Get) {
    using Clock = std::chrono::steady_clock;

    std::tuple<std::string, std::string, uint32_t> init_data[] = {
        {"key1", "value1", 0}
    };

    KVStorage<Clock> storage(std::span{init_data});

    auto result = storage.get("key1");
    auto resultFalse = storage.get("key2");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1");

    ASSERT_FALSE(resultFalse.has_value());
};
TEST(KVStorageTest, Set) {
    using Clock = std::chrono::steady_clock;

    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});

    storage.set("key1","value1",0);
    storage.set("key1","value2",0);
    auto result = storage.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value2");

};
TEST(KVStorageTest, Remove) {
    using Clock = std::chrono::steady_clock;

    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});

    storage.set("key1","value1",0);

    EXPECT_EQ(storage.remove("key1"),true);
    EXPECT_EQ(storage.remove("key2"),false);

    
};
TEST(KVStoraeTest, GetManySorted ) {
    using Clock = std::chrono::steady_clock;

    std::tuple<std::string, std::string, uint32_t> init_data1[] = {
        {"a", "1", 0},
        {"b", "2", 0},
        {"c", "3", 0},
        {"d", "4", 0},
        {"e", "5", 0},
        {"f", "6", 0},
        {"g", "7", 0},
        {"h", "8", 0},
        {"i", "9", 0},
        {"j", "10", 0}
    };
    KVStorage<Clock> storage(std::span{init_data1});

    auto result = storage.getManySorted("e",4);
    std::vector<std::pair<std::string, std::string>> want_result={{"e","5"}, {"f","6"},{"g","7"},{"h","8"}};
    EXPECT_EQ(result, want_result);

    auto result_start = storage.getManySorted("a", 3);
    std::vector<std::pair<std::string, std::string>> want_start = {
        {"a","1"}, {"b","2"}, {"c","3"}
    };
    EXPECT_EQ(result_start, want_start);

    auto result_end = storage.getManySorted("i", 4);
    std::vector<std::pair<std::string, std::string>> want_end = {
        {"i","9"}, {"j","10"}
    };
    EXPECT_EQ(result_end, want_end);

    auto result_missing = storage.getManySorted("z", 3);
    std::vector<std::pair<std::string, std::string>> want_missing = {}; 
    EXPECT_EQ(result_missing, want_missing);


    auto result_one = storage.getManySorted("c", 1);
    std::vector<std::pair<std::string, std::string>> want_one = {{"c","3"}};
    EXPECT_EQ(result_one, want_one);
};

TEST(KVStorageTest, removeOneExpiredEntry) {
    using Clock = MockClock;

    std::tuple<std::string, std::string, uint32_t> init_data[] = {
        {"a", "1", 60},
        {"b", "2", 61},
        {"c", "3", 62},
        {"d", "4", 63},
        {"e", "5", 64},
        {"f", "6", 0},
        {"g", "7", 0},
        {"h", "8", 0},
        {"i", "9", 0},
        {"j", "10", 0}
    };
    
    KVStorage<Clock> storage(std::span{init_data});
    MockClock::offset = std::chrono::seconds(64);

    while(storage.removeOneExpiredEntry()) {}
    auto result = storage.getManySorted("a", 10);
    std::vector<std::pair<std::string, std::string>> want_result = {
        {"f","6"}, {"g","7"}, {"h","8"}, {"i","9"}, {"j","10"}
    };
    EXPECT_EQ(result, want_result);
};

TEST(KVStorageTest, ZeroExpiredExpect) {
    using Clock = std::chrono::steady_clock;
    std::tuple<std::string, std::string, uint32_t> init_data[] = {
        {"f", "6", 0},
        {"g", "7", 0},
        {"h", "8", 0},
        {"i", "9", 0},
        {"j", "10", 0}
    };
    KVStorage<Clock> storage(std::span{init_data});
    for (int i = 0;i<6;i++){
        auto  result= storage.removeOneExpiredEntry();
        EXPECT_FALSE(result.has_value());
    }
}
TEST(KVStorageTest, ChangedTTLinExistingEntry) {
    using Clock = MockClock;

    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});
    storage.set("key","value",30);
    storage.set("key","value",100);
    MockClock::offset = std::chrono::seconds(50);
    storage.removeOneExpiredEntry();
    auto result =storage.get("key");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result, "value");
};
TEST(KVStorageTest, EmptyStorage) {
    using Clock = std::chrono::steady_clock;
    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});
    auto result_get = storage.get("key");
    auto result_getMany = storage.getManySorted("key",3);
    auto result_remove = storage.remove("key");
    auto result_removeExpiredOne=storage.removeOneExpiredEntry();
    EXPECT_FALSE(result_get.has_value());
    EXPECT_TRUE(result_getMany.empty());
    EXPECT_FALSE(result_remove);
    EXPECT_FALSE(result_removeExpiredOne.has_value());
}
TEST(KVStorageTest, RemoveAndExpiredEntry) {
    using Clock = MockClock;
    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});
    storage.set("key","value",30);
    EXPECT_TRUE(storage.remove("key"));
    MockClock::offset=std::chrono::seconds(30);
    EXPECT_FALSE(storage.removeOneExpiredEntry());
}
TEST(KVStorageTest, SortedStorageCheck){
    using Clock = std::chrono::steady_clock;

    std::tuple<std::string, std::string, uint32_t> init_data1[] = {
        {"j", "1", 0},
        {"i", "2", 0},
        {"h", "3", 0},
        {"g", "4", 0},
        {"f", "5", 0},
        {"e", "6", 0},
        {"d", "7", 0},
        {"c", "8", 0},
        {"b", "9", 0},
        {"a", "10", 0}
    };
    KVStorage<Clock> storage(std::span{init_data1});

    auto result = storage.getManySorted("e",4);
    std::vector<std::pair<std::string, std::string>> want_result={{"e","6"}, {"f","5"},{"g","4"},{"h","3"}};
    EXPECT_EQ(result, want_result);
}
TEST(KVStorageTest, ManyUpdatesSameKey) {
    using Clock = MockClock;
    KVStorage<Clock> storage(std::span<std::tuple<std::string, std::string, uint32_t>>{});
    for (uint32_t i = 1; i <= 999; ++i) {
        storage.set("key", "value" + std::to_string(i), i);
    }
    storage.set("key","value1010",1010);
    MockClock::offset = std::chrono::seconds(1000);
    while (storage.removeOneExpiredEntry()) {}
    auto result = storage.get("key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1010");
}
