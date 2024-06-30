//
// string class which offers a special feature vs. std::string:
// - the internal buffer can be read/WRITE accessed by external C/C++ code via the data() method,
//   allowing userland code to use arbitrary, fast, C- code to edit the string content,
//   including, f.e., injecting NUL sentinels a la strtok() et al.
// - built-in whitespace trimming methods.
//

#include <parameters/CString.hpp>

namespace parameters {

}
