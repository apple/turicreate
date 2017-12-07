#ifndef EXPECT_C_STATIC_ASSERT
#error EXPECT_C_STATIC_ASSERT not defined
#endif
#ifndef EXPECT_C_FUNCTION_PROTOTYPES
#error EXPECT_C_FUNCTION_PROTOTYPES not defined
#endif
#ifndef EXPECT_C_RESTRICT
#error EXPECT_C_RESTRICT not defined
#endif

#if !EXPECT_C_STATIC_ASSERT
#if EXPECT_C_STATIC_ASSERT
#error "Expect c_static_assert feature"
#endif
#else
#if !EXPECT_C_STATIC_ASSERT
#error "Expect no c_static_assert feature"
#endif
#endif

#if !EXPECT_C_FUNCTION_PROTOTYPES
#error Expect c_function_prototypes support
#endif

#if !EXPECT_C_RESTRICT
#if EXPECT_C_RESTRICT
#error Expect c_restrict support
#endif
#else
#if !EXPECT_C_RESTRICT
#error Expect no c_restrict support
#endif
#endif

int main()
{
}
