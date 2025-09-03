/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#include "type_erased_storage.hpp"
#include <dink/test.hpp>
#include <array>
#include <cstdlib>

namespace dink {
namespace {

struct type_erased_storage_test_t : Test
{
    struct mock_allocator_t
    {
        MOCK_METHOD(void*, allocate, (std::size_t size, std::align_val_t alignment));
        MOCK_METHOD(void, roll_back, (), (noexcept));
        virtual ~mock_allocator_t() = default;
    };
    StrictMock<mock_allocator_t> mock_allocator;

    struct allocator_t
    {
        auto allocate(std::size_t size, std::align_val_t alignment) -> void* { return mock->allocate(size, alignment); }
        auto roll_back() noexcept -> void { return mock->roll_back(); }
        mock_allocator_t* mock = nullptr;
    };

    using sut_t = type_erased_storage_t<allocator_t>;
};

// ---------------------------------------------------------------------------------------------------------------------

struct type_erased_storage_test_construction_t : type_erased_storage_test_t
{
    struct instance_t
    {};

    struct move_only_instance_t
    {
        move_only_instance_t() noexcept = default;
        move_only_instance_t(move_only_instance_t const&) = delete;
        auto operator=(move_only_instance_t const&) -> move_only_instance_t& = delete;
        move_only_instance_t(move_only_instance_t&&) noexcept = default;
        auto operator=(move_only_instance_t&&) noexcept -> move_only_instance_t& = default;
    };

    union storage_t
    {
        instance_t instance;
        move_only_instance_t move_only_instance;

        storage_t() noexcept {}
        ~storage_t() {}
    };
    storage_t storage;

    sut_t sut{allocator_t{&mock_allocator}};
};

TEST_F(type_erased_storage_test_construction_t, construction_succeeds)
{
    EXPECT_CALL(mock_allocator, allocate(sizeof(instance_t), std::align_val_t{alignof(instance_t)}))
        .WillOnce(Return(&storage.instance));

    ASSERT_EQ(&storage.instance, &sut.template create<instance_t>());
}

TEST_F(type_erased_storage_test_construction_t, construction_of_move_only_type_succeeds)
{
    EXPECT_CALL(mock_allocator, allocate(sizeof(move_only_instance_t), std::align_val_t{alignof(move_only_instance_t)}))
        .WillOnce(Return(&storage.move_only_instance));

    ASSERT_EQ(&storage.move_only_instance, &sut.template create<move_only_instance_t>());
}

TEST_F(type_erased_storage_test_construction_t, throw_on_allocate)
{
    EXPECT_CALL(mock_allocator, allocate(sizeof(instance_t), std::align_val_t{alignof(instance_t)}))
        .WillOnce(Throw(std::bad_alloc{}));

    EXPECT_THROW(sut.template create<instance_t>(), std::bad_alloc);

    /*
        There isn't much visible to test here. Since the allocation threw, the new, partial instance will be popped as
        part of rolling back, so it's dtor function won't be called during destruction. If it were, it would be called
        with nullptr, and typed_dtor() asserts it is not null. As long as that assert isn't hit, this was rolled back
        correctly.
    */
}

TEST_F(type_erased_storage_test_construction_t, throw_on_ctor_rolls_back)
{
    EXPECT_CALL(mock_allocator, allocate(sizeof(instance_t), std::align_val_t{alignof(instance_t)}))
        .WillOnce(Return(&storage.instance));
    EXPECT_CALL(mock_allocator, roll_back());

    struct throwing_instance_t
    {
        throwing_instance_t() { throw std::runtime_error(""); }
    };

    EXPECT_THROW(sut.template create<throwing_instance_t>(), std::runtime_error);
}

// ---------------------------------------------------------------------------------------------------------------------

struct type_erased_storage_test_construction_params_t : type_erased_storage_test_t
{
    using trivial_ctor_param_t = int_t;
    using nontrivial_ctor_param_t = std::string;
    using move_only_ctor_param_t = std::unique_ptr<bool>;

    struct ctor_params_spy_t
    {
        trivial_ctor_param_t trivial_ctor_param;
        nontrivial_ctor_param_t nontrivial_ctor_param;
        move_only_ctor_param_t move_only_ctor_param;
    };

    trivial_ctor_param_t const expected_trivial_ctor_param = 3;
    nontrivial_ctor_param_t const expected_nontrivial_ctor_param = "nontrivial_ctor_param";
    bool const expected_move_only_ctor_param_value = true;

    union storage_t
    {
        ctor_params_spy_t ctor_params_spy;

        storage_t() noexcept {}
        ~storage_t() {}
    };
    storage_t storage;

    sut_t sut{allocator_t{&mock_allocator}};
};

TEST_F(type_erased_storage_test_construction_params_t, ctor_params_are_forwarded_correctly)
{
    EXPECT_CALL(mock_allocator, allocate(sizeof(ctor_params_spy_t), std::align_val_t{alignof(ctor_params_spy_t)}))
        .WillOnce(Return(&storage.ctor_params_spy));

    auto& result = sut.template create<ctor_params_spy_t>(
        expected_trivial_ctor_param, expected_nontrivial_ctor_param,
        std::make_unique<bool>(expected_move_only_ctor_param_value)
    );

    ASSERT_EQ(&storage.ctor_params_spy, &result);
    ASSERT_EQ(expected_trivial_ctor_param, result.trivial_ctor_param);
    ASSERT_EQ(expected_nontrivial_ctor_param, result.nontrivial_ctor_param);
    ASSERT_EQ(expected_move_only_ctor_param_value, *result.move_only_ctor_param);
}

TEST_F(type_erased_storage_test_construction_params_t, alignment_is_forwarded_correctly)
{
    auto const alignment = alignof(ctor_params_spy_t) << 1;
    EXPECT_CALL(mock_allocator, allocate(sizeof(ctor_params_spy_t), std::align_val_t{alignment}))
        .WillOnce(Return(&storage.ctor_params_spy));

    auto& result = sut.template create<ctor_params_spy_t>(
        std::align_val_t{alignment}, expected_trivial_ctor_param, expected_nontrivial_ctor_param,
        std::make_unique<bool>(expected_move_only_ctor_param_value)
    );

    ASSERT_EQ(&storage.ctor_params_spy, &result);
    ASSERT_EQ(expected_trivial_ctor_param, result.trivial_ctor_param);
    ASSERT_EQ(expected_nontrivial_ctor_param, result.nontrivial_ctor_param);
    ASSERT_EQ(expected_move_only_ctor_param_value, *result.move_only_ctor_param);
}

// ---------------------------------------------------------------------------------------------------------------------

struct type_erased_storage_test_destruction_order_t : type_erased_storage_test_t
{
    struct mock_dtor_tracker_t
    {
        MOCK_METHOD(void, dtor_called, (), (noexcept));
        virtual ~mock_dtor_tracker_t() = default;
    };
    std::array<StrictMock<mock_dtor_tracker_t>, 3> mock_dtor_trackers{};

    struct dtor_tracker_t
    {
        ~dtor_tracker_t() { mock->dtor_called(); }

        mock_dtor_tracker_t* mock = nullptr;
    };

    union dtor_tracker_storage_t
    {
        std::array<dtor_tracker_t, 3> instances;

        dtor_tracker_storage_t() noexcept {}
        ~dtor_tracker_storage_t() {}
    };
    dtor_tracker_storage_t dtor_tracker_storage;

    // this must be instantiated after mock_dtor_trackers so its dtor runs before the mocks are destroyed
    sut_t sut{allocator_t{&mock_allocator}};
};

TEST_F(type_erased_storage_test_destruction_order_t, instance_destroyed_in_reverse_order_of_construction)
{
    auto seq = InSequence{}; // orderered

    EXPECT_CALL(mock_allocator, allocate(sizeof(dtor_tracker_t), std::align_val_t{alignof(dtor_tracker_t)}))
        .WillOnce(Return(&dtor_tracker_storage.instances[0]))
        .WillOnce(Return(&dtor_tracker_storage.instances[1]))
        .WillOnce(Return(&dtor_tracker_storage.instances[2]));

    EXPECT_CALL(mock_dtor_trackers[2], dtor_called);
    EXPECT_CALL(mock_dtor_trackers[1], dtor_called);
    EXPECT_CALL(mock_dtor_trackers[0], dtor_called);

    auto& instance_0 = sut.template create<dtor_tracker_t>(&mock_dtor_trackers[0]);
    auto& instance_1 = sut.template create<dtor_tracker_t>(&mock_dtor_trackers[1]);
    auto& instance_2 = sut.template create<dtor_tracker_t>(&mock_dtor_trackers[2]);

    ASSERT_EQ(&instance_0, &dtor_tracker_storage.instances[0]);
    ASSERT_EQ(&instance_1, &dtor_tracker_storage.instances[1]);
    ASSERT_EQ(&instance_2, &dtor_tracker_storage.instances[2]);
}

} // namespace
} // namespace dink
