#include "GDPHYSX-Master1.h"
#include <random>
#include <chrono>
#include "Camera.h"
#include "Particle.h"
#include "Shader.h"

using namespace std;

enum CameraType {
    THIRDPERSON,
    FIRSTPERSON,
    TOPDOWN
};

CameraType cameraType = TOPDOWN;

#ifndef PERSPECTIVE_CAM_H

class PerspectiveCamera : public Physics::Camera
{
public:
    PerspectiveCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = Physics::YAW,
        float pitch = Physics::PITCH)
        : Camera(position, up, yaw, pitch)
    {
        updateCameraVectors();
    }
};

#endif

#ifndef ORTHOGRAPHIC_CAM_H

class OrthographicCamera : public Physics::Camera
{
public:
    OrthographicCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = Physics::YAW,
        float pitch = Physics::PITCH)
        : Camera(position, up, yaw, pitch)
    {
        updateCameraVectors();
    }
};

#endif

float windowWidth = 800;
float windowHeight = 800;

std::mt19937 rng(42); // seed for reproducibility
std::uniform_real_distribution<float> dist(-25.f, 25.f);

PerspectiveCamera thirdPerson(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
PerspectiveCamera firstPerson(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
OrthographicCamera topDown(glm::vec3(0.f, 0.0f, 1.0f));

float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;
bool firstMouse = true;

//Global variables
float deltaTime = 0.f;
float lastFrame = 0.f;
bool allowLightControl = false;
bool perspectiveSwitch = false;
bool cameraSwitch = false;
bool tankMoved = false;

float orbitYawOffset = 180.f;
float orbitPitch = 25.f;
const float sensitivity = 0.1f;

// Third-person shoulder camera state
glm::vec3 modelPosition = glm::vec3(0.f, 0.f, 10.f);
float modelYaw = 0.f;
float firstPersonYaw = 0.f;
float firstPersonPitch = 0.f;
float firstPersonFOV = 45.f;

// Shoulder camera offsets (tweak these to taste)
const float CAM_DIST = 8.f;
const float CAM_HEIGHT = 2.5f;
const float CAM_SIDE = 0.0f;

glm::vec3 topDownPosition = glm::vec3(0.f, 60.f, 0.f);

class PointLight
{
public:
    glm::vec3 position;
    glm::vec3 color;
    float strength;

    float ambientStr;
    glm::vec3 ambientColor;

    float specularStr;
    float specularPhong;

    float constant;
    float linear;
    float quadratic;
    float range;

    PointLight(glm::vec3 position = glm::vec3(0.f), glm::vec3 color = glm::vec3(1.f),
        float strength = 1.f, float ambientStr = 0.1f, glm::vec3 ambientColor = glm::vec3(1.f),
        float specularStr = 0.5f, float specularPhong = 32.f, float constant = 1.f,
        float linear = 0.09f, float quadratic = 0.032f, float range = 2.f)
        : position(position), color(color), strength(strength),
        ambientStr(ambientStr), ambientColor(ambientColor),
        specularStr(specularStr), specularPhong(specularPhong),
        constant(constant), linear(linear), quadratic(quadratic), range(range)
    {
    }

    void Apply(Physics::Shader& shader, const std::string& name = "pointLight")
    {
        shader.passPointLight(name, position, color, strength,
            ambientStr, ambientColor, specularStr,
            specularPhong, constant, linear, quadratic,
            range);
    }
};

class DirectionalLight
{
public:
    glm::vec3 direction;
    glm::vec3 color;
    float strength;

    float ambientStr;
    glm::vec3 ambientColor;

    float specularStr;
    float specularPhong;

    DirectionalLight(glm::vec3 direction = glm::vec3(0.f, -1.f, 0.f), glm::vec3 color = glm::vec3(1.f),
        float strength = 1.f, float ambientStr = 0.1f, glm::vec3 ambientColor = glm::vec3(1.f),
        float specularStr = 0.5f, float specularPhong = 32.f)
        : direction(direction), color(color), strength(strength),
        ambientStr(ambientStr), ambientColor(ambientColor),
        specularStr(specularStr), specularPhong(specularPhong)
    {
    }

    void Apply(Physics::Shader& shader, const std::string& name = "dirLight")
    {
        shader.passDirLight(name, direction, color, strength,
            ambientStr, ambientColor, specularStr, specularPhong);
    }
};

PointLight pointLight(
    glm::vec3(0.f, 3.f, 5.f), //Pos
    glm::vec3(1.0f, 1.0f, 1.0f),
    2.5f, //strength
    0.1f, //ambientstr
    glm::vec3(1.f, 1.f, 1.f), //ambientColor
    0.3f, //specularStr
    32.0f, //specularPhong
    1.0f, //constant
    0.007f, //linear
    0.0002f, //quadratic
    15.0f //range
);

DirectionalLight dirLight(
    glm::vec3(-0.2f, -1.0f, -0.6f), // direction: high angle, slightly angled like a moon
    glm::vec3(0.6f, 0.7f, 1.0f),    // color: cool blue-white moonlight
    0.2f,                            // strength: dim, moon is much weaker than sun
    //1.f,
    0.08f,                            // ambientStr: dark night ambient
    glm::vec3(0.05f, 0.05f, 0.15f), // ambientColor: deep blue night tint
    0.1f,                             // specularStr: subtle glint
    64.f                              // specularPhong
);

void processInput(GLFWwindow* window)
{
    static bool onePress = false;
    static bool twoPress = false;
    static bool fPress = false;
    static int lightIntensity = 0;
    static CameraType prevMode = TOPDOWN;

    const float lightIntensities[] = { 2.5f, 5.f, 7.5f };

    const float moveSpeed = 20.f;
    const float turnSpeed = 90.f;

    const float fpTurnSpeed = 60.f;
    const float fpMoveSpeed = 10.f;

    const float panSpeed = 20.f;

    //Exit Game
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //ThirdPerson or FirstPerson
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!onePress && cameraType != TOPDOWN)
        {
            cameraType = (cameraType == THIRDPERSON) ? FIRSTPERSON : THIRDPERSON;
            prevMode = cameraType;
        }
        onePress = true;
    }
    else onePress = false;

    //Perspective or Ortho
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    {
        if (!twoPress)
        {
            if (cameraType != TOPDOWN)
            {
                prevMode = cameraType; // save current before switching
                cameraType = TOPDOWN;
                topDownPosition = modelPosition + glm::vec3(0.f, 40.f, 0.f);
            }
            else
            {
                cameraType = prevMode; // return to previous
            }
        }
        twoPress = true;
    }
    else twoPress = false;

    //Point light
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        if (!fPress)
        {
            lightIntensity = (lightIntensity + 1) % 3;
            pointLight.strength = lightIntensities[lightIntensity];
        }
        fPress = true;
    }
    else fPress = false;


    //Move
    if (cameraType == THIRDPERSON)
    {
        // --- Rotate tank in place (A = turn left, D = turn right) -----------
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            modelYaw += turnSpeed * deltaTime;
            tankMoved = true;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            modelYaw -= turnSpeed * deltaTime;
            tankMoved = true;
        }

        // --- Compute tank's local forward vector from its current yaw -------
        glm::vec3 tankForward = glm::normalize(glm::vec3(
            cos(glm::radians(-modelYaw)),
            0.f,
            sin(glm::radians(-modelYaw))
        ));

        // --- Drive forward / backward (W / S) --------------------------------
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            modelPosition += tankForward * moveSpeed * deltaTime;
            tankMoved = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            modelPosition -= tankForward * moveSpeed * deltaTime;
            tankMoved = true;
        }

        if (tankMoved)
        {
            firstPersonYaw = -modelYaw;
            firstPersonPitch = 0.f;
        }
    }
    else if (cameraType == FIRSTPERSON)
    {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            firstPersonYaw -= fpTurnSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            firstPersonYaw += fpTurnSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            firstPersonPitch += fpMoveSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            firstPersonPitch -= fpMoveSpeed * deltaTime;

        // Clamp pitch
        if (firstPersonPitch > 89.f) firstPersonPitch = 89.f;
        if (firstPersonPitch < -89.f) firstPersonPitch = -89.f;

        // Q/E - zoom in/out
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            firstPersonFOV -= fpMoveSpeed * deltaTime * 10.f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            firstPersonFOV += fpMoveSpeed * deltaTime * 10.f;

        if (firstPersonFOV < 10.f) firstPersonFOV = 10.f;
        if (firstPersonFOV > 120.f) firstPersonFOV = 120.f;

    }

    else {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            topDownPosition.x -= panSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            topDownPosition.x += panSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            topDownPosition.z -= panSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            topDownPosition.z += panSpeed * deltaTime;
    }

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    switch (cameraType) {
    case THIRDPERSON:

        orbitYawOffset += xoffset * sensitivity;
        orbitPitch += yoffset * sensitivity;

        if (orbitPitch < 5.f)  orbitPitch = 5.f;
        if (orbitPitch > 80.f) orbitPitch = 80.f;

        thirdPerson.ProcessMouseMovement(orbitYawOffset, orbitPitch);
        break;
    }
}

//MODEL CLASS
#ifndef MODEL_H
class Model {
private:
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    tinyobj::attrib_t attributes;
    std::vector<GLuint> mesh_indices;
    GLuint vao, vbo, ebo, vbo_uv;
    std::vector<GLfloat> fullVertexData;
    GLuint diffuseTexture;
    GLuint normalTexture;
    GLuint overlayTexture;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
    glm::vec3 scale = glm::vec3(1.f);
    Physics::Shader* shader;

public:
    string modelName;
    string texName;
    string normName;
    string overlayName;
    string baseMtl;

    Model(string _name, string _tex, string _file, string _norm = "", string _normFile = "", string _over = "", string _overFile = "", string _mtlPath = "3D/");
    void InitModel();
    void InitCustomModel();
    void InitCube();
    void DrawCube();
    void DrawModel();
    void DeleteBuffers();
    void InitTextures();
    void InitOverlayMap();
    void InitNormals();
    void Scale(glm::vec3 s);
    GLuint GetDiffuse();
    GLuint GetNormal();
    GLuint GetOverlay();
    Physics::Shader* GetShader();
    void AssignShader(Physics::Shader* s);
};

Physics::Shader* Model::GetShader() {
    return this->shader;
}

void Model::AssignShader(Physics::Shader* s) {
    this->shader = s;
}

GLuint Model::GetDiffuse() {
    return this->diffuseTexture;
}

GLuint Model::GetNormal() {
    return this->normalTexture;
}

GLuint Model::GetOverlay() {
    return this->overlayTexture;
}

Model::Model(string _name, string _tex, string _file, string _norm, string _normFile, string _over, string _overFile, string _mtlPath) {
    modelName = "3D/" + _name + ".obj";
    texName = "3D/" + _tex + _file;
    normName = "3D/" + _norm + _normFile;
    overlayName = "3D/" + _over + _overFile;
    baseMtl = _mtlPath;
}

void Model::InitOverlayMap() {
    stbi_set_flip_vertically_on_load(true);
    int img_width3, img_height3, colorChannels3;
    GLenum format3{};
    unsigned char* overlay_bytes = stbi_load(overlayName.c_str(), &img_width3, &img_height3, &colorChannels3, 0);

    if (overlay_bytes) {
        if (colorChannels3 == 1) format3 = GL_RED;
        else if (colorChannels3 == 3) format3 = GL_RGB;
        else if (colorChannels3 == 4) format3 = GL_RGBA;

        //(TEXTURE) Load / bind after shaders
        glGenTextures(1, &overlayTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, overlayTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(
            GL_TEXTURE_2D,
            0, //Texture 0
            format3, //Target color format
            img_width3,
            img_height3,
            0,
            format3,
            GL_UNSIGNED_BYTE,
            overlay_bytes //loaded texture in bytes
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(overlay_bytes);
    }

    else {
        stbi_image_free(overlay_bytes);
    }
}

void Model::InitTextures() {
    stbi_set_flip_vertically_on_load(true);
    int img_width, img_height, colorChannels;
    GLenum format{};
    unsigned char* tex_bytes = stbi_load(texName.c_str(), &img_width, &img_height, &colorChannels, 0);

    if (tex_bytes) {
        GLenum format = (colorChannels == 4) ? GL_RGBA : GL_RGB;

        //(TEXTURE) Load / bind after shaders
        glGenTextures(1, &diffuseTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0, //Texture 0
            format, //Target color format
            img_width,
            img_height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            tex_bytes //loaded texture in bytes
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Anisotropic filtering
        float maxAniso = 8.f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

        stbi_image_free(tex_bytes);
    }

    else {
        stbi_image_free(tex_bytes);
    }
}

void Model::InitNormals() {
    int img_width2, img_height2, colorChannels2;
    unsigned char* normal_bytes = stbi_load(normName.c_str(), &img_width2, &img_height2, &colorChannels2, 0);

    if (normal_bytes) {
        GLenum format = (colorChannels2 == 4) ? GL_RGBA : GL_RGB;
        //(TEXTURE) Load / bind after shaders
        glGenTextures(1, &normalTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTexture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format, //Target color format
            img_width2,
            img_height2,
            0,
            format,
            GL_UNSIGNED_BYTE,
            normal_bytes //loaded texture in bytes
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Anisotropic filtering
        float maxAniso = 8.f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

        stbi_image_free(normal_bytes);
    }
    else {
        stbi_image_free(normal_bytes);
    }
}

void Model::DrawCube() {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, mesh_indices.size(), GL_UNSIGNED_INT, 0);
}

void Model::DrawModel() {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, glm::vec3(0.f, 0.f, 0.f));
    m = glm::scale(m, scale);
    this->shader->setMat4("transform", 1, m);
    //noobShader.setMat4("transform", 1, m);

    glBindVertexArray(vao);
    //Vertex Data Method
    glDrawArrays(GL_TRIANGLES, 0, fullVertexData.size() / 8);
}

void Model::Scale(glm::vec3 s) {
    scale = s;
}

void Model::DeleteBuffers() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Model::InitCube() {
    //Load Object. If success, it loads
    bool success = tinyobj::LoadObj(
        &attributes,
        &shapes,
        &materials,
        &error,
        modelName.c_str()
    );

    for (int i = 0; i < shapes[0].mesh.indices.size(); i++) {
        mesh_indices.push_back(
            shapes[0].mesh.indices[i].vertex_index
        );
    }

    //(SHADERS) Generate vertices and buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &vbo_uv);
    glGenBuffers(1, &ebo);

    //(POSITIONS) VBO
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GL_FLOAT) * attributes.vertices.size(),
        &attributes.vertices[0],
        GL_DYNAMIC_DRAW
    );

    //EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(GLuint) * mesh_indices.size(),
        mesh_indices.data(),
        GL_DYNAMIC_DRAW
    );
    glVertexAttribPointer(
        //0 = Position
        0, // Index / buffer index
        3, // x y z
        GL_FLOAT, // array of floats
        GL_FALSE, // if its normalized
        3 * sizeof(float), // size of data per vertex
        (void*)0
    );
    glEnableVertexAttribArray(0);

    //UV
    glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(float) * (sizeof(UV) / sizeof(UV[0])),
        &UV[0],
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void Model::InitModel() {
    //Load Object. If success, it loads
    bool success = tinyobj::LoadObj(
        &attributes,
        &shapes,
        &materials,
        &error,
        modelName.c_str()
    );



    for (int i = 0; i < shapes.size(); i++) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); j += 3) {
            tinyobj::index_t vData1 = shapes[i].mesh.indices[j];
            tinyobj::index_t vData2 = shapes[i].mesh.indices[j + 1];
            tinyobj::index_t vData3 = shapes[i].mesh.indices[j + 2];

            //Pos
            glm::vec3 v1 = glm::vec3(
                attributes.vertices[vData1.vertex_index * 3],
                attributes.vertices[vData1.vertex_index * 3 + 1],
                attributes.vertices[vData1.vertex_index * 3 + 2]
            );

            glm::vec3 v2 = glm::vec3(
                attributes.vertices[vData2.vertex_index * 3],
                attributes.vertices[vData2.vertex_index * 3 + 1],
                attributes.vertices[vData2.vertex_index * 3 + 2]
            );

            glm::vec3 v3 = glm::vec3(
                attributes.vertices[vData3.vertex_index * 3],
                attributes.vertices[vData3.vertex_index * 3 + 1],
                attributes.vertices[vData3.vertex_index * 3 + 2]
            );

            //UV
            glm::vec2 uv1 = glm::vec2(
                attributes.texcoords[(vData1.texcoord_index * 2)],
                attributes.texcoords[(vData1.texcoord_index * 2) + 1]
            );

            glm::vec2 uv2 = glm::vec2(
                attributes.texcoords[(vData2.texcoord_index * 2)],
                attributes.texcoords[(vData2.texcoord_index * 2) + 1]
            );

            glm::vec2 uv3 = glm::vec2(
                attributes.texcoords[(vData3.texcoord_index * 2)],
                attributes.texcoords[(vData3.texcoord_index * 2) + 1]
            );

            glm::vec3 deltaPos1 = v2 - v1;
            glm::vec3 deltaPos2 = v3 - v1;

            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;

            float r = 1.0f / ((deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x));
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            tangents.push_back(tangent);
            tangents.push_back(tangent);
            tangents.push_back(tangent);

            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
        }
    }

    for (int i = 0; i < shapes.size(); i++) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); j++) {
            tinyobj::index_t vData = shapes[i].mesh.indices[j];
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3)]); //X
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3) + 1]); //Y
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3) + 2]); //Z
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3)]);
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3) + 1]);
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3) + 2]);
            fullVertexData.push_back(attributes.texcoords[(vData.texcoord_index * 2)]); //U
            fullVertexData.push_back(attributes.texcoords[(vData.texcoord_index * 2) + 1]); //V

            fullVertexData.push_back(tangents[i].x);
            fullVertexData.push_back(tangents[i].y);
            fullVertexData.push_back(tangents[i].z);
            fullVertexData.push_back(bitangents[i].x);
            fullVertexData.push_back(bitangents[i].y);
            fullVertexData.push_back(bitangents[i].z);
        }
    }


    GLintptr normalPtr = 3 * sizeof(float);
    GLintptr uvPtr = 6 * sizeof(float);
    GLintptr tangentPtr = 8 * sizeof(float);
    GLintptr bitangentPtr = 11 * sizeof(float);

    //(SHADERS) Generate vertices and buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    //(POSITIONS) VBO
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    //Using fullvertexdata
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GLfloat) * fullVertexData.size(),
        fullVertexData.data(),
        GL_DYNAMIC_DRAW
    );
    glVertexAttribPointer(
        0, // Index / buffer index
        3, // x y z
        GL_FLOAT, // array of floats
        GL_FALSE, // if its normalized
        14 * sizeof(GLfloat), // size of data per vertex
        (void*)0
    );
    glEnableVertexAttribArray(0);

    //Normal
    glVertexAttribPointer(
        1,
        3, // x y z
        GL_FLOAT, // array of floats
        GL_FALSE, // if its normalized
        14 * sizeof(GLfloat), // size of data per vertex
        (void*)normalPtr
    );
    glEnableVertexAttribArray(1);

    //UV
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)uvPtr
    );
    glEnableVertexAttribArray(2);

    //Tangent
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)tangentPtr
    );
    glEnableVertexAttribArray(3);

    //Bitangent
    glVertexAttribPointer(
        4,
        3,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)bitangentPtr
    );
    glEnableVertexAttribArray(4);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Model::InitCustomModel() {
    //Load Object. If success, it loads
    bool success = tinyobj::LoadObj(
        &attributes,
        &shapes,
        &materials,
        &error,
        modelName.c_str(),
        baseMtl.c_str()
    );

    if (texName == "3D/" && !materials.empty() && !materials[0].diffuse_texname.empty())
        texName = baseMtl + materials[0].diffuse_texname;

    if (normName == "3D/" && !materials.empty() && !materials[0].bump_texname.empty())
        normName = baseMtl + materials[0].bump_texname;

    for (int i = 0; i < shapes.size(); i++) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); j += 3) {
            tinyobj::index_t vData1 = shapes[i].mesh.indices[j];
            tinyobj::index_t vData2 = shapes[i].mesh.indices[j + 1];
            tinyobj::index_t vData3 = shapes[i].mesh.indices[j + 2];

            //Pos
            glm::vec3 v1 = glm::vec3(
                attributes.vertices[vData1.vertex_index * 3],
                attributes.vertices[vData1.vertex_index * 3 + 1],
                attributes.vertices[vData1.vertex_index * 3 + 2]
            );

            glm::vec3 v2 = glm::vec3(
                attributes.vertices[vData2.vertex_index * 3],
                attributes.vertices[vData2.vertex_index * 3 + 1],
                attributes.vertices[vData2.vertex_index * 3 + 2]
            );

            glm::vec3 v3 = glm::vec3(
                attributes.vertices[vData3.vertex_index * 3],
                attributes.vertices[vData3.vertex_index * 3 + 1],
                attributes.vertices[vData3.vertex_index * 3 + 2]
            );

            //UV
            glm::vec2 uv1 = glm::vec2(
                attributes.texcoords[(vData1.texcoord_index * 2)],
                attributes.texcoords[(vData1.texcoord_index * 2) + 1]
            );

            glm::vec2 uv2 = glm::vec2(
                attributes.texcoords[(vData2.texcoord_index * 2)],
                attributes.texcoords[(vData2.texcoord_index * 2) + 1]
            );

            glm::vec2 uv3 = glm::vec2(
                attributes.texcoords[(vData3.texcoord_index * 2)],
                attributes.texcoords[(vData3.texcoord_index * 2) + 1]
            );

            glm::vec3 deltaPos1 = v2 - v1;
            glm::vec3 deltaPos2 = v3 - v1;

            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;

            float r = 1.0f / ((deltaUV1.x * deltaUV2.y) - (deltaUV1.y * deltaUV2.x));
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            tangents.push_back(tangent);
            tangents.push_back(tangent);
            tangents.push_back(tangent);

            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
        }
    }

    for (int i = 0; i < shapes.size(); i++) {
        for (int j = 0; j < shapes[i].mesh.indices.size(); j++) {
            tinyobj::index_t vData = shapes[i].mesh.indices[j];
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3)]); //X
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3) + 1]); //Y
            fullVertexData.push_back(attributes.vertices[(vData.vertex_index * 3) + 2]); //Z
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3)]);
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3) + 1]);
            fullVertexData.push_back(attributes.normals[(vData.normal_index * 3) + 2]);
            fullVertexData.push_back(attributes.texcoords[(vData.texcoord_index * 2)]); //U
            fullVertexData.push_back(attributes.texcoords[(vData.texcoord_index * 2) + 1]); //V

            fullVertexData.push_back(tangents[i].x);
            fullVertexData.push_back(tangents[i].y);
            fullVertexData.push_back(tangents[i].z);
            fullVertexData.push_back(bitangents[i].x);
            fullVertexData.push_back(bitangents[i].y);
            fullVertexData.push_back(bitangents[i].z);
        }
    }


    GLintptr normalPtr = 3 * sizeof(float);
    GLintptr uvPtr = 6 * sizeof(float);
    GLintptr tangentPtr = 8 * sizeof(float);
    GLintptr bitangentPtr = 11 * sizeof(float);

    //(SHADERS) Generate vertices and buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    //(POSITIONS) VBO
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    //Using fullvertexdata
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(GLfloat) * fullVertexData.size(),
        fullVertexData.data(),
        GL_DYNAMIC_DRAW
    );
    glVertexAttribPointer(
        0, // Index / buffer index
        3, // x y z
        GL_FLOAT, // array of floats
        GL_FALSE, // if its normalized
        14 * sizeof(GLfloat), // size of data per vertex
        (void*)0
    );
    glEnableVertexAttribArray(0);

    //Normal
    glVertexAttribPointer(
        1,
        3, // x y z
        GL_FLOAT, // array of floats
        GL_FALSE, // if its normalized
        14 * sizeof(GLfloat), // size of data per vertex
        (void*)normalPtr
    );
    glEnableVertexAttribArray(1);

    //UV
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)uvPtr
    );
    glEnableVertexAttribArray(2);

    //Tangent
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)tangentPtr
    );
    glEnableVertexAttribArray(3);

    //Bitangent
    glVertexAttribPointer(
        4,
        3,
        GL_FLOAT,
        GL_FALSE,
        14 * sizeof(GLfloat),
        (void*)bitangentPtr
    );
    glEnableVertexAttribArray(4);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
#endif

#ifndef SKYBOX_H
float skyboxVertices[]{
    -1.f, -1.f, 1.f, //0
    1.f, -1.f, 1.f,  //1
    1.f, -1.f, -1.f, //2
    -1.f, -1.f, -1.f,//3
    -1.f, 1.f, 1.f,  //4
    1.f, 1.f, 1.f,   //5
    1.f, 1.f, -1.f,  //6
    -1.f, 1.f, -1.f  //7
};

//Skybox Indices
unsigned int skyboxIndices[]{
    1,2,6, //R
    6,5,1,

    0,4,7, //L
    7,3,0,

    4,5,6, //T
    6,7,4,

    0,3,2, //B
    2,1,0,

    0,1,5, //Front
    5,4,0,

    3,7,6, //Back
    6,2,3
};

class Skybox {
private:
    GLuint skyVAO, skyboxTex;

public:
    string right, left, top, bottom, front, back;

    Skybox(string _right, string _left, string _top, string _bottom, string _front, string _back);
    void InitSky();
    void DrawSky();
    void InitTextures();
};

Skybox::Skybox(string _right, string _left, string _top, string _bottom, string _front, string _back) {
    right = "Skybox/" + _right + ".png";
    left = "Skybox/" + _left + ".png";
    top = "Skybox/" + _top + ".png";
    bottom = "Skybox/" + _bottom + ".png";
    front = "Skybox/" + _front + ".png";
    back = "Skybox/" + _back + ".png";
}

void Skybox::InitTextures() {
    std::string faceSky[]{
        right,
        left,
        top,
        bottom,
        front,
        back
    };

    //unsigned int skyboxTex;
    glGenTextures(1, &skyboxTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //if 3d this, else only s and t
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (unsigned int i = 0; i < 6; i++) {
        int w, h, skyCh;
        stbi_set_flip_vertically_on_load(false);

        unsigned char* skybytes = stbi_load(faceSky[i].c_str(), &w, &h, &skyCh, 0);

        if (skybytes) {
            GLenum format = (skyCh == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, skybytes);
            stbi_image_free(skybytes);
        }
        else {
            std::cout << "Failed to load skybox face: " << faceSky[i] << std::endl;
            stbi_image_free(skybytes);
        }

    }
}

void Skybox::DrawSky() {
    glBindVertexArray(skyVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void Skybox::InitSky() {
    unsigned int skyVBO, skyEBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glGenBuffers(1, &skyEBO);

    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(skyboxVertices),
        &skyboxVertices,
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(int) * 36,
        &skyboxIndices,
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);
}
#endif


//MAIN
int main(void)
{
    constexpr std::chrono::nanoseconds timestep(16ms);

    GLFWwindow* window;
    /* Initialize the library */
    if (!glfwInit()) return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_SAMPLES, 8);

    window = glfwCreateWindow(windowWidth, windowHeight, "GRAPH-MP_Ragudo_Nikko_Taylan_EJ", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress); // if using CMAKE
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    Physics::Shader noobShader("Shaders/noobShader.vert", "Shaders/noobShader.frag");

    //Load Vertices
    GLfloat vertices[]{
        0.f, 0.5f, 0.f,     //0
        -1.f, -0.f, 0.f,    //1
        0.5f, -0.5f, 0.f    //2
    };
    GLuint indices[]{ 0,1,2 };

    //Skybox skybox("cubemap1_right", "cubemap1_left", "cubemap1_top", "cubemap1_bottom", "cubemap1_front", "cubemap1_back");
    //skybox.InitSky();
    //skybox.InitTextures();

    //Model shipmentObj = Model("Sci-fi Large container", "Sci-fi Container _Base_Color", ".png", "Sci-fi Container _Normal_OpenGL", ".png", "", "", "3D/");

    Model sphereModel = Model("sphere", "", "");

    //Create new model plane then initialize texture, overlay map, and normals
    //newModel.InitModel();
    //newModel.InitTextures();
    //newModel.InitNormals();

    sphereModel.InitModel();
    sphereModel.Scale(glm::vec3(50.f));
    sphereModel.AssignShader(&noobShader);

    //Camera instances
    topDown.Projection = glm::ortho(
        -500.f, //L
        500.f, //R
        -500.f, //Bottom
        500.f, //Top
        -500.0f, //Near
        500.f //Far
    );

    //Enable anti-aliasing and blend
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    using clock = std::chrono::high_resolution_clock;
    auto curr_time = clock::now();
    auto prev_time = curr_time;
    std::chrono::nanoseconds curr_ns(0);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();

        curr_time = clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(curr_time - prev_time);
        prev_time = curr_time;

        curr_ns += dur;
        if (curr_ns >= timestep) {
            constexpr float timestep_sec = timestep.count() / (float)(1E09);
        }


        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*
        //Skybox
        skyboxShader.use();
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.setInt("skybox", 0);
        if (cameraType == THIRDPERSON) skyboxShader.PassSkybox(thirdPerson, lookTarget);
        else skyboxShader.PassOrthoSkybox(topDown);


        skybox.DrawSky();

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        //EOF Skybox
        */
        
        noobShader.use();
        noobShader.passOrthoCamera(topDown);

        /*glm::mat4 sphereM = glm::mat4(1.0f);
        sphereM = glm::translate(sphereM, glm::vec3(0.f, 0.f, 0.f));
        sphereM = glm::scale(sphereM, glm::vec3(50.0f));
        noobShader.setMat4("transform", 1, sphereM);*/
        sphereModel.DrawModel();

        /*
        defaultShader.use();
        if (cameraType == THIRDPERSON) defaultShader.passPerspectiveCamera(thirdPerson, lookTarget);
        else defaultShader.passOrthoCamera(topDown);

        //Pass lights to shader
        pointLight.Apply(defaultShader, "pointLight");
        dirLight.Apply(defaultShader, "dirLight");

        //Floor
        glm::mat4 floor = glm::mat4(1.0f);
        floor = glm::translate(floor, glm::vec3(0.f, 0.f, 0.f));
        floor = glm::scale(floor, glm::vec3(1000.0f));
        defaultShader.setFloat("tiling", 50.f);
        defaultShader.setMat4("transform", 1, floor);
        defaultShader.setBool("useAlphaClip", false);
        defaultShader.LoadTexture(floorModel.GetDiffuse());
        defaultShader.LoadNormal(floorModel.GetNormal());
        floorModel.DrawModel();
        */

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        
    }

    //Tank
    sphereModel.DeleteBuffers();

    //Terminate gl
    glfwTerminate();
    return 0;
}

