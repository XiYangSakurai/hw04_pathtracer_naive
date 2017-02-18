#include "diffusearealight.h"

Color3f DiffuseAreaLight::L(const Intersection &isect, const Vector3f &w) const
{
    //TODO
    if(twoSided==false)
    {
        if(glm::dot(w,isect.normalGeometric)<=0.0f)
            return Color3f(0.0f);
    }

    return emittedLight;
}
