
#pragma once

// This file should be included at the very beginning of any
// file which would like to use any of the metaprogramming
// features. With these defines we can essentially reserve keywords
// for our metaprogram and make the compiler ignore them.

#define introspect(...)
#define nice_name(name)  // For redefining the name to be shown in properties panel etc.

