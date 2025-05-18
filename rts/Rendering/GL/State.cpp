#include "State.h"

decltype(GL::State::Attributes) GL::State::Attributes;

void GL::myGL::PixelStoreUnpackAlignment(GLint alignment)
{
	assert(0 < alignment && alignment <= 4);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}