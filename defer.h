#define __DEFER_CONCAT_HELPER( a, b ) a##b
#define __DEFER_CONCAT( a, b ) __DEFER_CONCAT_HELPER( a, b )

template< typename F >
struct ScopeExit {
    ScopeExit( F f_ ) : f( f_ ) { }
    ~ScopeExit() { f(); }
    F f;
};

#define defer(STATMENTS) ScopeExit __DEFER_CONCAT(DEFER_, __COUNTER__)([&](){STATMENTS})
