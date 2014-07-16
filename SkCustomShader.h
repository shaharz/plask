#ifndef SkCustomShader_DEFINED
#define SkCustomShader_DEFINED

#include "core/SkShader.h"

class SK_API SkCustomShader : public SkShader {
public:
    static SkShader* Create(SkSize dims, SkScalar seed, const SkMatrix* localMatrix = NULL);
    
    
    virtual bool asNewEffect(GrContext* context, const SkPaint&, const SkMatrix*, GrColor*,
                             GrEffectRef**) const SK_OVERRIDE;
    
    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkCustomShader)
    
protected:
    SkCustomShader(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
   
private:
    SkCustomShader(SkSize dims, SkScalar seed, const SkMatrix* localMatrix = NULL);
    virtual ~SkCustomShader();
    
    /*const*/ SkScalar                  fSeed;
    SkSize fDims;
    
    
    typedef SkShader INHERITED;
};

#endif
