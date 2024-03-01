#include <iostream>

class SpriteRenderer
{
private:
    const char* tag = "SpriteRenderer";
public:
    SpriteRenderer(/* args */);
    ~SpriteRenderer();

    void setBufferData();
    void draw();
};