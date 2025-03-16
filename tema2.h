#pragma once
#include "components/simple_scene.h"
#include "lab_m1/tema2/camera.h"

namespace m1
{
    class Tema2 : public gfxc::SimpleScene
    {
    public:
        Tema2();
        ~Tema2();
        void Init() override;
    private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;
        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;
        void RenderDrone(float deltaTimeSeconds);
        void RenderPropeller(glm::mat4 modelMatrix, float x, float y, float z);
        void RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, const glm::vec3& color = glm::vec3(1));
        void UpdateCamera();
        void GenerateObstacles();
        void RenderTerrain();
        void CreateTerrain(int rows, int cols);
        bool CheckPreciseCollision(const glm::vec3& newPosition);
        void HandleDroneCrash(float deltaTimeSeconds);
        void ApplyCameraShake(glm::vec3& cameraPosition);
        void GenerateClouds();
        void RenderAlternateViewport(float deltaTimeSeconds);
        void GenerateNewPackagePosition();
        void GenerateNewDestinationPosition();
        void RenderPackage();
        void RenderDestination();
        void RenderDirectionalArrow();
        void CheckPackageCollection();
        bool IsPositionValidForGameObject(const glm::vec3& position);

        struct Obstacle {
            glm::mat4 modelMatrix;   // Model matrix for positioning and scaling the obstacle
            glm::vec3 color;         // Color of the obstacle
            std::string meshType;    // Type of mesh (e.g., "cone", "cylinder")
            bool isFrame = false;            // Flag to indicate whether the obstacle is a frame
        };


        struct CloudCluster {
            std::vector<glm::mat4> elementMatrices;
            glm::vec3 centerPosition;
            float scale;
        };

        struct Package {
            glm::vec3 position;
            bool isCollected;
            bool isFalling;           // New: track if package is falling
            float fallSpeed;          // New: falling speed
            glm::vec3 groundPosition; // New: where package will land
            bool hasLanded;           // New: track if package has landed
        };

        struct Destination {
            glm::vec3 position;
            bool isActive;
        };

    protected:
        m1::Camera* camera;
        glm::mat4 projectionMatrix;
        bool renderCameraTarget;

        // Drone properties
        glm::vec3 dronePosition;
        float droneRotationY;
        float droneRotationX;  // Add pitch rotation
        float propellerAngle;

        // Camera properties
        float fov;
        float orthoLeft, orthoRight;
        float orthoBottom, orthoTop;
        bool isPerspective;

        // Movement speeds
        const float DRONE_SPEED = 10.0f;
        const float ROTATION_SPEED = 4.0f;
        const float PROPELLER_SPEED = 10.0f;
        const float MOUSE_SENSITIVITY = 0.002f;  // Mouse sensitivity for rotation

        bool isCameraFirstPerson;
        glm::vec3 thirdPersonOffset;

        // Terrain properties
        Mesh* terrainMesh;

        // Obstacles
        std::vector<Obstacle> obstacles;
        std::vector<CloudCluster> cloudClusters;

        bool isDroneCrashed = false;
        float crashTimer = 0.0f;
        float MAX_CRASH_DURATION = 1.0f;
        float cameraShakeIntensity = 0.0f;

        Package currentPackage;
        Destination currentDestination;
        int collectedPackages;
        float arrowRotation;

        const float GRAVITY = 9.81f;
        const float PACKAGE_FALL_SPEED = 5.0f;
        const float PACKAGE_BOUNCE_FACTOR = 0.3f;
        const float MIN_FALL_SPEED = 0.1f;

    };
}