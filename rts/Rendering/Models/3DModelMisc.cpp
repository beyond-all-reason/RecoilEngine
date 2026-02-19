#include "3DModelMisc.hpp"
#include "3DModelVAO.hpp"

void S3DModelHelpers::BindLegacyAttrVBOs()
{
	S3DModelVAO::GetInstance().BindLegacyVertexAttribsAndVBOs();
}
void S3DModelHelpers::UnbindLegacyAttrVBOs()
{
	S3DModelVAO::GetInstance().UnbindLegacyVertexAttribsAndVBOs();
}