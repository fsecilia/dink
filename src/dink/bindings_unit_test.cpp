/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include <dink/test.hpp>
#include <dink/bindings.hpp>

namespace dink::bindings {
namespace {

struct bindings_test_t : Test
{
    using resolved_t = int_t;
    inline static auto resolved = resolved_t{11};
};

// ------------------------------------------------------------------------------------------------------------------ //

struct transient_bindings_test_t : bindings_test_t
{
    using sut_t = transient_t<resolved_t>;
    sut_t sut{};
};

// ------------------------------------------------------------------------------------------------------------------ //

struct transient_bindings_test_unbound_t : transient_bindings_test_t
{};

TEST_F(transient_bindings_test_unbound_t, initially_unbound)
{
    ASSERT_FALSE(sut.is_bound());
}

TEST_F(transient_bindings_test_unbound_t, unbind_does_not_crash)
{
    sut.unbind();
}

TEST_F(transient_bindings_test_unbound_t, unbind_multiple_times_does_not_crash)
{
    sut.unbind();
    sut.unbind();
}

TEST_F(transient_bindings_test_unbound_t, bind)
{
    sut.bind(resolved);
    ASSERT_TRUE(sut.is_bound());
}

// ------------------------------------------------------------------------------------------------------------------ //

struct transient_bindings_test_bound_t : transient_bindings_test_t
{
    transient_bindings_test_bound_t() { sut.bind(resolved); }
};

TEST_F(transient_bindings_test_bound_t, bound_bind_replaces)
{
    auto replacement = resolved + 1;
    sut.bind(replacement);
    ASSERT_EQ(replacement, sut.bound());
}

TEST_F(transient_bindings_test_bound_t, bound)
{
    ASSERT_EQ(resolved, sut.bound());
}

TEST_F(transient_bindings_test_bound_t, unbind)
{
    sut.unbind();
    ASSERT_FALSE(sut.is_bound());
}

TEST_F(transient_bindings_test_bound_t, unbind_multiple_times_does_not_crash)
{
    sut.unbind();
    sut.unbind();
}

// ------------------------------------------------------------------------------------------------------------------ //

struct shared_bindings_test_t : bindings_test_t
{
    using sut_t = shared_t<resolved_t>;
    sut_t sut{};
};

// ------------------------------------------------------------------------------------------------------------------ //

struct shared_bindings_test_unbound_t : shared_bindings_test_t
{};

TEST_F(shared_bindings_test_unbound_t, initially_unbound)
{
    ASSERT_FALSE(sut.is_bound());
}

TEST_F(shared_bindings_test_unbound_t, unbind_does_not_crash)
{
    sut.unbind();
}

TEST_F(shared_bindings_test_unbound_t, unbind_multiple_times_does_not_crash)
{
    sut.unbind();
    sut.unbind();
}

TEST_F(shared_bindings_test_unbound_t, bind)
{
    sut.bind(resolved);
    ASSERT_TRUE(sut.is_bound());
}

// ------------------------------------------------------------------------------------------------------------------ //

struct shared_bindings_test_bound_t : shared_bindings_test_t
{
    shared_bindings_test_bound_t() { sut.bind(resolved); }
};

TEST_F(shared_bindings_test_bound_t, bound_bind_replaces)
{
    auto replacement = resolved + 1;
    sut.bind(replacement);
    ASSERT_EQ(replacement, sut.bound());
}

TEST_F(shared_bindings_test_bound_t, bound)
{
    ASSERT_EQ(resolved, sut.bound());
}

TEST_F(shared_bindings_test_bound_t, unbind)
{
    sut.unbind();
    ASSERT_FALSE(sut.is_bound());
}

TEST_F(shared_bindings_test_bound_t, unbind_multiple_times_does_not_crash)
{
    sut.unbind();
    sut.unbind();
}

} // namespace
} // namespace dink::bindings
