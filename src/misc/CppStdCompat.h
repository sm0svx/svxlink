#ifndef CPP_STD_COMPAT_INCLUDED
#define CPP_STD_COMPAT_INCLUDED

#if __cplusplus >= 201102L
  #define CONSTEXPR constexpr
#else
  #define CONSTEXPR const
#endif

#endif /* CPP_STD_COMPAT_INCLUDED */
