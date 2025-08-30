/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "memory.hpp"
#include <dink/test.hpp>

namespace dink::memory {
namespace {

struct memory_fallback_test_t : Test
{
    static constexpr auto is_nonzero_power_of_two(auto value) noexcept -> bool
    {
        return value && !(value & (value - 1));
    }
};

TEST_F(memory_fallback_test_t, cache_line_size_is_power_of_two)
{
    ASSERT_TRUE(is_nonzero_power_of_two(fallback::cache_line_size_t{}()));
}

TEST_F(memory_fallback_test_t, page_size_is_power_of_two)
{
    ASSERT_TRUE(is_nonzero_power_of_two(fallback::page_size_t{}()));
}

// ---------------------------------------------------------------------------------------------------------------------

#if groundwork_memory_impl == groundwork_memory_impl_posix

struct memory_posix_test_cache_line_size_t : Test
{
    struct mock_api_t
    {
        MOCK_METHOD(long int, sysconf, (int name), (const, noexcept));
        virtual ~mock_api_t() = default;
    };
    StrictMock<mock_api_t> mock_api;

    struct api_t
    {
        auto sysconf(int name) const noexcept -> long int { return mock->sysconf(name); }
        mock_api_t* mock = nullptr;
    };
    api_t api{&mock_api};

    struct mock_fallback_t
    {
        MOCK_METHOD(std::size_t, call_operator, (), (const, noexcept));
        virtual ~mock_fallback_t() = default;
    };
    StrictMock<mock_fallback_t> mock_fallback;

    struct fallback_t
    {
        auto operator()() const noexcept -> std::size_t { return mock->call_operator(); }
        mock_fallback_t* mock = nullptr;
    };
    fallback_t fallback{&mock_fallback};

    using sut_t = posix::cache_line_size_t<api_t, fallback_t>;
    sut_t sut{api, fallback};
};

TEST_F(memory_posix_test_cache_line_size_t, succeed)
{
    auto const expected = std::size_t{128};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_cache_line_size_name)).WillOnce(Return(static_cast<int>(expected)));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

TEST_F(memory_posix_test_cache_line_size_t, failed_sysconf_uses_fallback)
{
    auto const expected = std::size_t{64};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_cache_line_size_name)).WillOnce(Return(static_cast<int>(0)));
    EXPECT_CALL(mock_fallback, call_operator()).WillOnce(Return(expected));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

// ---------------------------------------------------------------------------------------------------------------------

struct memory_posix_test_page_size_t : Test
{
    struct mock_api_t
    {
        MOCK_METHOD(long int, sysconf, (int name), (const, noexcept));
        virtual ~mock_api_t() = default;
    };
    StrictMock<mock_api_t> mock_api;

    struct api_t
    {
        auto sysconf(int name) const noexcept -> long int { return mock->sysconf(name); }
        mock_api_t* mock = nullptr;
    };
    api_t api{&mock_api};

    struct mock_fallback_t
    {
        MOCK_METHOD(std::size_t, call_operator, (), (const, noexcept));
        virtual ~mock_fallback_t() = default;
    };
    StrictMock<mock_fallback_t> mock_fallback;

    struct fallback_t
    {
        auto operator()() const noexcept -> std::size_t { return mock->call_operator(); }
        mock_fallback_t* mock = nullptr;
    };
    fallback_t fallback{&mock_fallback};

    using sut_t = posix::page_size_t<api_t, fallback_t>;
    sut_t sut{api, fallback};
};

TEST_F(memory_posix_test_page_size_t, succeed)
{
    auto const expected = std::size_t{4096};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_page_size_name)).WillOnce(Return(static_cast<int>(expected)));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

TEST_F(memory_posix_test_page_size_t, failed_sysconf_uses_fallback)
{
    auto const expected = std::size_t{1024};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_page_size_name)).WillOnce(Return(static_cast<int>(0)));
    EXPECT_CALL(mock_fallback, call_operator()).WillOnce(Return(expected));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

#endif

} // namespace
} // namespace dink::memory
