/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "cache_line_size.hpp"
#include <dink/test.hpp>

namespace dink::cache_line_size {
namespace {

struct cache_line_size_test_t : Test
{
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
};

#if groundwork_cache_line_size_impl == groundwork_cache_line_size_impl_posix

struct posix_cache_line_size_test_t : cache_line_size_test_t
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

TEST_F(posix_cache_line_size_test_t, succeed)
{
    auto const expected = std::size_t{128};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_name)).WillOnce(Return(static_cast<int>(expected)));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

TEST_F(posix_cache_line_size_test_t, failed_sysconf_uses_fallback)
{
    auto const expected = std::size_t{64};
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_name)).WillOnce(Return(static_cast<int>(0)));
    EXPECT_CALL(mock_fallback, call_operator()).WillOnce(Return(expected));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

#endif

} // namespace
} // namespace dink::cache_line_size
