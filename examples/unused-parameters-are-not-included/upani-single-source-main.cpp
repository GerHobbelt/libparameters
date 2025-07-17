// ditto, but now we have all *defined* parameters
// in a single source/compile unit 
// ==>
// this showcases whether your compiler/settings
// split a single source into multiple 
// independent objects/particles for the linker
// to include or discard in the final application.


#include "a.cpp"

#include "b.cpp"

#include "c.cpp"

#include "d.cpp"

#define SINGLE_SOURCE_DEMO 1
#include "upani-main.cpp"

