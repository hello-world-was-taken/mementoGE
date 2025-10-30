#include "core/components/Sprite.h"

std::array<glm::vec2, 4> Sprite::getNormalizedTextureCoordinates() const
{
    if (!texture)
        return {glm::vec2{0.0f}, glm::vec2{0.0f}, glm::vec2{0.0f}, glm::vec2{0.0f}};

    float texW = static_cast<float>(texture->getWidth());
    float texH = static_cast<float>(texture->getHeight());

    glm::vec2 topLeftLocal = {topLeft.x / texW, topLeft.y / texH};
    glm::vec2 bottomLeft = {topLeft.x / texW, (topLeft.y + height) / texH};
    glm::vec2 bottomRight = {(topLeft.x + width) / texW, (topLeft.y + height) / texH};
    glm::vec2 topRight = {(topLeft.x + width) / texW, topLeft.y / texH};

    auto coords = std::array{topLeftLocal, bottomLeft, bottomRight, topRight};

    if (flipX)
    {
        std::swap(coords[0], coords[3]);
        std::swap(coords[1], coords[2]);
    }
    if (flipY)
    {
        std::swap(coords[0], coords[1]);
        std::swap(coords[3], coords[2]);
    }

    return coords;
}

#ifdef EDITOR_BUILD
#include "core/GlResourceManager.h"

void Sprite::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Sprite";
    out << YAML::Value << YAML::BeginMap;

    // Serialize top-left position and size
    out << YAML::Key << "TopLeft" << YAML::Value << YAML::BeginSeq << topLeft.x << topLeft.y << YAML::EndSeq;
    out << YAML::Key << "Width" << YAML::Value << width;
    out << YAML::Key << "Height" << YAML::Value << height;

    // Serialize color
    out << YAML::Key << "Color" << YAML::Value << YAML::BeginSeq << color.r << color.g << color.b << color.a
        << YAML::EndSeq;

    // Serialize flip flags
    out << YAML::Key << "FlipX" << YAML::Value << flipX;
    out << YAML::Key << "FlipY" << YAML::Value << flipY;

    if (texture)
        texture->serialize(out);

    out << YAML::EndMap;
}

void Sprite::deserialize(const YAML::Node &in)
{
    // Deserialize top-left, width, height
    auto topLeftNode = in["Sprite"]["TopLeft"];
    topLeft = {topLeftNode[0].as<float>(), topLeftNode[1].as<float>()};
    width = in["Sprite"]["Width"].as<float>();
    height = in["Sprite"]["Height"].as<float>();

    // Deserialize color
    auto colorNode = in["Sprite"]["Color"];
    color = {colorNode[0].as<float>(), colorNode[1].as<float>(), colorNode[2].as<float>(), colorNode[3].as<float>()};

    // Deserialize flip flags
    flipX = in["Sprite"]["FlipX"] ? in["Sprite"]["FlipX"].as<bool>() : false;
    flipY = in["Sprite"]["FlipY"] ? in["Sprite"]["FlipY"].as<bool>() : false;

    // Deserialize texture
    auto textureNode = in["Sprite"]["Texture"];
    std::string filePath = textureNode["FilePath"].as<std::string>();
    bool isTextureAtlas = textureNode["isTextureAtlas"].as<bool>();
    texture = GlResourceManager::instance().getTexture(filePath, isTextureAtlas);
    texture->bind();
}

void Sprite::drawInspector(Sprite &sprite)
{
    if (ImGui::CollapsingHeader("Sprite Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Color", &sprite.color.x);
        ImGui::Checkbox("Flip X", &sprite.flipX);
        ImGui::Checkbox("Flip Y", &sprite.flipY);

        ImGui::Text("Texture: %s", sprite.texture ? sprite.texture->getFilePath().c_str() : "None");

        ImGui::Separator();
        ImGui::Text("Size: %.1f x %.1f", sprite.width, sprite.height);
    }
}
#endif // EDITOR_BUILD
