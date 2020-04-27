

struct source_location
{
    const char* file_name;
    unsigned line_number;
    const char* function_name;
};

#define CUR_SOURCE_LOCATION                                                                        \
    source_location                                                                                \
    {                                                                                              \
        __FILE__, __LINE__, __func__                                                               \
    }

void doAssert(bool expr, const source_location& loc, const char* expression)
{
    if (!expr)
    {
        // handle failed assertion
        std::abort();
    }
}

#if DEBUG_ASSERT_ENABLED
#define OE_DEBUG_ASSERT(Expr) doAssert(expr, CUR_SOURCE_LOCATION, #Expr)
#else
#define OE_DEBUG_ASSERT(Expr)
#endif