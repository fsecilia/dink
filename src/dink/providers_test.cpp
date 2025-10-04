/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "providers.hpp"
#include <dink/test.hpp>

namespace dink::providers {
namespace {

struct providers_factory_test_t : Test
{
    static constexpr int_t unexpected_id = -1;

    struct provided_t
    {
        static constexpr int_t expected_id = 3;
        int_t id = unexpected_id;
    };

    struct provider_factory_t
    {
        static constexpr int_t expected_id = 5;
        int id = unexpected_id;
        auto operator()() const noexcept -> provided_t;
        auto operator==(provider_factory_t const&) const noexcept -> bool = default;
    };

    struct mock_resolver_t
    {
        MOCK_METHOD(provided_t, resolve, (provider_factory_t const& factory));
        ~mock_resolver_t() = default;
    };
    StrictMock<mock_resolver_t> mock_resolver;

    struct resolver_t
    {
        auto resolve(provider_factory_t const& factory) -> provided_t { return mock->resolve(factory); }
        mock_resolver_t* mock = nullptr;
    };
    resolver_t resolver{&mock_resolver};

    using sut_t = factory_t<provider_factory_t>;
    sut_t sut{provider_factory_t{provider_factory_t::expected_id}};
};

TEST_F(providers_factory_test_t, provide_passes_provider_factory_to_resolver_provide)
{
    EXPECT_CALL(mock_resolver, resolve(Eq(provider_factory_t{.id = provider_factory_t::expected_id})))
        .WillOnce(Return(provided_t{.id = provided_t::expected_id}));

    auto const result = sut(resolver);

    ASSERT_EQ(provided_t::expected_id, result.id);
}

} // namespace
} // namespace dink::providers
