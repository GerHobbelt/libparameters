// 
// Support code for FMT library usage within and without libparameters.
//

#ifndef PARAMETERS_FMT_SUPPORT_H_
#define PARAMETERS_FMT_SUPPORT_H_

#include <fmt/base.h>
#include <fmt/format.h>


#define DECL_FMT_FORMAT_PARAMENUMTYPE(Type)                                                  \
                                                                                             \
} /* close current namespace parameters */                                                   \
                                                                                             \
namespace fmt {                                                                              \
                                                                                             \
  template <>                                                                                \
  struct formatter<parameters::Type> : formatter<std::string_view> {                         \
    /* parse is inherited from formatter<string_view>. */                                    \
                                                                                             \
    auto format(parameters::Type c, format_context &ctx) const -> decltype(ctx.out());       \
  };                                                                                         \
                                                                                             \
}                                                                                            \
                                                                                             \
namespace parameters {                                                                       \
  /* re-open namepsace parameters */


#define DECL_FMT_FORMAT_PARAMOBJTYPE(Type)                                                   \
                                                                                             \
} /* close current namespace parameters */                                                   \
                                                                                             \
namespace fmt {                                                                              \
                                                                                             \
  template <>                                                                                \
  struct formatter<parameters::Type> : formatter<std::string_view> {                         \
    /* parse is inherited from formatter<string_view>. */                                    \
                                                                                             \
    auto format(const parameters::Type &c, format_context &ctx) const -> decltype(ctx.out());        \
  };                                                                                         \
                                                                                             \
}                                                                                            \
                                                                                             \
namespace parameters {                                                                       \
  /* re-open namepsace parameters */


#endif 
