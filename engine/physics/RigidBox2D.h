#pragma once

#include <box2d/box2d.h>

enum class BodyType
{
    Static = 0,
    Dynamic,
    Kinematic
};

class Rigidbody2D
{
public:
    BodyType m_type = BodyType::Static;
    bool m_fixedRotation = false;

    b2BodyId m_runtimeBody = b2_nullBodyId;

    void setType(BodyType type)
    {
        m_type = type;

        if (b2Body_IsValid(m_runtimeBody))
        {
            b2BodyType box2dType = (type == BodyType::Static)    ? b2_staticBody
                                   : (type == BodyType::Dynamic) ? b2_dynamicBody
                                                                 : b2_kinematicBody;

            b2Body_SetType(m_runtimeBody, box2dType);
        }
    }

    std::string getBodyType()
    {
        return (m_type == BodyType::Static) ? "Static" : (m_type == BodyType::Dynamic) ? "Dynamic"
                                                                                       : "Kinematic";
    }
};