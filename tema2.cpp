#include "lab_m1/tema2/tema2.h"

#include <vector>
#include <string>
#include <iostream>

using namespace std;
using namespace m1;

Tema2::Tema2()
{
}

Tema2::~Tema2()
{
}

void Tema2::Init()
{
    renderCameraTarget = false;

    {
        Mesh* coneMesh = new Mesh("cone");
        coneMesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "cone.obj");
        meshes[coneMesh->GetMeshID()] = coneMesh;
    }

    {
        Mesh* cylinderMesh = new Mesh("cylinder");
        cylinderMesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "cylinder.obj");
        meshes[cylinderMesh->GetMeshID()] = cylinderMesh;
    }

    {
        Mesh* pyramidMesh = new Mesh("pyramid");
        pyramidMesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "pyramid.obj");
        meshes[pyramidMesh->GetMeshID()] = pyramidMesh;
    }

    {
        Mesh* pyramidMesh = new Mesh("sphere");
        pyramidMesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
        meshes[pyramidMesh->GetMeshID()] = pyramidMesh;
    }

    {
        Mesh* mesh = new Mesh("box");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    {
        Shader* shader = new Shader("ColorShader");
        shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "VertexShader.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "FragmentShader.glsl"), GL_FRAGMENT_SHADER);
        shader->CreateAndLink();
        shaders["VertexColor"] = shader;
    }

    {
        Shader* groundShader = new Shader("GroundShader");
        groundShader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "GroundVertexShader.glsl"), GL_VERTEX_SHADER);
        groundShader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "GroundFragmentShader.glsl"), GL_FRAGMENT_SHADER);
        groundShader->CreateAndLink();
        shaders["GroundShader"] = groundShader;
    }

    {
        std::vector<VertexFormat> vertices = {
            VertexFormat(glm::vec3(0, 0, 1)),    // tip
            VertexFormat(glm::vec3(-1, 0, -1)),  // left
            VertexFormat(glm::vec3(1, 0, -1))    // right
        };

        std::vector<unsigned int> indices = {
            0, 1, 2
        };

        meshes["triangle"] = new Mesh("triangle");
        meshes["triangle"]->InitFromData(vertices, indices);
    }

    isCameraFirstPerson = false;
    thirdPersonOffset = glm::vec3(0, 2, 3);

    camera = new m1::Camera();
    camera->Set(glm::vec3(0, 2, 3.5f), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));

    // Initialize drone properties
    dronePosition = glm::vec3(0, 2, 0);
    droneRotationY = RADIANS(225);
    droneRotationX = 0;  // Add this line
    propellerAngle = 0;

    // Camera properties
    fov = 60.0f;
    orthoLeft = -8.0f;
    orthoRight = 8.0f;
    orthoBottom = -4.5f;
    orthoTop = 4.5f;
    isPerspective = true;

    projectionMatrix = glm::perspective(RADIANS(60), window->props.aspectRatio, 0.01f, 200.0f);

    // Initialize terrain
    CreateTerrain(200, 200);

    // Initialize obstacles
    srand(static_cast<unsigned>(time(nullptr)));
    GenerateObstacles();
    GenerateClouds();

    collectedPackages = 0;
    currentPackage.isCollected = false;
    currentDestination.isActive = false;

    // Generate first package
    GenerateNewPackagePosition();
}

void Tema2::GenerateNewPackagePosition() {
    float minBound = 10.0f;
    float maxBound = 190.0f;

    bool validPosition = false;
    glm::vec3 newPosition;

    // Keep trying until we find a valid position
    int maxAttempts = 100;  // Prevent infinite loop
    int attempts = 0;

    while (!validPosition && attempts < maxAttempts) {
        newPosition = glm::vec3(
            minBound + static_cast<float>(rand()) / RAND_MAX * (maxBound - minBound),
            1.0f,
            minBound + static_cast<float>(rand()) / RAND_MAX * (maxBound - minBound)
        );

        if (IsPositionValidForGameObject(newPosition)) {
            validPosition = true;
        }

        attempts++;
    }

    // If we couldn't find a valid position after max attempts, use the last generated position
    currentPackage.position = newPosition;
    currentPackage.isCollected = false;
    currentPackage.isFalling = false;
    currentPackage.fallSpeed = 0.0f;
    currentPackage.hasLanded = true;
    currentPackage.groundPosition = currentPackage.position;
}

void Tema2::CheckPackageCollection() {
    if (!currentPackage.isCollected) {
        float distance = glm::distance(dronePosition, currentPackage.position);
        if (distance < 2.0f) { // Adjust collection radius as needed
            currentPackage.isCollected = true;
            collectedPackages++;
            // Generate new package position if needed
            GenerateNewPackagePosition();
        }
    }
}


void Tema2::GenerateNewDestinationPosition() {
    float minBound = 10.0f;
    float maxBound = 190.0f;

    glm::vec2 packagePos = glm::vec2(currentPackage.position.x, currentPackage.position.z);

    float maxDistance = 0.0f;
    glm::vec3 bestPosition;
    bool foundValidPosition = false;

    const int NUM_ATTEMPTS = 50;  // Increased attempts to account for obstacle checking

    for (int i = 0; i < NUM_ATTEMPTS; i++) {
        glm::vec3 candidatePos = glm::vec3(
            minBound + static_cast<float>(rand()) / RAND_MAX * (maxBound - minBound),
            0.5f,
            minBound + static_cast<float>(rand()) / RAND_MAX * (maxBound - minBound)
        );

        // Check if position is valid (away from obstacles)
        if (!IsPositionValidForGameObject(candidatePos)) {
            continue;  // Skip this position if too close to obstacles
        }

        // Calculate distance from package
        float distance = glm::distance(
            glm::vec2(candidatePos.x, candidatePos.z),
            packagePos
        );

        // Update if this is the farthest valid position found
        if (distance > maxDistance) {
            maxDistance = distance;
            bestPosition = candidatePos;
            foundValidPosition = true;
        }
    }

    // If no valid position was found, try again with relaxed constraints
    if (!foundValidPosition) {
        // Use the original package generation logic as fallback
        GenerateNewPackagePosition();  // This will at least ensure basic validity
        bestPosition = currentPackage.position;
    }

    currentDestination.position = bestPosition;
    currentDestination.isActive = true;
}

bool Tema2::IsPositionValidForGameObject(const glm::vec3& position) {
    // Check distance from all obstacles
    for (const auto& obs : obstacles) {
        // Extract obstacle position from its model matrix
        glm::vec3 obsPosition(
            glm::value_ptr(obs.modelMatrix)[12],
            glm::value_ptr(obs.modelMatrix)[13],
            glm::value_ptr(obs.modelMatrix)[14]
        );

        float distance = glm::distance(
            glm::vec2(position.x, position.z),
            glm::vec2(obsPosition.x, obsPosition.z)
        );

        if (distance < 10.0f) {
            return false;  // Too close to an obstacle
        }
    }
    return true;
}

void Tema2::GenerateClouds() {
    // Generate 10-15 cloud clusters
    int clusterCount = 20;

    for (int i = 0; i < clusterCount; i++) {
        CloudCluster cloudCluster;

        // Randomize cluster center position within map (avoiding 10-unit border)
        float centerX = 10.0f + static_cast<float>(rand()) / RAND_MAX * (180.0f);
        float centerZ = 10.0f + static_cast<float>(rand()) / RAND_MAX * (180.0f);
        float centerY = 40.0f + static_cast<float>(rand()) / RAND_MAX * 15.0f;  // Height between 50-65

        cloudCluster.centerPosition = glm::vec3(centerX, centerY, centerZ);
        cloudCluster.scale = 15.0f + static_cast<float>(rand()) / RAND_MAX * 5.0f;  // Scale range 10-15

        // Generate 10-15 spheres per cluster
        int sphereCount = 20 + rand() % 10;

        for (int j = 0; j < sphereCount; j++) {
            // Randomize offset from cluster center (tight distribution to form a cohesive cloud)
            float offsetX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudCluster.scale * 0.8f;
            float offsetY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudCluster.scale * 0.5f;
            float offsetZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudCluster.scale * 0.8f;

            // Individual sphere scale variation (relative to cluster scale)
            float sphereScale = cloudCluster.scale * (0.2f + static_cast<float>(rand()) / RAND_MAX * 0.4f);

            glm::mat4 sphereMatrix = glm::translate(glm::mat4(1.0f),
                cloudCluster.centerPosition + glm::vec3(offsetX, offsetY, offsetZ)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(sphereScale, sphereScale, sphereScale));

            cloudCluster.elementMatrices.push_back(sphereMatrix);
        }

        cloudClusters.push_back(cloudCluster);
    }
}

void Tema2::RenderAlternateViewport(float deltaTimeSeconds) {
    // Save the current viewport and projection matrix
    glm::mat4 originalProjection = projectionMatrix;
    glViewport(0, 0, window->GetResolution().x, window->GetResolution().y);

    // Set up minimap viewport in the top-right corner
    int minimapWidth = (window->GetResolution().x / 4);   // 320 pixels for 1280
    int minimapHeight = (window->GetResolution().y / 4);  // 180 pixels for 720

    // Position exactly at top-right corner (no gap)
    glViewport(window->GetResolution().x - minimapWidth,  // 1280 - 320 = 960
        window->GetResolution().y - minimapHeight,  // 720 - 180 = 540
        minimapWidth,    // 320
        minimapHeight);  // 180

    // Rest of your code remains the same...
    float mapSize = 20.0f;
    projectionMatrix = glm::ortho(-mapSize, mapSize, -mapSize, mapSize, 0.1f, 500.0f);

    // Create a camera position and look-at for top-down view
    glm::vec3 originalCameraPosition = camera->position;
    glm::vec3 originalCameraForward = camera->forward;
    glm::vec3 originalCameraUp = camera->up;

    // Position camera above the map looking down
    camera->position = glm::vec3(dronePosition.x, 100, dronePosition.z); // Camera follows drone position
    camera->forward = glm::vec3(0, -1, 0);
    camera->up = glm::vec3(0, 0, -1);
    camera->right = glm::cross(camera->forward, camera->up);

    // Render scene elements for minimap
    RenderTerrain();
    RenderDrone(deltaTimeSeconds);

    // Render obstacles
    for (const auto& obstacle : obstacles) {
        RenderSimpleMesh(meshes[obstacle.meshType], shaders["VertexColor"],
            obstacle.modelMatrix, obstacle.color);
    }

    RenderDirectionalArrow();
    // Render package and destination if they exist
    if (!currentPackage.isCollected) {
        RenderPackage();
    }
    if (currentDestination.isActive) {
        RenderDestination();
    }

    // Restore original camera properties
    camera->position = originalCameraPosition;
    camera->forward = originalCameraForward;
    camera->up = originalCameraUp;
    camera->right = glm::cross(camera->forward, camera->up);

    // Restore original projection and viewport
    projectionMatrix = originalProjection;
    glViewport(0, 0, window->GetResolution().x, window->GetResolution().y);
}


void Tema2::Update(float deltaTimeSeconds)
{
    // Handle drone crash mechanism
    HandleDroneCrash(deltaTimeSeconds);

    RenderTerrain();

    // Render obstacles
    for (const Obstacle& obs : obstacles) {
        if (obs.meshType == "cone") {
            RenderSimpleMesh(meshes["cone"], shaders["VertexColor"], obs.modelMatrix, obs.color);
        }
        else if (obs.meshType == "cylinder") {
            RenderSimpleMesh(meshes["cylinder"], shaders["VertexColor"], obs.modelMatrix, obs.color);
        }
        else if (obs.meshType == "box") {
            RenderSimpleMesh(meshes["box"], shaders["VertexColor"], obs.modelMatrix, obs.color);
        }
        else if (obs.meshType == "pyramid") {
            RenderSimpleMesh(meshes["pyramid"], shaders["VertexColor"], obs.modelMatrix, obs.color);
        }
    }

    // Render drone as before
    RenderDrone(deltaTimeSeconds);

    // Update camera position relative to drone
    UpdateCamera();

    for (const CloudCluster& cluster : cloudClusters) {
        for (const glm::mat4& sphereMatrix : cluster.elementMatrices) {
            // White with slight variation
            glm::vec3 cloudColor(0.9f + (rand() % 100) / 1000.0f,
                0.9f + (rand() % 100) / 1000.0f,
                0.9f + (rand() % 100) / 1000.0f);

            RenderSimpleMesh(meshes["sphere"], shaders["VertexColor"], sphereMatrix, cloudColor);
        }
    }
    RenderAlternateViewport(deltaTimeSeconds);

    if (!currentPackage.isCollected) {
        if (glm::distance(dronePosition, currentPackage.position) < 2.0f) {
            currentPackage.isCollected = true;
            GenerateNewDestinationPosition();
        }
    }

    // Update package position if collected
    if (currentPackage.isCollected) {
        // Place package below drone
        currentPackage.position = dronePosition - glm::vec3(0, 1.5f, 0);

        // Check for delivery
        if (currentDestination.isActive) {
            float distToDestination = glm::distance(
                glm::vec2(dronePosition.x, dronePosition.z),
                glm::vec2(currentDestination.position.x, currentDestination.position.z)
            );

            if (distToDestination < 3.0f) {
                collectedPackages++;
                currentDestination.isActive = false;
                currentPackage.isCollected = false;
                GenerateNewPackagePosition();
            }
        }
    }

    

    // Render game elements
    RenderDirectionalArrow();
    RenderPackage();
    if (currentDestination.isActive) {
        RenderDestination();
    }
}

void Tema2::RenderPackage() {
    if (currentPackage.isCollected) {
        // Package follows and rotates with drone
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, dronePosition);
        modelMatrix = glm::rotate(modelMatrix, droneRotationY, glm::vec3(0, 1, 0));
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -1.0f, 0)); // Offset below drone
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], modelMatrix, glm::vec3(1, 0, 0));
    }
    else {
        // Package on ground or falling
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, currentPackage.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], modelMatrix, glm::vec3(1, 0, 0));

        glm::vec3 signPosition = currentPackage.position + glm::vec3(0, 8.0f, 0);  // 5 units above destination

        // Calculate direction from sign to drone
        glm::vec3 directionToDrone = glm::normalize(dronePosition - signPosition);

        // Calculate rotation to face the drone
        float angleY = atan2(directionToDrone.x, directionToDrone.z);

        // Create model matrix for the floating sign
        glm::mat4 signMatrix = glm::mat4(1.0f);
        signMatrix = glm::translate(signMatrix, signPosition);
        signMatrix = glm::rotate(signMatrix, angleY, glm::vec3(0, 1, 0));  // Rotate to face drone
        signMatrix = glm::rotate(signMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));  // Point downward

        // Scale the arrow appropriately
        signMatrix = glm::scale(signMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

        // Render the floating arrow sign
        RenderSimpleMesh(meshes["triangle"], shaders["VertexColor"], signMatrix, glm::vec3(1, 0, 0));  // Yellow arrow
    }
}


void Tema2::RenderDestination() {
    if (currentDestination.isActive) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, currentDestination.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 0.3f, 2.0f));
        RenderSimpleMesh(meshes["cylinder"], shaders["VertexColor"], modelMatrix, glm::vec3(1, 1, 0));
    }

    glm::vec3 signPosition = currentDestination.position + glm::vec3(0, 8.0f, 0);  // 5 units above destination

    // Calculate direction from sign to drone
    glm::vec3 directionToDrone = glm::normalize(dronePosition - signPosition);

    // Calculate rotation to face the drone
    float angleY = atan2(directionToDrone.x, directionToDrone.z);

    // Create model matrix for the floating sign
    glm::mat4 signMatrix = glm::mat4(1.0f);
    signMatrix = glm::translate(signMatrix, signPosition);
    signMatrix = glm::rotate(signMatrix, angleY, glm::vec3(0, 1, 0));  // Rotate to face drone
    signMatrix = glm::rotate(signMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0));  // Point downward

    // Scale the arrow appropriately
    signMatrix = glm::scale(signMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

    // Render the floating arrow sign
    RenderSimpleMesh(meshes["triangle"], shaders["VertexColor"], signMatrix, glm::vec3(1, 1, 0));  // Yellow arrow
}

void Tema2::RenderDirectionalArrow() {
    // Calculate direction vector based on whether package is collected
    glm::vec2 dronePos2D(dronePosition.x, dronePosition.z);
    glm::vec2 targetPos2D;
    glm::vec3 color;

    if (!currentPackage.isCollected) {
        // Point to package if not collected
        targetPos2D = glm::vec2(currentPackage.position.x, currentPackage.position.z);
        color = glm::vec3(1, 0, 0);
    }
    else {
        // Point to destination if package is collected
        targetPos2D = glm::vec2(currentDestination.position.x, currentDestination.position.z);
        color = glm::vec3(1, 1, 0);
    }

    glm::vec2 direction = glm::normalize(targetPos2D - dronePos2D);

    // Calculate angle between direction vector and forward vector (0,1)
    float angle = atan2(direction.x, direction.y);

    // Base transformation for both parts
    glm::mat4 baseMatrix = glm::mat4(1.0f);
    baseMatrix = glm::translate(baseMatrix, dronePosition + glm::vec3(0, 0.75f, 0));
    baseMatrix = glm::rotate(baseMatrix, angle, glm::vec3(0, 1, 0));

    // Render shaft (rectangle)
    glm::mat4 shaftMatrix = baseMatrix;
    shaftMatrix = glm::translate(shaftMatrix, glm::vec3(0, 0, -0.5f));
    shaftMatrix = glm::scale(shaftMatrix, glm::vec3(0.1f, 0.01f, 1.0f));
    RenderSimpleMesh(meshes["box"], shaders["VertexColor"], shaftMatrix, color);

    // Render arrow head (triangle)
    glm::mat4 headMatrix = baseMatrix;
    headMatrix = glm::translate(headMatrix, glm::vec3(0, 0, 0.25f));
    headMatrix = glm::scale(headMatrix, glm::vec3(0.3f, 0.1f, 0.3f));
    RenderSimpleMesh(meshes["triangle"], shaders["VertexColor"], headMatrix, color);
}


void Tema2::FrameStart()
{
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Sky blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::ivec2 resolution = window->GetResolution();
    glViewport(0, 0, resolution.x, resolution.y); // Main viewport for the full screen
}

void Tema2::RenderPropeller(glm::mat4 modelMatrix, float x, float y, float z)
{
    // Transform from local propeller position to drone body space
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));
    modelMatrix = glm::rotate(modelMatrix, propellerAngle, glm::vec3(0, 1, 0));

    // Propeller hub
    {
        glm::mat4 propHub = modelMatrix;
        propHub = glm::scale(propHub, glm::vec3(0.2f));
        RenderMesh(meshes["box"], shaders["VertexNormal"], propHub);
    }

    // Propeller blades
    {
        glm::mat4 blade1 = modelMatrix;
        blade1 = glm::scale(blade1, glm::vec3(0.8f, 0.05f, 0.15f));
        RenderMesh(meshes["box"], shaders["VertexColor"], blade1);

        glm::mat4 blade2 = modelMatrix;
        blade2 = glm::rotate(blade2, RADIANS(90.0f), glm::vec3(0, 1, 0));
        blade2 = glm::scale(blade2, glm::vec3(0.8f, 0.05f, 0.15f));
        RenderMesh(meshes["box"], shaders["VertexColor"], blade2);
    }
}

void Tema2::UpdateCamera()
{
    glm::vec3 finalCameraPosition;

    if (isCameraFirstPerson) {
        // First-person camera: positioned at the drone's current position
        glm::vec3 cameraOffset(0, 0.5, 0);  // Slight vertical offset for a more natural view

        // Create rotation matrix based on drone's rotations
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), droneRotationY, glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), droneRotationX, glm::vec3(1, 0, 0));

        // Calculate forward direction based on drone's rotation
        glm::vec3 droneForward = glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0));

        // Set camera position to drone position with a slight offset
        finalCameraPosition = dronePosition + cameraOffset;

        // Apply camera shake if drone is crashed
        ApplyCameraShake(finalCameraPosition);

        camera->Set(
            finalCameraPosition,                        // Camera position
            dronePosition + droneForward + cameraOffset,// Look at point (drone's forward direction)
            glm::vec3(0, 1, 0)                          // Up vector
        );
    }
    else {
        // Third-person camera logic
        glm::vec3 cameraOffset = thirdPersonOffset;

        // Rotate the offset based on drone's rotation
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), droneRotationY, glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), droneRotationX, glm::vec3(1, 0, 0));

        glm::vec3 rotatedOffset = glm::vec3(rotationMatrix * glm::vec4(cameraOffset, 1.0f));

        // Set camera position
        finalCameraPosition = dronePosition + rotatedOffset;

        // Apply camera shake if drone is crashed
        ApplyCameraShake(finalCameraPosition);

        // Camera looks at the drone's position
        camera->Set(
            finalCameraPosition,   // Camera position
            dronePosition,          // Look at drone
            glm::vec3(0, 1, 0)      // Up vector
        );
    }
}

void Tema2::RenderDrone(float deltaTimeSeconds)
{
    // Colors
    glm::vec3 droneBodyColor(0.7f, 0.7f, 0.7f);  // Light gray for drone body
    glm::vec3 propellerColor(0.0f, 0.0f, 0.0f);  // Black for propellers

    // Base transformation for the entire drone
    glm::mat4 modelMatrix = glm::mat4(1);
    modelMatrix = glm::translate(modelMatrix, dronePosition);
    modelMatrix = glm::rotate(modelMatrix, droneRotationY, glm::vec3(0, 1, 0));

    // Draw drone body arms (X shape)
    float armLength = 1.0f;
    float armAngle = RADIANS(45.0f);

    // First arm (front-right to back-left)
    {
        glm::mat4 arm1 = modelMatrix;
        arm1 = glm::rotate(arm1, armAngle, glm::vec3(0, 1, 0));
        arm1 = glm::scale(arm1, glm::vec3(2.0f, 0.2f, 0.2f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], arm1, droneBodyColor);
    }

    // Second arm (front-left to back-right)
    {
        glm::mat4 arm2 = modelMatrix;
        arm2 = glm::rotate(arm2, -armAngle, glm::vec3(0, 1, 0));
        arm2 = glm::scale(arm2, glm::vec3(2.0f, 0.2f, 0.2f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], arm2, droneBodyColor);
    }

    // Calculate propeller positions in local space
    glm::vec3 propellerPositions[4] = {
        glm::vec3(armLength * cos(armAngle), 0, armLength * sin(armAngle)),   // Front-right
        glm::vec3(-armLength * cos(armAngle), 0, armLength * sin(armAngle)),  // Front-left
        glm::vec3(-armLength * cos(armAngle), 0, -armLength * sin(armAngle)), // Back-left
        glm::vec3(armLength * cos(armAngle), 0, -armLength * sin(armAngle))   // Back-right
    };

    // Render all propellers
    for (const auto& pos : propellerPositions) {
        // Move the propellers 1 unit upwards (along Y-axis)
        glm::mat4 propellerMatrix = modelMatrix;
        propellerMatrix = glm::translate(propellerMatrix, glm::vec3(pos.x, pos.y + 0.07f, pos.z));

        // Propeller hub
        glm::mat4 propHub = propellerMatrix;
        propHub = glm::scale(propHub, glm::vec3(0.2f, 0.35f, 0.2f));
        propHub = glm::rotate(propHub, RADIANS(45.0f), glm::vec3(0, 1, 0)); // Rotate 45 degrees around Y-axis
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], propHub, droneBodyColor);

        // Propeller blades
        glm::mat4 bladeBase = propellerMatrix;
        bladeBase = glm::translate(bladeBase, glm::vec3(0, 0.2f, 0));
        bladeBase = glm::rotate(bladeBase, propellerAngle, glm::vec3(0, 1, 0));

        // First blade
        glm::mat4 blade1 = bladeBase;
        blade1 = glm::scale(blade1, glm::vec3(0.8f, 0.05f, 0.1f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], blade1, propellerColor);

        // Second blade (perpendicular)
        glm::mat4 blade2 = bladeBase;
        blade2 = glm::rotate(blade2, RADIANS(90.0f), glm::vec3(0, 1, 0));
        blade2 = glm::scale(blade2, glm::vec3(0.8f, 0.05f, 0.1f));
        RenderSimpleMesh(meshes["box"], shaders["VertexColor"], blade2, propellerColor);
    }

    // Update propeller rotation
    propellerAngle += deltaTimeSeconds * PROPELLER_SPEED;
    if (propellerAngle >= 360.0f) {
        propellerAngle -= 360.0f;
    }
}

void Tema2::FrameEnd()
{
    DrawCoordinateSystem(camera->GetViewMatrix(), projectionMatrix);
}

bool Tema2::CheckPreciseCollision(const glm::vec3& newPosition) {
    if (newPosition.y < 0.1f) {
        return true;
    }
    for (const Obstacle& obs : obstacles) {
        // Extract obstacle's translation and scale from the model matrix
        glm::vec3 obstaclePos = glm::vec3(obs.modelMatrix[3]);
        glm::vec3 obstacleScale = glm::vec3(
            glm::length(glm::vec3(obs.modelMatrix[0])),
            glm::length(glm::vec3(obs.modelMatrix[1])),
            glm::length(glm::vec3(obs.modelMatrix[2]))
        );

        // Drone's size (you might want to adjust these based on your drone model)
        float droneRadius = 0.8f;

        // Check collision based on mesh type
        if (obs.meshType == "cone") {
            // Cone collision check
            float coneHeight = obstacleScale.y;
            float coneBaseRadius = std::max(obstacleScale.x, obstacleScale.z);

            // Distance from the drone's position to the cone's base in the XZ plane
            float distanceXZ = glm::distance(glm::vec2(newPosition.x, newPosition.z), glm::vec2(obstaclePos.x, obstaclePos.z));

            // Interpolate max radius based on drone's Y position within the cone's height
            float interpolatedRadius = coneBaseRadius * (1.0f - (newPosition.y - obstaclePos.y) / coneHeight);

            // Check if the drone is within the cone's base radius and between its vertical bounds
            if (distanceXZ < interpolatedRadius + droneRadius &&
                newPosition.y >= obstaclePos.y - 3 &&
                newPosition.y <= obstaclePos.y + coneHeight) {
                isDroneCrashed = true;
                crashTimer = 0.0f;
                cameraShakeIntensity = 0.5f;
                return true;
            }
        }

        else if (obs.meshType == "cylinder") {
            // Cylinder collision check
            float cylinderHeight = obstacleScale.y;
            float cylinderRadius = std::max(obstacleScale.x, obstacleScale.z);

            // Distance from drone's position to the cylinder's center in the XZ plane
            float distanceXZ = glm::distance(glm::vec2(newPosition.x, newPosition.z), glm::vec2(obstaclePos.x, obstaclePos.z));

            // Check if the drone is within the cylinder's radius and within its vertical bounds
            if (distanceXZ < cylinderRadius + droneRadius &&
                newPosition.y >= obstaclePos.y - 4.5 &&
                newPosition.y <= obstaclePos.y + cylinderHeight) {
                isDroneCrashed = true;
                crashTimer = 0.0f;
                cameraShakeIntensity = 0.5f;
                return true;
            }
        }

        else if (obs.meshType == "box") {
            // Axis-Aligned Bounding Box (AABB) collision check
            glm::vec3 minPoint = obstaclePos - obstacleScale / 2.0f;
            glm::vec3 maxPoint = obstaclePos + obstacleScale / 2.0f;

            bool collisionX = (newPosition.x >= minPoint.x - droneRadius &&
                newPosition.x <= maxPoint.x + droneRadius);
            bool collisionY = (newPosition.y >= minPoint.y - droneRadius &&
                newPosition.y <= maxPoint.y + droneRadius);
            bool collisionZ = (newPosition.z >= minPoint.z - droneRadius &&
                newPosition.z <= maxPoint.z + droneRadius);

            if (collisionX && collisionY && collisionZ) {
                isDroneCrashed = true;
                crashTimer = 0.0f;
                cameraShakeIntensity = 0.5f;
                return true;
            }
        }
        else if (obs.meshType == "pyramid") {
            // Pyramid collision (simplified)
            float pyramidHeight = obstacleScale.y;
            float pyramidBaseX = obstacleScale.x;
            float pyramidBaseZ = obstacleScale.z;

            // Check if within pyramid's base projection
            float distanceXZ = glm::distance(glm::vec2(newPosition.x, newPosition.z),
                glm::vec2(obstaclePos.x, obstaclePos.z));

            // Linear interpolation of max radius based on height
            float maxBaseRadius = std::max(pyramidBaseX, pyramidBaseZ) / 2.0f;
            float interpolatedRadius = maxBaseRadius * (1.0f - (newPosition.y - obstaclePos.y) / pyramidHeight);

            if (distanceXZ < interpolatedRadius + droneRadius &&
                newPosition.y >= obstaclePos.y - 2 &&
                newPosition.y <= obstaclePos.y + pyramidHeight) {
                isDroneCrashed = true;
                crashTimer = 0.0f;
                cameraShakeIntensity = 0.5f;
                return true;
            }
        }
    }
    return false;
}



void Tema2::OnInputUpdate(float deltaTime, int mods)
{
    glm::vec3 newPosition = dronePosition;

    // Create rotation matrix based on current drone orientation
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), droneRotationY, glm::vec3(0, 1, 0)) *
        glm::rotate(glm::mat4(1.0f), droneRotationX, glm::vec3(1, 0, 0));

    glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0));
    glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0));
    glm::vec3 up = glm::vec3(0, 1, 0);  // Always use world up vector for vertical movement

    if (window->KeyHold(GLFW_KEY_W)) {
        newPosition += forward * DRONE_SPEED * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_S)) {
        newPosition -= forward * DRONE_SPEED * deltaTime;
    }

    // Left/Right movement (relative to current view direction)
    if (window->KeyHold(GLFW_KEY_A)) {
        newPosition -= right * DRONE_SPEED * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_D)) {
        newPosition += right * DRONE_SPEED * deltaTime;
    }

    // Vertical movement (always world up/down)
    if (window->KeyHold(GLFW_KEY_UP)) {
        newPosition += up * DRONE_SPEED * deltaTime;
    }

    if (window->KeyHold(GLFW_KEY_DOWN)) {
        // Prevent drone from going below terrain (y = 0)
        if (currentPackage.isCollected && newPosition.y > 1.7f) {
            newPosition -= up * DRONE_SPEED * deltaTime;
        }
        if (!currentPackage.isCollected && newPosition.y > 0.2f) {
            newPosition -= up * DRONE_SPEED * deltaTime;
        }   
    }

    if (window->KeyHold(GLFW_KEY_LEFT)) {  // Rotate left
        droneRotationY += deltaTime * ROTATION_SPEED;
    }

    if (window->KeyHold(GLFW_KEY_RIGHT)) {  // Rotate right
        droneRotationY -= deltaTime * ROTATION_SPEED;
    }

    // Check for precise collisions before updating position
    // Also ensure drone stays above the terrain
    if (newPosition.y >= 0.1f && !CheckPreciseCollision(newPosition)) {
        dronePosition = newPosition;
    }
}


void Tema2::CreateTerrain(int rows, int cols) {
    vector<VertexFormat> vertices;
    vector<unsigned int> indices;

    // Define the grid spacing
    float gridSpacing = 1.0f; // Adjust as necessary for terrain scale

    for (int i = 0; i <= rows; ++i) {
        for (int j = 0; j <= cols; ++j) {
            // Initialize vertex positions; height (y) will be modified in the shader
            vertices.emplace_back(glm::vec3(j * gridSpacing, 0, i * gridSpacing), // Position
                glm::vec3(0, 1, 0),                          // Normal
                glm::vec3(0, 1, 0));                         // Color (can be adjusted in shaders)
        }
    }

    // Define the grid indices for triangles
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int start = i * (cols + 1) + j;
            indices.push_back(start);
            indices.push_back(start + 1);
            indices.push_back(start + cols + 1);

            indices.push_back(start + 1);
            indices.push_back(start + cols + 2);
            indices.push_back(start + cols + 1);
        }
    }

    // Create and store the terrain mesh
    Mesh* terrainMesh = new Mesh("terrain");
    terrainMesh->InitFromData(vertices, indices);
    meshes["terrain"] = terrainMesh;
}


void Tema2::GenerateObstacles() {
    int treeCount1 = 20;           // Number of trees in the first group
    int treeCount2 = 20;           // Number of trees in the second group
    int buildingCount = 20;        // Number of buildings
    float terrainSize = 200.0f;    // Size of the terrain in world coordinates
    float margin = 10.0f;           // Margin around the edges where obstacles are not allowed
    int frameCount = 20;

    obstacles.clear();            // Clear any previously stored obstacles

    // Lambda function to generate random positions within the terrain, respecting the margin
    auto generateRandomPosition = [&](float& x, float& z) {
        x = margin + static_cast<float>(rand()) / RAND_MAX * (terrainSize - 2 * margin);
        z = margin + static_cast<float>(rand()) / RAND_MAX * (terrainSize - 2 * margin);
        };

    for (int i = 0; i < treeCount1; ++i) {
        bool positionValid = false;
        float x, z;

        while (!positionValid) {
            generateRandomPosition(x, z);

            // Ensure the tree is not too close to the drone's initial position
            if (glm::distance(glm::vec2(x, z), glm::vec2(dronePosition.x, dronePosition.z)) < margin) {
                continue; // Regenerate position if too close to drone
            }

            positionValid = true;

            // Check distance from all existing obstacles
            for (const auto& obs : obstacles) {
                float distX = x - glm::value_ptr(obs.modelMatrix)[12];
                float distZ = z - glm::value_ptr(obs.modelMatrix)[14];

                if (sqrt(distX * distX + distZ * distZ) < 10) { // Minimum distance between obstacles
                    positionValid = false;
                    break;
                }
            }
        }

        // Create tree trunk
        glm::mat4 trunkModelMatrix = glm::mat4(1.0f);
        trunkModelMatrix = glm::translate(trunkModelMatrix, glm::vec3(x, 1, z));
        trunkModelMatrix = glm::scale(trunkModelMatrix, glm::vec3(0.8f, 1.0f, 0.8f));
        obstacles.push_back({ trunkModelMatrix, glm::vec3(0.5f, 0.3f, 0.0f), "cylinder" });

        // Create tree foliage (cones)
        glm::mat4 foliageModelMatrix1 = glm::mat4(1.0f);
        foliageModelMatrix1 = glm::translate(foliageModelMatrix1, glm::vec3(x, 4.5f, z));
        foliageModelMatrix1 = glm::scale(foliageModelMatrix1, glm::vec3(2.5f, 3.0f, 2.5f));
        obstacles.push_back({ foliageModelMatrix1, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });

        glm::mat4 foliageModelMatrix2 = glm::mat4(1.0f);
        foliageModelMatrix2 = glm::translate(foliageModelMatrix2, glm::vec3(x, 6.5f, z));
        foliageModelMatrix2 = glm::scale(foliageModelMatrix2, glm::vec3(2.25f, 2.5f, 2.25f));
        obstacles.push_back({ foliageModelMatrix2, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });

        glm::mat4 foliageModelMatrix3 = glm::mat4(1.0f);
        foliageModelMatrix3 = glm::translate(foliageModelMatrix3, glm::vec3(x, 8.5f, z));
        foliageModelMatrix3 = glm::scale(foliageModelMatrix3, glm::vec3(2.0f, 2.0f, 2.0f));
        obstacles.push_back({ foliageModelMatrix3, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });
    }

    for (int i = 0; i < frameCount; ++i) {
        bool positionValid = false;
        float x, z;

        while (!positionValid) {
            generateRandomPosition(x, z);

            // Ensure the frame is not too close to the drone's initial position
            if (glm::distance(glm::vec2(x, z), glm::vec2(dronePosition.x, dronePosition.z)) < margin) {
                continue; // Regenerate position if too close to drone
            }

            positionValid = true;

            // Check distance from all existing obstacles
            for (const auto& obs : obstacles) {
                float distX = x - glm::value_ptr(obs.modelMatrix)[12];
                float distZ = z - glm::value_ptr(obs.modelMatrix)[14];

                if (sqrt(distX * distX + distZ * distZ) < 20) { // Minimum distance between obstacles
                    positionValid = false;
                    break;
                }
            }
        }

        // Generate a random rotation angle for the entire frame
        float randomRotationAngle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), randomRotationAngle, glm::vec3(0, 1, 0));

        float frameSize = 4.0f;      // Size of the frame sides
        float frameThickness = 0.05f; // Thickness of the bars
        float frameHeight = 4.0f;    // Height of the frame from the ground

        // Function to create a rotated and translated matrix
        auto createRotatedMatrix = [&](const glm::vec3& translation, const glm::vec3& scale) {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(x, 0, z)); // Translate to base position
            modelMatrix = modelMatrix * rotationMatrix; // Apply random rotation
            modelMatrix = glm::translate(modelMatrix, translation); // Adjust local position
            modelMatrix = glm::scale(modelMatrix, scale);
            return modelMatrix;
            };

        // Bottom bar (white side)
        glm::mat4 bottomBarModelMatrix = createRotatedMatrix(
            glm::vec3(0, frameHeight, 0),
            glm::vec3(frameSize, frameThickness, frameThickness)
        );
        obstacles.push_back({ bottomBarModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Top bar (white side)
        glm::mat4 topBarModelMatrix = createRotatedMatrix(
            glm::vec3(0, frameHeight + frameSize, 0),
            glm::vec3(frameSize, frameThickness, frameThickness)
        );
        obstacles.push_back({ topBarModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Left bar (white side)
        glm::mat4 leftBarModelMatrix = createRotatedMatrix(
            glm::vec3(-frameSize / 2, frameHeight + frameSize / 2, 0),
            glm::vec3(frameThickness, frameSize, frameThickness)
        );
        obstacles.push_back({ leftBarModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Right bar (white side)
        glm::mat4 rightBarModelMatrix = createRotatedMatrix(
            glm::vec3(frameSize / 2, frameHeight + frameSize / 2, 0),
            glm::vec3(frameThickness, frameSize, frameThickness)
        );
        obstacles.push_back({ rightBarModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Left leg
        glm::mat4 leftLegModelMatrix = createRotatedMatrix(
            glm::vec3(-frameSize / 2, frameHeight / 2, 0),
            glm::vec3(frameThickness, frameHeight, frameThickness)
        );
        obstacles.push_back({ leftLegModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Right leg
        glm::mat4 rightLegModelMatrix = createRotatedMatrix(
            glm::vec3(frameSize / 2, frameHeight / 2, 0),
            glm::vec3(frameThickness, frameHeight, frameThickness)
        );
        obstacles.push_back({ rightLegModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f), "box", true });

        // Colored Frame
        frameSize = 4.4f;
        frameHeight = 3.8f;
        frameThickness = 0.15f;

        // Bottom bar (red)
        glm::mat4 bottomBarModelMatrix1 = createRotatedMatrix(
            glm::vec3(0, frameHeight, 0),
            glm::vec3(frameSize, frameThickness, 0.01f)
        );
        obstacles.push_back({ bottomBarModelMatrix1, glm::vec3(1.0f, 0.0f, 0.0f), "box", true });

        // Top bar (red)
        glm::mat4 topBarModelMatrix1 = createRotatedMatrix(
            glm::vec3(0, frameHeight + frameSize, 0),
            glm::vec3(frameSize, frameThickness, 0.01f)
        );
        obstacles.push_back({ topBarModelMatrix1, glm::vec3(1.0f, 0.0f, 0.0f), "box", true });

        // Left bar (red)
        glm::mat4 leftBarModelMatrix1 = createRotatedMatrix(
            glm::vec3(-frameSize / 2, frameHeight + frameSize / 2, 0),
            glm::vec3(frameThickness, frameSize, 0.01f)
        );
        obstacles.push_back({ leftBarModelMatrix1, glm::vec3(1.0f, 0.0f, 0.0f), "box", true });

        // Right bar (red)
        glm::mat4 rightBarModelMatrix1 = createRotatedMatrix(
            glm::vec3(frameSize / 2, frameHeight + frameSize / 2, 0),
            glm::vec3(frameThickness, frameSize, 0.01f)
        );
        obstacles.push_back({ rightBarModelMatrix1, glm::vec3(1.0f, 0.0f, 0.0f), "box", true });

        // Bottom bar (green)
        glm::mat4 bottomBarModelMatrix2 = createRotatedMatrix(
            glm::vec3(0, frameHeight, -0.01f),
            glm::vec3(frameSize, frameThickness, 0.01f)
        );
        obstacles.push_back({ bottomBarModelMatrix2, glm::vec3(0.0f, 1.0f, 0.0f), "box", true });

        // Top bar (green)
        glm::mat4 topBarModelMatrix2 = createRotatedMatrix(
            glm::vec3(0, frameHeight + frameSize, -0.01f),
            glm::vec3(frameSize, frameThickness, 0.01f)
        );
        obstacles.push_back({ topBarModelMatrix2, glm::vec3(0.0f, 1.0f, 0.0f), "box", true });

        // Left bar (green)
        glm::mat4 leftBarModelMatrix2 = createRotatedMatrix(
            glm::vec3(-frameSize / 2, frameHeight + frameSize / 2, -0.01f),
            glm::vec3(frameThickness, frameSize, 0.01f)
        );
        obstacles.push_back({ leftBarModelMatrix2, glm::vec3(0.0f, 1.0f, 0.0f), "box", true });

        // Right bar (green)
        glm::mat4 rightBarModelMatrix2 = createRotatedMatrix(
            glm::vec3(frameSize / 2, frameHeight + frameSize / 2, -0.01f),
            glm::vec3(frameThickness, frameSize, 0.01f)
        );
        obstacles.push_back({ rightBarModelMatrix2, glm::vec3(0.0f, 1.0f, 0.0f), "box", true });
    }

    for (int i = 0; i < treeCount2; ++i) {
        bool positionValid = false;
        float x, z;

        while (!positionValid) {
            generateRandomPosition(x, z);

            // Ensure the tree is not too close to the drone's initial position
            if (glm::distance(glm::vec2(x, z), glm::vec2(dronePosition.x, dronePosition.z)) < margin) {
                continue; // Regenerate position if too close to drone
            }

            positionValid = true;

            // Check distance from all existing obstacles
            for (const auto& obs : obstacles) {
                float distX = x - glm::value_ptr(obs.modelMatrix)[12];
                float distZ = z - glm::value_ptr(obs.modelMatrix)[14];

                if (sqrt(distX * distX + distZ * distZ) < 10) {
                    positionValid = false;
                    break;
                }
            }
        }

        // Create tree trunk
        glm::mat4 trunkModelMatrix = glm::mat4(1.0f);
        trunkModelMatrix = glm::translate(trunkModelMatrix, glm::vec3(x, 3, z));
        trunkModelMatrix = glm::scale(trunkModelMatrix, glm::vec3(0.8f, 3.0f, 0.8f));
        obstacles.push_back({ trunkModelMatrix, glm::vec3(0.5f, 0.3f, 0.0f), "cylinder" });

        // Create tree foliage (cones)
        glm::mat4 foliageModelMatrix1 = glm::mat4(1.0f);
        foliageModelMatrix1 = glm::translate(foliageModelMatrix1, glm::vec3(x, 7.5f, z));
        foliageModelMatrix1 = glm::scale(foliageModelMatrix1, glm::vec3(2.5f, 3.0f, 2.5f));
        obstacles.push_back({ foliageModelMatrix1, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });

        glm::mat4 foliageModelMatrix2 = glm::mat4(1.0f);
        foliageModelMatrix2 = glm::translate(foliageModelMatrix2, glm::vec3(x, 9, z));
        foliageModelMatrix2 = glm::scale(foliageModelMatrix2, glm::vec3(2.25f, 2.5f, 2.25f));
        obstacles.push_back({ foliageModelMatrix2, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });

        glm::mat4 foliageModelMatrix3 = glm::mat4(1.0f);
        foliageModelMatrix3 = glm::translate(foliageModelMatrix3, glm::vec3(x, 10.5f, z));
        foliageModelMatrix3 = glm::scale(foliageModelMatrix3, glm::vec3(2.0f, 2.0f, 2.0f));
        obstacles.push_back({ foliageModelMatrix3, glm::vec3(0.0f, 0.7f, 0.0f), "cone" });
    }

    for (int i = 0; i < buildingCount; ++i) {
        bool positionValid = false;
        float x, z;

        while (!positionValid) {
            generateRandomPosition(x, z);

            // Ensure the building is not too close to the drone's initial position
            if (glm::distance(glm::vec2(x, z), glm::vec2(dronePosition.x, dronePosition.z)) < margin) {
                continue; // Regenerate position if too close to drone
            }

            positionValid = true;

            // Check distance from all existing obstacles
            for (const auto& obs : obstacles) {
                float distX = x - glm::value_ptr(obs.modelMatrix)[12];
                float distZ = z - glm::value_ptr(obs.modelMatrix)[14];

                if (sqrt(distX * distX + distZ * distZ) < 10) {
                    positionValid = false;
                    break;
                }
            }
        }

        // Create building base
        glm::mat4 buildingModelMatrix = glm::mat4(1.0f);
        buildingModelMatrix = glm::translate(buildingModelMatrix, glm::vec3(x, 2, z));
        buildingModelMatrix = glm::scale(buildingModelMatrix, glm::vec3(5.0f, 4.0f, 5.0f));
        obstacles.push_back({ buildingModelMatrix, glm::vec3(0.18f, 0.31f, 0.31f), "box" });

        // Create building roof
        glm::mat4 roofModelMatrix = glm::mat4(1.0f);
        roofModelMatrix = glm::translate(roofModelMatrix, glm::vec3(x, 6, z));
        roofModelMatrix = glm::scale(roofModelMatrix, glm::vec3(3.5f, 2.0f, 3.5f));
        obstacles.push_back({ roofModelMatrix, glm::vec3(0.58f, 0.0f, 0.13f), "pyramid" });
    }
}


void Tema2::RenderTerrain() {
    RenderSimpleMesh(meshes["terrain"], shaders["GroundShader"], glm::mat4(1));
}

void Tema2::OnKeyPress(int key, int mods)
{
    if (key == GLFW_KEY_T)
    {
        renderCameraTarget = !renderCameraTarget;
    }

    if (key == GLFW_KEY_O)
    {
        isPerspective = false;
        projectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, 0.01f, 200.0f);
    }

    if (key == GLFW_KEY_P)
    {
        isPerspective = true;
        projectionMatrix = glm::perspective(RADIANS(fov), window->props.aspectRatio, 0.01f, 200.0f);
    }

    if (key == GLFW_KEY_C)
    {
        isCameraFirstPerson = !isCameraFirstPerson;
    }
}

void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix, const glm::vec3& color) {
    if (!mesh || !shader || !shader->GetProgramID())
        return;

    // If mesh is a grid, skip rendering
    if (mesh->GetMeshID() == "grid") {
        return;
    }

    glUseProgram(shader->program);

    GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
    glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    GLint loc_view_matrix = glGetUniformLocation(shader->program, "View");
    glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    GLint loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
    glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    GLint object_color_loc = glGetUniformLocation(shader->program, "object_color");
    glUniform3fv(object_color_loc, 1, glm::value_ptr(color));

    glBindVertexArray(mesh->GetBuffers()->m_VAO);
    glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}


void Tema2::OnKeyRelease(int key, int mods)
{
}

void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Prevent mouse movement when not in first-person mode
    if (!isCameraFirstPerson) return;

    // Update drone rotation around Y-axis (horizontal mouse movement)
    droneRotationY -= deltaX * MOUSE_SENSITIVITY * 4;

    // Update drone pitch (vertical mouse movement) with angle limits
    float maxPitchAngle = RADIANS(80.0f);  // Maximum pitch angle (30 degrees)
    droneRotationX -= deltaY * MOUSE_SENSITIVITY;

    // Clamp the pitch rotation to prevent excessive tilting
    droneRotationX = glm::clamp(droneRotationX, -maxPitchAngle, maxPitchAngle);
}

void Tema2::HandleDroneCrash(float deltaTimeSeconds) {
    if (!isDroneCrashed) return;

    crashTimer += deltaTimeSeconds;

    // Handle drone crash animation
    if (crashTimer < MAX_CRASH_DURATION) {
        // Predict the new position after decreasing height
        glm::vec3 newPosition = dronePosition;
        newPosition.y -= 2 * deltaTimeSeconds;

        if (!CheckPreciseCollision(newPosition)) {
            if (currentPackage.isCollected && newPosition.y > 1.7f) {
                dronePosition = newPosition;
            }
            if (!currentPackage.isCollected && newPosition.y > 0.2f) {
                dronePosition = newPosition;
            }
        }
        else {
            dronePosition.y = glm::max(dronePosition.y, 0.2f);
        }

        cameraShakeIntensity = glm::max(0.0f, 1.0f - (crashTimer / MAX_CRASH_DURATION));
    }
    else {
        isDroneCrashed = false;
        crashTimer = 0.0f;
        cameraShakeIntensity = 0.0f;
    }
}

void Tema2::ApplyCameraShake(glm::vec3& cameraPosition)
{
    if (cameraShakeIntensity <= 0.0f) return;

    // Generate random shake based on intensity
    float shakeAmount = 0.2f * cameraShakeIntensity;
    cameraPosition.x += (rand() % 100 / 100.0f - 0.5f) * shakeAmount;
    cameraPosition.y += (rand() % 100 / 100.0f - 0.5f) * shakeAmount;
    cameraPosition.z += (rand() % 100 / 100.0f - 0.5f) * shakeAmount;
}

void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
}

void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
}

void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
}

void Tema2::OnWindowResize(int width, int height)
{
}
