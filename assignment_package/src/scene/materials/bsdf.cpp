#include "bsdf.h"
#include <warpfunctions.h>

BSDF::BSDF(const Intersection& isect, float eta /*= 1*/)
//TODO: Properly set worldToTangent and tangentToWorld
    : worldToTangent(glm::transpose(Matrix3x3(isect.tangent,isect.bitangent,isect.normalGeometric))),
      tangentToWorld(Matrix3x3(isect.tangent,isect.bitangent,isect.normalGeometric)),
//      worldToTangent(Matrix3x3(isect.tangent,isect.bitangent,isect.normalGeometric)),
//            tangentToWorld(glm::transpose(worldToTangent)),
      normal(isect.normalGeometric),
      eta(eta),
      numBxDFs(0),
      bxdfs{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
{}


void BSDF::UpdateTangentSpaceMatrices(const Normal3f& n, const Vector3f& t, const Vector3f b)
{
    //TODO: Update worldToTangent and tangentToWorld based on the normal, tangent, and bitangent
//    worldToTangent=Matrix3x3(t,b,n);
//    tangentToWorld=glm::transpose(Matrix3x3(t,b,n));
    worldToTangent=glm::transpose(Matrix3x3(t,b,n));
    tangentToWorld=Matrix3x3(t,b,n);
}


//
Color3f BSDF::f(const Vector3f &woW, const Vector3f &wiW, BxDFType flags /*= BSDF_ALL*/) const
{
    //TODO
    //return Color3f(0.f);


    //compute all BSDF
    Color3f f(0.0f);
    Vector3f wo=worldToTangent*woW;
    Vector3f wi=worldToTangent*wiW;
    bool is_reflect= glm::dot(wiW, normal)*glm::dot(woW,normal)>0;
    f=Color3f(0.0f);
    for(int i=0;i<numBxDFs;i++)
        {
            if(bxdfs[i]->MatchesFlags(flags) &&
                 ((is_reflect && (bxdfs[i]->type & BSDF_REFLECTION))||
                  (!is_reflect && (bxdfs[i]->type &BSDF_TRANSMISSION))))
            {
                f += bxdfs[i]->f(wo,wi);
            }
        }

    return f;

}

// Use the input random number _xi_ to select
// one of our BxDFs that matches the _type_ flags.

// After selecting our random BxDF, rewrite the first uniform
// random number contained within _xi_ to another number within
// [0, 1) so that we don't bias the _wi_ sample generated from
// BxDF::Sample_f.

// Convert woW and wiW into tangent space and pass them to
// the chosen BxDF's Sample_f (along with pdf).
// Store the color returned by BxDF::Sample_f and convert
// the _wi_ obtained from this function back into world space.

// Iterate over all BxDFs that we DID NOT select above (so, all
// but the one sampled BxDF) and add their PDFs to the PDF we obtained
// from BxDF::Sample_f, then average them all together.

// Finally, iterate over all BxDFs and sum together the results of their
// f() for the chosen wo and wi, then return that sum.

Color3f BSDF::Sample_f(const Vector3f &woW, Vector3f *wiW, const Point2f &xi,
                       float *pdf, BxDFType type, BxDFType *sampledType) const
{
    //TODO
    //choose BxDF
    int matchtype=BxDFsMatchingFlags(type);
    if(matchtype==0)
    {
        *pdf=0.0f;
        if(sampledType)
            *sampledType=BxDFType(0);
        return Color3f(0.0f);
    }
    //generate random BxDF type using xi
    int bxdf_type = std::min((int)std::floor(xi[0]*matchtype),matchtype-1);
    BxDF* bxdf = nullptr;
    int count=bxdf_type;
    for(int i=0;i<numBxDFs;i++)
    {
        if(bxdfs[i]->MatchesFlags(type) && count--==0)
        {
            bxdf=bxdfs[i];
        }
    }
    //rewrite xi
    Point2f xi_new=Point2f(std::min(xi[0]*matchtype-bxdf_type,OneMinusEpsilon),xi[1]);

    //sample BxDF
    Vector3f wi;
    Vector3f wo=worldToTangent*woW;
    //wo=glm::normalize(wo);
    if(fabs(wo.z)<FLT_EPSILON)
        return Color3f(0.0f);
    *pdf=0.0f;
    if(sampledType)
        *sampledType=bxdf->type;

    //compute f of BxDF
    Color3f f=bxdf->Sample_f(wo,&wi,xi_new,pdf,sampledType);
    if(fabs(*pdf)<FLT_EPSILON)
    {
        if(sampledType)
            *sampledType=BxDFType(0);
        return Color3f(0.0f);
    }
    *wiW=tangentToWorld*wi;
    //wiW=glm::normalize(*wiW);
    //compute all pdf
    if(!(bxdf->type & BSDF_SPECULAR)&& matchtype>1)
    {
        for(int i=0;i<numBxDFs;++i)
        {
            if(bxdfs[i]!=bxdf && bxdfs[i]->MatchesFlags(type))
            {
                *pdf+=bxdfs[i]->Pdf(wo,wi);
            }
        }
    }
    if(matchtype>1)
        *pdf/=matchtype;

    //compute all BSDF
    if(!(bxdf->type &BSDF_SPECULAR) && matchtype>1)
    {
        bool is_reflect= glm::dot(*wiW, normal)*glm::dot(woW,normal)>0;
        f=Color3f(0.0f);
        for(int i=0;i<numBxDFs;i++)
        {
            if(bxdfs[i]->MatchesFlags(type) &&
                 ((is_reflect && (bxdfs[i]->type & BSDF_REFLECTION))||
                  (!is_reflect && (bxdfs[i]->type &BSDF_TRANSMISSION))))
            {
                f += bxdfs[i]->f(wo,wi);
            }
        }
    }
//    if(!(bxdf->type & BSDF_SPECULAR)&& matchtype>1)
//    {
//        *pdf+=this->Pdf(woW,*wiW,type);
//    }
//    if(!(bxdf->type &BSDF_SPECULAR) && matchtype>1)
//    {
//        f += this->f(woW,*wiW,type);
//    }

    return f;
    //return Color3f(0.f);
}


float BSDF::Pdf(const Vector3f &woW, const Vector3f &wiW, BxDFType flags) const
{
    //TODO
    if(numBxDFs ==0)
        return 0.0f;
    Vector3f wo=worldToTangent*woW;
    Vector3f wi=worldToTangent*wiW;
    if(wo.z==0.0f)
        return 0.0f;
    Float pdf=0.0f;
    int matchNum=0;
    for(int i=0;i<numBxDFs;i++)
    {
        if(bxdfs[i]->MatchesFlags(flags))
        {
            ++matchNum;
            pdf+=bxdfs[i]->Pdf(wo,wi);
        }

    }
    float final_pdf=0.0f;
    if(matchNum>0)
        final_pdf=pdf/matchNum;
    return final_pdf;
    //return 0.f;
}

Color3f BxDF::Sample_f(const Vector3f &wo, Vector3f *wi, const Point2f &xi,
                       Float *pdf, BxDFType *sampledType) const
{
    //TODO
    if(!sampledType)
    {
        *sampledType=BxDFType(0);
        return Color3f(0.0f);
    }

    *wi=WarpFunctions::squareToHemisphereUniform(xi);
    if(wo.z<0.0f)
        wi->z*=-1.0f;
    Color3f s_f=f(wo,*wi);
    *pdf=Pdf(wo,*wi);
    return s_f;
}

// The PDF for uniform hemisphere sampling
float BxDF::Pdf(const Vector3f &wo, const Vector3f &wi) const
{
    return SameHemisphere(wo, wi) ? Inv2Pi : 0;
}
