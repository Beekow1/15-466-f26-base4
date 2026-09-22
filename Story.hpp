#include <memory>
#include <string>
#include <glm/glm.hpp>

struct StoryNode
{
    std::string text;

    glm::vec2 character_position = glm::vec2(0.0f, 55.0f);
    float lerp_time = 0.5f;

    uint8_t visa = 0; // 0 -> None 1 -> fake 2 _> real

    std::string choiceLText;
    std::shared_ptr<StoryNode> choiceL;

    std::string choiceRText;
    std::shared_ptr<StoryNode> choiceR;

    bool isEnding() const
    {
        return !choiceL && !choiceR;
    }
};

class Story
{
public:
    std::shared_ptr<StoryNode> root = std::make_shared<StoryNode>();
    std::shared_ptr<StoryNode> current = root;

    void chooseLeft()
    {
        if (current->choiceL)
            current = current->choiceL;
        else
            throw "Missing choice";
    }

    void chooseRight()
    {
        if (current->choiceR)
            current = current->choiceR;
        else
            throw "Missing choice";
    }

    void restart()
    {
        current = root;
    }
};