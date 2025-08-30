/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "memory.hpp"
#include <dink/test.hpp>

namespace dink::memory {
namespace {

struct memory_test_t : Test
{
    struct mock_fallback_t
    {
        MOCK_METHOD(std::size_t, cache_line_size, (), (const, noexcept));
        virtual ~mock_fallback_t() = default;
    };
    StrictMock<mock_fallback_t> mock_fallback;

    struct fallback_t
    {
        auto cache_line_size() const noexcept -> std::size_t { return mock->cache_line_size(); }
        mock_fallback_t* mock = nullptr;
    };
    fallback_t fallback{&mock_fallback};
};

// ---------------------------------------------------------------------------------------------------------------------

#if groundwork_memory_impl == groundwork_memory_impl_posix

struct posix_memory_test_t : memory_test_t
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

    using sut_t = posix::impl_t<api_t, fallback_t>;
    sut_t sut{api, fallback};
};

TEST_F(posix_memory_test_t, cache_line_size_succeed)
{
    auto const expected = std::size_t{128};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_name)).WillOnce(Return(static_cast<int>(expected)));

    auto const actual = sut.cache_line_size();
    ASSERT_EQ(expected, actual);
}

TEST_F(posix_memory_test_t, cache_line_size_failed_sysconf_uses_fallback)
{
    auto const expected = std::size_t{64};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_name)).WillOnce(Return(static_cast<int>(0)));
    EXPECT_CALL(mock_fallback, cache_line_size()).WillOnce(Return(expected));

    auto const actual = sut.cache_line_size();
    ASSERT_EQ(expected, actual);
}

#endif

} // namespace
} // namespace dink::memory
