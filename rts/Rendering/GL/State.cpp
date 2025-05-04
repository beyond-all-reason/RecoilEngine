#include "State.h"

decltype(GL::State::Attributes) GL::State::Attributes;

void GL::myGL::PixelStoreUnpackAlignment(GLint alignment)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}