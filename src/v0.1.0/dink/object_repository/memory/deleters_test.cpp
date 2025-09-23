/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "deleters.hpp"
#include <dink/test.hpp>

namespace dink::deleters {
namespace {

struct delegating_deleter_test_t : Test
{
    struct instance_t
    {
        instance_t* prev = nullptr;
    };

    struct mock_deleter_t
    {
        MOCK_METHOD(void, call, (instance_t * instance), (const, noexcept));
        virtual ~mock_deleter_t() = default;
    };

    struct deleter_t
    {
        auto operator()(instance_t* instance) const noexcept -> void { mock->call(instance); }
        mock_deleter_t* mock = nullptr;
    };
};

// ---------------------------------------------------------------------------------------------------------------------

struct composite_deleter_test_t : delegating_deleter_test_t
{
    StrictMock<mock_deleter_t> mock_deleter_1{};
    StrictMock<mock_deleter_t> mock_deleter_2{};
    StrictMock<mock_deleter_t> mock_deleter_3{};

    struct deleter_1_t : deleter_t
    {};

    struct deleter_2_t : deleter_t
    {};

    struct deleter_3_t : deleter_t
    {};

    instance_t instance{};

    using sut_t = composite_t<instance_t, deleter_1_t, deleter_2_t, deleter_3_t>;
    sut_t sut{deleter_1_t{&mock_deleter_1}, deleter_2_t{&mock_deleter_2}, deleter_3_t{&mock_deleter_3}};

    InSequence seq{};
};

TEST_F(composite_deleter_test_t, call_operator_calls_all_deleters_in_order)
{
    EXPECT_CALL(mock_deleter_1, call(&instance));
    EXPECT_CALL(mock_deleter_2, call(&instance));
    EXPECT_CALL(mock_deleter_3, call(&instance));

    sut(&instance);
}

// ---------------------------------------------------------------------------------------------------------------------

struct destroying_deleter_test_t : Test
{
    struct mock_instance_t
    {
        MOCK_METHOD(void, dtor, (), (noexcept));
        virtual ~mock_instance_t() = default;
    };
    StrictMock<mock_instance_t> mock_instance;

    struct instance_t
    {
        ~instance_t() { mock->dtor(); }
        mock_instance_t* mock = nullptr;
    };

    union
    {
        instance_t instance;
    };

    using sut_t = destroying_t<instance_t>;
    sut_t sut;

    destroying_deleter_test_t() {}
    ~destroying_deleter_test_t() override {}
};

TEST_F(destroying_deleter_test_t, call_operator_calls_dtor)
{
    // use placement new to manually call ctor without automatically calling dtor
    new (&instance) instance_t{&mock_instance};

    EXPECT_CALL(mock_instance, dtor());

    sut(&instance);
}

} // namespace
} // namespace dink::deleters
