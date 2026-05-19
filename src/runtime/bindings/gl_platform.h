#pragma once

#ifdef VITA_BUILD
#  include <vitaGL.h>
typedef void (*GLADloadfunc)(void);
#else
#  include <glad/gl.h>
#endif
