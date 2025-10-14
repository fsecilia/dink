/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "container.hpp"
#include <dink/test.hpp>
#include <any>

namespace dink {
namespace {

struct container_test_t : Test
{
    struct cache_t
    {};

    struct mock_cache_traits_t
    {
        MOCK_METHOD(std::any, find_in_cache, (std::any cache));
        MOCK_METHOD(std::any, resolve_from_cache, (std::any cache, std::any factory));

        virtual ~mock_cache_traits_t() = default;
    };
    StrictMock<mock_cache_traits_t> mock_cache_traits;

    struct cache_traits_t
    {
        template <typename request_t, typename cache_t>
        auto find_in_cache(cache_t& cache) noexcept -> std::remove_cvref_t<request_t>*
        {
            return std::any_cast<std::remove_cvref_t<request_t>*>(mock->find_in_cache(std::any{&cache}));
        }

        template <typename request_t, typename provided_t, typename cache_t, typename factory_t>
        auto resolve_from_cache(cache_t& cache, factory_t&& factory) -> provided_t
        {
            if constexpr (std::is_reference_v<provided_t>)
            {
                return *std::any_cast<std::remove_reference_t<provided_t>*>(
                    mock->resolve_from_cache(std::any{&cache}, std::any{&factory})
                );
            }
            else return std::any_cast<provided_t>(mock->resolve_from_cache(std::any{&cache}, std::any{&factory}));
        }

        mock_cache_traits_t* mock = nullptr;
    };

    struct mock_delegate_t
    {
        MOCK_METHOD(std::any, delegate, ());
        virtual ~mock_delegate_t() = default;
    };
    StrictMock<mock_delegate_t> mock_delegate;

    struct delegate_t
    {
        template <typename request_t, typename>
        auto delegate() -> decltype(auto)
        {
            if constexpr (std::is_reference_v<request_t>)
            {
                return *std::any_cast<std::remove_reference_t<request_t>*>(mock->delegate());
            }
            else return std::any_cast<request_t>(mock->delegate());
        }
        mock_delegate_t* mock = nullptr;
    };

    struct mock_accessor_provider_t
    {
        MOCK_METHOD(std::any, get, (), (const));
        virtual ~mock_accessor_provider_t() = default;
    };
    StrictMock<mock_accessor_provider_t> mock_accessor_provider;

    template <typename instance_t>
    struct accessor_provider_t
    {
        using provided_t = std::remove_cvref_t<instance_t>;

        auto get() const -> instance_t
        {
            if constexpr (std::is_reference_v<instance_t>)
            {
                return *std::any_cast<std::remove_reference_t<instance_t>*>(mock->get());
            }
            else return std::any_cast<instance_t>(mock->get());
        }

        mock_accessor_provider_t* mock = nullptr;
    };

    struct mock_creator_provider_t
    {
        MOCK_METHOD(std::any, create, (std::any container));
        virtual ~mock_creator_provider_t() = default;
    };
    StrictMock<mock_creator_provider_t> mock_creator_provider;

    template <typename provided_t>
    struct creator_provider_t
    {
        template <typename dependency_chain_t, typename container_t>
        auto create(container_t& container) -> provided_t
        {
            if constexpr (std::is_reference_v<provided_t>)
            {
                return *std::any_cast<std::remove_reference_t<provided_t>*>(mock->create(std::any{&container}));
            }
            else return std::any_cast<provided_t>(mock->create(std::any{&container}));
        }

        mock_creator_provider_t* mock = nullptr;
    };

    struct default_provider_factory_t
    {
        template <typename provided_t>
        auto create() -> creator_provider_t<provided_t>
        {
            return creator_provider_t<provided_t>{mock_creator_provider};
        }

        mock_creator_provider_t* mock_creator_provider = nullptr;
    };

    struct mock_request_traits_t
    {
        MOCK_METHOD(std::any, as_requested, (std::any source));

        virtual ~mock_request_traits_t() = default;
    };
    StrictMock<mock_request_traits_t> mock_request_traits;

    struct request_traits_t
    {
        // sonnet's current version
        template <typename request_t, typename source_t>
        auto as_requested(source_t&& source) -> request_t
        {
            if constexpr (std::is_reference_v<request_t>)
            {
                auto* ptr = std::any_cast<std::remove_reference_t<request_t>*>(mock->as_requested(std::any{&source}));
                return *ptr;
            }
            else { return *std::any_cast<std::remove_reference_t<request_t>*>(mock->as_requested(std::any{&source})); }
        }

        mock_request_traits_t* mock = nullptr;
    };
};

// ---------------------------------------------------------------------------------------------------------------------

struct container_test_without_binding_t : container_test_t
{
    struct config_t
    {
        template <typename>
        auto find_binding() -> not_found_t
        {
            return not_found;
        }
    };

    struct policy_t
    {
        using cache_t = cache_t;
        using cache_traits_t = cache_traits_t;
        using delegate_t = delegate_t;
        using default_provider_factory_t = default_provider_factory_t;
        using request_traits_t = request_traits_t;
    };

    using sut_t = container_t<policy_t, config_t>;
    sut_t sut{
        cache_t{},
        cache_traits_t{&mock_cache_traits},
        config_t{},
        delegate_t{&mock_delegate},
        default_provider_factory_t{&mock_creator_provider},
        request_traits_t{&mock_request_traits}
    };
};

// ---------------------------------------------------------------------------------------------------------------------

struct container_test_with_bound_accessor_t : container_test_t
{
    struct provided_t
    {};
    using provider_t = accessor_provider_t<provided_t&>;
    static_assert(provider::is_accessor<provider_t>);

    using binding_t = binding_t<provided_t, provided_t, provider_t, scope::default_t>;
    static_assert(provider::is_accessor<typename std::remove_pointer_t<binding_t>::provider_type>);

    struct config_t
    {
        binding_t binding;

        template <typename>
        auto find_binding() -> binding_t*
        {
            return &binding;
        }
    };
    static_assert(is_config<config_t>);

    struct policy_t
    {
        using cache_t = cache_t;
        using cache_traits_t = cache_traits_t;
        using delegate_t = delegate_t;
        using default_provider_factory_t = default_provider_factory_t;
        using request_traits_t = request_traits_t;
    };

    using sut_t = container_t<policy_t, config_t>;
    sut_t sut{
        cache_t{},
        cache_traits_t{&mock_cache_traits},
        config_t{provider_t{&mock_accessor_provider}},
        delegate_t{&mock_delegate},
        default_provider_factory_t{&mock_creator_provider},
        request_traits_t{&mock_request_traits}
    };
};

TEST_F(container_test_with_bound_accessor_t, accessor_provider_bypasses_everything)
{
    auto expected_provided = provided_t{};

    EXPECT_CALL(mock_accessor_provider, get()).WillOnce(Return(std::any{&expected_provided}));
    EXPECT_CALL(mock_request_traits, as_requested(_)).WillOnce(ReturnArg<0>());

    ASSERT_EQ(&expected_provided, &sut.resolve<provided_t&>());
}

} // namespace
} // namespace dink
