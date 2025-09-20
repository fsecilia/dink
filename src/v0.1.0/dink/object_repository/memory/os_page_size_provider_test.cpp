/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "os_page_size_provider.hpp"
#include <dink/test.hpp>

namespace dink {
namespace posix {
namespace {

#if dink_page_size_provider_impl == dink_page_size_provider_impl_posix

struct posix_page_size_provider_test_t : Test
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

    using sut_t = page_size_provider_t<api_t>;
    sut_t sut{api};
};

TEST_F(posix_page_size_provider_test_t, succeed)
{
    auto const expected = fallback_page_size_provider * 2;
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_page_size_provider_name)).WillOnce(Return(static_cast<int>(expected)));

    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

TEST_F(posix_page_size_provider_test_t, failed_sysconf_uses_fallback)
{
    EXPECT_CALL(mock_api, sysconf(sut_t::sysconf_page_size_provider_name)).WillOnce(Return(static_cast<int>(0)));

    auto const expected = fallback_page_size_provider;
    auto const actual = sut();
    ASSERT_EQ(expected, actual);
}

#endif

} // namespace
} // namespace posix
} // namespace dink
