#include "opengl/Texture.h"
#include "core/Sprite.h"

#include <memory>
#include <filesystem>


/**
 * @brief Texture atlas sprite representation
 */
class SpriteSheet
{
public:
    SpriteSheet(std::shared_ptr<Texture> texture);
    ~SpriteSheet();

    static SpriteSheet fromJson(const std::filesystem::path &jsonPath);

    void addSprite(Sprite &&sprite);
    std::vector<Sprite> getSprites();
    std::shared_ptr<Texture> getTexture();

private:
    std::shared_ptr<Texture> m_texture;
    std::vector<Sprite> m_sprites;
};
