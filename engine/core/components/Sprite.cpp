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

void EditorExtensions::serializeSprite(YAML::Emitter &out, const Sprite &sprite)
{
    out << YAML::Key << "Sprite";
    out << YAML::Value << YAML::BeginMap;

    // Serialize top-left position and size
    out << YAML::Key << "TopLeft" << YAML::Value
        << YAML::BeginSeq << sprite.topLeft.x << sprite.topLeft.y << YAML::EndSeq;
    out << YAML::Key << "Width" << YAML::Value << sprite.width;
    out << YAML::Key << "Height" << YAML::Value << sprite.height;

    // Serialize color
    out << YAML::Key << "Color" << YAML::Value
        << YAML::BeginSeq << sprite.color.r << sprite.color.g << sprite.color.b << sprite.color.a << YAML::EndSeq;

    // Serialize flip flags
    out << YAML::Key << "FlipX" << YAML::Value << sprite.flipX;
    out << YAML::Key << "FlipY" << YAML::Value << sprite.flipY;

    if (sprite.texture)
        sprite.texture->serialize(out);

    out << YAML::EndMap;
}

void EditorExtensions::deserializeSprite(const YAML::Node &in, Sprite &sprite)
{
    // Deserialize top-left, width, height
    auto topLeftNode = in["Sprite"]["TopLeft"];
    sprite.topLeft = {topLeftNode[0].as<float>(), topLeftNode[1].as<float>()};
    sprite.width = in["Sprite"]["Width"].as<float>();
    sprite.height = in["Sprite"]["Height"].as<float>();

    // Deserialize color
    auto colorNode = in["Sprite"]["Color"];
    sprite.color = {
        colorNode[0].as<float>(),
        colorNode[1].as<float>(),
        colorNode[2].as<float>(),
        colorNode[3].as<float>()};

    // Deserialize flip flags
    sprite.flipX = in["Sprite"]["FlipX"] ? in["Sprite"]["FlipX"].as<bool>() : false;
    sprite.flipY = in["Sprite"]["FlipY"] ? in["Sprite"]["FlipY"].as<bool>() : false;

    // Deserialize texture
    auto textureNode = in["Sprite"]["Texture"];
    std::string filePath = textureNode["FilePath"].as<std::string>();
    bool isTextureAtlas = textureNode["isTextureAtlas"].as<bool>();
    sprite.texture = GlResourceManager::instance().getTexture(filePath, isTextureAtlas);
    sprite.texture->bind();
}

void EditorExtensions::drawSpriteInspector(Sprite &sprite)
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
