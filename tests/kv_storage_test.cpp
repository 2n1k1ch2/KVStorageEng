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