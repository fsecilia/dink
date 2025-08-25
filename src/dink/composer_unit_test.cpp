/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/composer.hpp>

namespace dink {
namespace {

struct composer_test_t : Test
{
    using resolved_t = int_t;

    struct mock_transient_binding_t
    {
        MOCK_METHOD(void, bind, (resolved_t), (noexcept, const));
        MOCK_METHOD(bool, is_bound, (), (noexcept, const));
        MOCK_METHOD(void, unbind, (), (noexcept));
        MOCK_METHOD(resolved_t const&, bound, (), (const, noexcept));
        MOCK_METHOD(resolved_t&, bound, (), (noexcept));

        virtual ~mock_transient_binding_t() = default;
    };

    struct mock_shared_binding_t
    {
        MOCK_METHOD(void, bind, (resolved_t&), (noexcept, const));
        MOCK_METHOD(bool, is_bound, (), (noexcept, const));
        MOCK_METHOD(void, unbind, (), (noexcept));
        MOCK_METHOD(resolved_t const&, bound, (), (const, noexcept));
        MOCK_METHOD(resolved_t&, bound, (), (noexcept));

        virtual ~mock_shared_binding_t() = default;
    };

    struct transient_resolver_t
    {
        inline static constexpr resolved_t const expected_result = 3;
        inline static mock_transient_binding_t* binding{};

        template <typename resolved_t>
        constexpr auto bind(resolved_t&& resolved) -> void
        {
            binding->bind(std::forward<resolved_t>(resolved));
        }

        template <typename resolved_t>
        constexpr auto unbind() noexcept -> void
        {
            binding->unbind();
        }

        template <typename resolved_t>
        constexpr auto is_bound() const noexcept -> bool
        {
            return binding->is_bound();
        }

        template <typename resolved_t>
        constexpr auto bound() const noexcept -> resolved_t&
        {
            return binding->bound();
        }

        template <typename resolved_t, typename composer_t>
        constexpr auto resolve(composer_t&) const -> resolved_t
        {
            return expected_result;
        }
    };

    struct shared_resolver_t
    {
        using nested_t = shared_resolver_t;

        inline static resolved_t expected_result = 5;
        inline static mock_shared_binding_t* binding{};

        template <typename resolved_t>
        constexpr auto bind(resolved_t&& resolved) -> void
        {
            binding->bind(std::forward<resolved_t>(resolved));
        }

        template <typename resolved_t>
        constexpr auto unbind() noexcept -> void
        {
            binding->unbind();
        }

        template <typename resolved_t>
        constexpr auto is_bound() const noexcept -> bool
        {
            return binding->is_bound();
        }

        template <typename resolved_t>
        constexpr auto bound() const noexcept -> resolved_t&
        {
            return binding->bound();
        }

        template <typename resolved_t, typename composer_t>
        constexpr auto resolve(composer_t&) const -> resolved_t&
        {
            return expected_result;
        }
    };
    using sut_t = composer_t<transient_resolver_t, shared_resolver_t>;

    sut_t sut{};

    StrictMock<mock_transient_binding_t> transient_binding{};
    StrictMock<mock_shared_binding_t> shared_binding{};

    composer_test_t() noexcept
    {
        transient_resolver_t::binding = &transient_binding;
        shared_resolver_t::binding = &shared_binding;
    }
};

TEST_F(composer_test_t, bind_transient)
{
    auto resolved = resolved_t{};
    EXPECT_CALL(transient_binding, bind(resolved));
    sut.bind(resolved);
}

TEST_F(composer_test_t, resolve_transient)
{
    ASSERT_EQ(transient_resolver_t::expected_result, sut.template resolve<resolved_t>());
}

TEST_F(composer_test_t, resolve_shared)
{
    ASSERT_EQ(&shared_resolver_t::expected_result, &sut.template resolve<resolved_t&>());
}

} // namespace
} // namespace dink
