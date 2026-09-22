#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "TextRenderer.hpp"
#include "Story.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	TextRenderer text;

	struct Image
	{
		GLuint texture = 0;
		glm::uvec2 size = glm::uvec2(0);
	};

	glm::vec2 character_position = glm::vec2(135.0f, 55.0f);
	glm::vec2 last_position = character_position;
	float char_timer = 0.0f;

	Image backdrop, character, foreground;
	Image fake_visa, real_visa;
	GLuint image_vao = 0, image_vbo = 0;
	Story story;

	void generate_story();

	Image load_image(std::string const &filename);
	void draw_image(Image const &image, glm::vec2 position);
};
