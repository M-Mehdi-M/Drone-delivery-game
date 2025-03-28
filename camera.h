#pragma once

#include "utils/glm_utils.h"
#include "utils/math_utils.h"


namespace m1
{
    class Camera
    {
    public:
        Camera()
        {
            position = glm::vec3(0, 2, 5);
            forward = glm::vec3(0, 0, -1);
            up = glm::vec3(0, 1, 0);
            right = glm::vec3(1, 0, 0);
            distanceToTarget = 2;
        }

        Camera(const glm::vec3& position, const glm::vec3& center, const glm::vec3& up)
        {
            Set(position, center, up);
        }

        ~Camera()
        { }

        void Set(const glm::vec3& position, const glm::vec3& center, const glm::vec3& up)
        {
            this->position = position;
            forward = glm::normalize(center - position);
            right = glm::cross(forward, up);
            this->up = glm::cross(right, forward);
        }

        void MoveForward(float distance)
        {
            glm::vec3 dir = glm::normalize(glm::vec3(forward.x, 0, forward.z));
            position += dir * distance;
        }

        void TranslateForward(float distance)
        {
            position += forward * distance;
        }

        void TranslateUpward(float distance)
        {
            position += up * distance;
        }

        void TranslateRight(float distance)
        {
            glm::vec3 right_ground = glm::normalize(glm::vec3(right.x, 0, right.z));
            position += right_ground * distance;
        }

        void RotateFirstPerson_OX(float angle)
        {
            glm::vec4 new_forward = glm::rotate(glm::mat4(1.0f), angle, right) * glm::vec4(forward, 0);
            glm::vec4 new_up = glm::rotate(glm::mat4(1.0f), angle, right) * glm::vec4(up, 0);
            forward = glm::normalize(glm::vec3(new_forward));
            up = glm::normalize(glm::vec3(new_up));
        }

        void RotateFirstPerson_OY(float angle)
        {
            glm::vec4 new_forward = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0)) * glm::vec4(forward, 0);
            glm::vec4 new_right = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0)) * glm::vec4(right, 0);
            forward = glm::normalize(glm::vec3(new_forward));
            right = glm::normalize(glm::vec3(new_right));
            up = glm::normalize(glm::cross(right, forward));
        }

        void RotateFirstPerson_OZ(float angle)
        {
            glm::vec4 new_right = glm::rotate(glm::mat4(1.0f), angle, forward) * glm::vec4(right, 0);
            glm::vec4 new_up = glm::rotate(glm::mat4(1.0f), angle, forward) * glm::vec4(up, 0);
            right = glm::normalize(glm::vec3(new_right));
            up = glm::normalize(glm::vec3(new_up));
        }

        void RotateThirdPerson_OX(float angle)
        {
            glm::vec3 target = GetTargetPosition();
            position -= target;
            glm::vec4 new_position = glm::rotate(glm::mat4(1.0f), angle, right) * glm::vec4(position, 1);
            glm::vec4 new_forward = glm::rotate(glm::mat4(1.0f), angle, right) * glm::vec4(forward, 0);
            glm::vec4 new_up = glm::rotate(glm::mat4(1.f), angle, right) * glm::vec4(up, 0);
            position = glm::vec3(new_position);
            forward = glm::normalize(glm::vec3(new_forward));
            up = glm::normalize(glm::vec3(new_up));
            position += target;
        }

        void RotateThirdPerson_OY(float angle)
        {
            glm::vec3 target = GetTargetPosition();
            position -= target;
            glm::vec4 new_position = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0)) * glm::vec4(position, 1);
            glm::vec4 new_forward = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0)) * glm::vec4(forward, 0);
            glm::vec4 new_right = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0)) * glm::vec4(right, 0);
            position = glm::vec3(new_position);
            forward = glm::normalize(glm::vec3(new_forward));
            right = glm::normalize(glm::vec3(new_right));
            up = glm::normalize(glm::cross(right, forward));
            position += target;
        }

        void RotateThirdPerson_OZ(float angle)
        {
            glm::vec3 target = GetTargetPosition();
            position -= target;
            glm::vec4 new_position = glm::rotate(glm::mat4(1.f), angle, forward) * glm::vec4(position, 1);
            glm::vec4 new_up = glm::rotate(glm::mat4(1.f), angle, forward) * glm::vec4(up, 0);
            glm::vec4 new_right = glm::rotate(glm::mat4(1.f), angle, forward) * glm::vec4(right, 0);
            position = glm::vec3(new_position);
            up = glm::normalize(glm::vec3(new_up));
            right = glm::normalize(glm::vec3(new_right));
            position += target;
        }

        glm::mat4 GetViewMatrix()
        {
            return glm::lookAt(position, position + forward, up);
        }

        glm::vec3 GetTargetPosition()
        {
            return position + forward * distanceToTarget;
        }

    public:
        float distanceToTarget;
        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 right;
        glm::vec3 up;
    };
}
