/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkCustomShader.h"
#include "SkColorFilter.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkShader.h"
#include "SkUnPreMultiply.h"
#include "SkString.h"

#if SK_SUPPORT_GPU
#include "GrContext.h"
#include "GrCoordTransform.h"
#include "gl/GrGLEffect.h"
#include "GrTBackendEffectFactory.h"
#include "SkGr.h"
#endif


SkShader* SkCustomShader::Create(SkSize dims, SkScalar seed, const SkMatrix* localMatrix) {
    return SkNEW_ARGS(SkCustomShader, (dims, seed, localMatrix));
}

SkCustomShader::SkCustomShader(SkSize dims, SkScalar seed, const SkMatrix* localMatrix)
: fDims(dims), fSeed(seed), INHERITED(localMatrix)
{
}

SkCustomShader::SkCustomShader(SkReadBuffer& buffer)
: INHERITED(buffer)
{
//    fSeed           = buffer.readScalar();
}

SkCustomShader::~SkCustomShader() {
    // Safety, should have been done in endContext()
//    SkDELETE(fPaintingData);
}

void SkCustomShader::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
}

/////////////////////////////////////////////////////////////////////

#if SK_SUPPORT_GPU

#include "GrTBackendEffectFactory.h"

class GrGLCustom : public GrGLEffect {
public:
    GrGLCustom(const GrBackendEffectFactory& factory,
                    const GrDrawEffect& drawEffect);
    virtual ~GrGLCustom() {}
    
    virtual void emitCode(GrGLShaderBuilder*,
                          const GrDrawEffect&,
                          EffectKey,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;
    
    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) SK_OVERRIDE;
    
    static inline EffectKey GenKey(const GrDrawEffect&, const GrGLCaps&);
    
private:
    
    GrGLUniformManager::UniformHandle   fAlphaUni;
    GrGLUniformManager::UniformHandle   fTimeUni;
    
private:
    typedef GrGLEffect INHERITED;
};

/////////////////////////////////////////////////////////////////////

class GrCustomEffect : public GrEffect {
public:
    static GrEffectRef* Create(float time, const SkMatrix& matrix, uint8_t alpha) {
        AutoEffectUnref effect(SkNEW_ARGS(GrCustomEffect, (time, matrix, alpha)));
        return CreateEffectRef(effect);
    }
    
    virtual ~GrCustomEffect() { }
    
    static const char* Name() { return "PerlinNoise"; }
    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<GrCustomEffect>::getInstance();
    }
    
    uint8_t alpha() const { return fAlpha; }
    float time() const { return fTime; }
    const SkMatrix& matrix() const { return fCoordTransform.getMatrix(); }
    
    typedef GrGLCustom GLEffect;
    
private:
    virtual bool onIsEqual(const GrEffect& sBase) const SK_OVERRIDE {
        const GrCustomEffect& s = CastEffect<GrCustomEffect>(sBase);
        return fAlpha == s.fAlpha &&
               fCoordTransform.getMatrix() == s.fCoordTransform.getMatrix();
    }
    
    GrCustomEffect(float time, const SkMatrix& matrix, uint8_t alpha)
    : fTime(time),
      fAlpha(alpha) {
//        this->willReadFragmentPosition();
//        this->setWillNotUseInputColor();
        SkMatrix m = matrix;
//        m.postTranslate(SK_Scalar1, SK_Scalar1);
        fCoordTransform.reset(kLocal_GrCoordSet, m);
        this->addCoordTransform(&fCoordTransform);
    }
    
    GR_DECLARE_EFFECT_TEST;
    
    GrCoordTransform                fCoordTransform;
    uint8_t                         fAlpha;
    float                           fTime;
    
    void getConstantColorComponents(GrColor*, uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0; // This is noise. Nothing is constant.
    }
    
private:
    typedef GrEffect INHERITED;
};

/////////////////////////////////////////////////////////////////////
GR_DEFINE_EFFECT_TEST(GrCustomEffect);

GrEffectRef* GrCustomEffect::TestCreate(SkRandom* random,
                                             GrContext* context,
                                             const GrDrawTargetCaps&,
                                             GrTexture**) {
    SkScalar seed = SkIntToScalar(random->nextU());
    
    SkShader* shader = SkCustomShader::Create(SkSize::Make(100, 100), seed);
    
    SkPaint paint;
    GrColor grColor;
    GrEffectRef* effect;
    shader->asNewEffect(context, paint, NULL, &grColor, &effect);
    
    SkDELETE(shader);
    
    return effect;
}

GrGLCustom::GrGLCustom(const GrBackendEffectFactory& factory, const GrDrawEffect& drawEffect)
: INHERITED (factory)
{
}

void GrGLCustom::emitCode(GrGLShaderBuilder* builder,
                               const GrDrawEffect&,
                               EffectKey key,
                               const char* outputColor,
                               const char* inputColor,
                               const TransformedCoordsArray& coords,
                               const TextureSamplerArray& samplers) {
    sk_ignore_unused_variable(inputColor);
    
//    SkString vCoords = builder->ensureFSCoords2D(coords, 0);
    fAlphaUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kFloat_GrSLType, "alpha");
    const char* alphaUni = builder->getUniformCStr(fAlphaUni);
    fTimeUni = builder->addUniform(GrGLShaderBuilder::kFragment_Visibility,
                                    kFloat_GrSLType, "time");
    const char* timeUni = builder->getUniformCStr(fTimeUni);

    
    
    builder->fsCodeAppendf("\n\t\tconst float color_intensity = 0.9;");
    builder->fsCodeAppendf("\n\t\tconst float Pi = 3.14159;");
//
    builder->fsCodeAppendf("\n\t\tvec2 p=(3.32*%s);", coords[0].c_str());
    builder->fsCodeAppendf("\n\t\tfor(int i=1;i<5;i++) {");
    builder->fsCodeAppendf("\n\t\tvec2 newp=p;");
    builder->fsCodeAppendf("\n\t\tnewp.x+=0.12/float(i)*sin(float(i)*Pi*p.y+%s*0.45)+0.1;", timeUni);
    builder->fsCodeAppendf("\n\t\tnewp.y+=0.13/float(i)*cos(float(i)*Pi*p.x+%s*-0.4)-0.1;", timeUni);
    builder->fsCodeAppendf("\n\t\tp=newp;");
    builder->fsCodeAppendf("\n\t\t}");
    builder->fsCodeAppendf("\n\t\tvec3 col=vec3(sin(p.x+p.y)*.55+.5,sin(p.x+p.y+6.)*.5+.5,sin(p.x+p.y+12.)*.5+.5);");

//    builder->fsCodeAppendf("\n\t\t%s = vec4(%s.x, %s.y, 0.0, 1.0);", outputColor, coords[0].c_str(), coords[0].c_str());
    
    builder->fsCodeAppendf("\n\t\t%s = vec4(col.rgb * %s.aaa, %s.a);\n", outputColor, inputColor, inputColor);
    // Pre-multiply the result
//    builder->fsCodeAppendf("\n\t\t%s = vec4(%s.rgb * %s.aaa, %s.a);\n",
//                           outputColor, inputColor, inputColor, inputColor);
}

GrGLEffect::EffectKey GrGLCustom::GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&) {
    const GrCustomEffect& custom = drawEffect.castEffect<GrCustomEffect>();
    
    EffectKey key = custom.alpha();
        
    return key;
}

void GrGLCustom::setData(const GrGLUniformManager& uman, const GrDrawEffect& drawEffect) {
    INHERITED::setData(uman, drawEffect);
    
    const GrCustomEffect& custom = drawEffect.castEffect<GrCustomEffect>();
    
    uman.set1f(fAlphaUni, SkScalarDiv(SkIntToScalar(custom.alpha()), SkIntToScalar(255)));
    uman.set1f(fTimeUni, custom.time());
}

/////////////////////////////////////////////////////////////////////

bool SkCustomShader::asNewEffect(GrContext* context, const SkPaint& paint,
                                      const SkMatrix* localMatrix, GrColor* grColor,
                                      GrEffectRef** grEffect) const {
    SkASSERT(NULL != context);
    
    *grColor = SkColor2GrColor(paint.getColor());
    
    SkMatrix matrix;
    matrix.setIDiv(fDims.width(), fDims.height());
    
    SkMatrix lmInverse;
    if (!this->getLocalMatrix().invert(&lmInverse)) {
        return false;
    }
    if (localMatrix) {
        SkMatrix inv;
        if (!localMatrix->invert(&inv)) {
            return false;
        }
        lmInverse.postConcat(inv);
    }
    matrix.preConcat(lmInverse);
    
    *grEffect = GrCustomEffect::Create(fSeed, matrix, paint.getAlpha());
    (*const_cast<SkScalar*>(&fSeed)) = fSeed + 0.1;
    
    return true;
}

#else

bool SkCustomShader::asNewEffect(GrContext* context, const SkPaint& paint,
                                      const SkMatrix* externalLocalMatrix, GrColor* grColor,
                                      GrEffectRef** grEffect) const {
    SkDEBUGFAIL("Should not call in GPU-less build");
    return false;
}

#endif

#ifndef SK_IGNORE_TO_STRING
void SkCustomShader::toString(SkString* str) const {
    str->append("SkCustomShader: (");
    
    str->append("type: ");
    str->appendScalar(fSeed);
    
    this->INHERITED::toString(str);
    
    str->append(")");
}
#endif
